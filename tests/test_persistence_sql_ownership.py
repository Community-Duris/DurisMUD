#!/usr/bin/env python3
"""Validate persistence ownership SQL semantics against the Docker MySQL DB.

The game worker writes every item movement to an audit table, then upserts one
current-owner row keyed by item_uid.  These tests protect the anti-duplication
contract: one persistent UID can have an audit trail, but only one current owner.

Run from the repository root with the Docker MySQL service running:

    python tests/test_persistence_sql_ownership.py

Override the container/database/user with:

    DURIS_MYSQL_CONTAINER=durismud-mysql-1 DURIS_TEST_DB=duris_test \
      DURIS_TEST_DB_USER=duris DURIS_TEST_DB_PASSWORD=duris \
      python tests/test_persistence_sql_ownership.py
"""

from __future__ import annotations

import os
import subprocess
import sys
from textwrap import dedent


def mysql(sql: str) -> str:
    container = os.environ.get("DURIS_MYSQL_CONTAINER", "durismud-mysql-1")
    database = os.environ.get("DURIS_TEST_DB", "duris_test")
    user = os.environ.get("DURIS_TEST_DB_USER", "duris")
    password = os.environ.get("DURIS_TEST_DB_PASSWORD", "duris")
    cmd = [
        "docker",
        "exec",
        "-i",
        container,
        "mysql",
        f"-u{user}",
        f"-p{password}",
        "--batch",
        "--skip-column-names",
        database,
    ]
    result = subprocess.run(cmd, input=sql, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(
            f"MySQL command failed with code {result.returncode}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result.stdout.strip()


def assert_equal(actual: str, expected: str, message: str) -> None:
    if actual != expected:
        raise AssertionError(f"{message}\nexpected: {expected!r}\nactual:   {actual!r}")


def main() -> int:
    setup = dedent(
        """
        DROP TABLE IF EXISTS persistence_item_event_audit_test;
        DROP TABLE IF EXISTS persistence_items_current_test;

        CREATE TABLE persistence_item_event_audit_test (
          event_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
          event_time BIGINT UNSIGNED NOT NULL,
          event_type VARCHAR(64) NOT NULL,
          item_uid BIGINT UNSIGNED NOT NULL,
          owner_type VARCHAR(32) NOT NULL,
          owner_ref VARCHAR(64) NOT NULL,
          actor_id INT NOT NULL DEFAULT -1,
          vnum INT NOT NULL DEFAULT -1,
          item_name VARCHAR(255) NOT NULL DEFAULT '',
          raw_event TEXT NOT NULL,
          created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
          PRIMARY KEY (event_id),
          KEY item_time (item_uid,event_time),
          KEY owner_lookup (owner_type,owner_ref)
        ) ENGINE=InnoDB;

        CREATE TABLE persistence_items_current_test (
          item_uid BIGINT UNSIGNED NOT NULL,
          owner_type VARCHAR(32) NOT NULL,
          owner_ref VARCHAR(64) NOT NULL,
          event_time BIGINT UNSIGNED NOT NULL,
          event_type VARCHAR(64) NOT NULL,
          actor_id INT NOT NULL DEFAULT -1,
          vnum INT NOT NULL DEFAULT -1,
          item_name VARCHAR(255) NOT NULL DEFAULT '',
          updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
            ON UPDATE CURRENT_TIMESTAMP,
          PRIMARY KEY (item_uid),
          KEY owner_lookup (owner_type,owner_ref)
        ) ENGINE=InnoDB;
        """
    )
    mysql(setup)

    upsert_template = """
      INSERT INTO persistence_item_event_audit_test
        (event_time,event_type,item_uid,owner_type,owner_ref,actor_id,vnum,item_name,raw_event)
      VALUES ({ts},'{event}',{uid},'{owner_type}','{owner_ref}',{actor},{vnum},'{name}','{raw}');

      INSERT INTO persistence_items_current_test
        (item_uid,owner_type,owner_ref,event_time,event_type,actor_id,vnum,item_name)
      VALUES ({uid},'{owner_type}','{owner_ref}',{ts},'{event}',{actor},{vnum},'{name}')
      ON DUPLICATE KEY UPDATE
        owner_type=IF(VALUES(event_time) >= event_time, VALUES(owner_type), owner_type),
        owner_ref=IF(VALUES(event_time) >= event_time, VALUES(owner_ref), owner_ref),
        event_type=IF(VALUES(event_time) >= event_time, VALUES(event_type), event_type),
        actor_id=IF(VALUES(event_time) >= event_time, VALUES(actor_id), actor_id),
        vnum=IF(VALUES(event_time) >= event_time, VALUES(vnum), vnum),
        item_name=IF(VALUES(event_time) >= event_time, VALUES(item_name), item_name),
        event_time=GREATEST(event_time, VALUES(event_time));
    """

    mysql(
        upsert_template.format(
            ts=100,
            event="owner_player",
            uid=900000000001,
            owner_type="player",
            owner_ref="101",
            actor=101,
            vnum=1234,
            name="test sword",
            raw="first owner",
        )
    )
    mysql(
        upsert_template.format(
            ts=120,
            event="owner_container",
            uid=900000000001,
            owner_type="item",
            owner_ref="900000000099",
            actor=-1,
            vnum=1234,
            name="test sword",
            raw="newer container owner",
        )
    )
    mysql(
        upsert_template.format(
            ts=90,
            event="owner_player",
            uid=900000000001,
            owner_type="player",
            owner_ref="202",
            actor=202,
            vnum=1234,
            name="test sword",
            raw="stale owner should not win",
        )
    )

    current = mysql(
        "SELECT COUNT(*), owner_type, owner_ref, event_time "
        "FROM persistence_items_current_test WHERE item_uid=900000000001;"
    )
    assert_equal(current, "1\titem\t900000000099\t120", "newest owner must be the only current row")

    audit_count = mysql(
        "SELECT COUNT(*) FROM persistence_item_event_audit_test "
        "WHERE item_uid=900000000001;"
    )
    assert_equal(audit_count, "3", "audit trail should retain every ownership event")

    mysql(
        upsert_template.format(
            ts=130,
            event="owner_destroyed",
            uid=900000000001,
            owner_type="destroyed",
            owner_ref="0",
            actor=-1,
            vnum=1234,
            name="test sword",
            raw="destroyed",
        )
    )

    destroyed = mysql(
        "SELECT owner_type, owner_ref, event_time "
        "FROM persistence_items_current_test WHERE item_uid=900000000001;"
    )
    assert_equal(destroyed, "destroyed\t0\t130", "destroyed event should clear live ownership")

    print("Persistence SQL ownership tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
