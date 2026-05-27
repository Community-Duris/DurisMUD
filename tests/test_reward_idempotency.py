#!/usr/bin/env python3
"""Validate reward persistence idempotency against the Docker MySQL DB.

Reward writes are visible gameplay events. Retrying a reward SQL job with the
same event key must not create a second epic gain, quest completion, or zone
touch row.

Run from the repository root with the Docker MySQL service running:

    python tests/test_reward_idempotency.py
"""

from __future__ import annotations

import os
import subprocess
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
    mysql(
        dedent(
            """
            DROP TABLE IF EXISTS reward_epic_gain_test;
            DROP TABLE IF EXISTS reward_world_quest_test;
            DROP TABLE IF EXISTS reward_zone_touches_test;

            CREATE TABLE reward_epic_gain_test (
              id INT UNSIGNED NOT NULL AUTO_INCREMENT,
              event_key VARCHAR(128) DEFAULT NULL,
              pid BIGINT NOT NULL DEFAULT 0,
              time DATETIME NOT NULL,
              type INT NOT NULL DEFAULT 0,
              type_id INT NOT NULL DEFAULT 0,
              epics INT NOT NULL DEFAULT 0,
              PRIMARY KEY (id),
              UNIQUE KEY reward_event_key (event_key)
            ) ENGINE=InnoDB;

            CREATE TABLE reward_world_quest_test (
              id INT UNSIGNED NOT NULL AUTO_INCREMENT,
              event_key VARCHAR(128) DEFAULT NULL,
              pid VARCHAR(45) NOT NULL DEFAULT '',
              timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
              quest_giver INT UNSIGNED NOT NULL DEFAULT 0,
              player_name VARCHAR(45) NOT NULL DEFAULT '',
              player_level INT UNSIGNED NOT NULL DEFAULT 0,
              quest_target INT NOT NULL DEFAULT 0,
              reward_vnum INT NOT NULL DEFAULT 0,
              reward_desc VARCHAR(255) NOT NULL DEFAULT '',
              PRIMARY KEY (id),
              UNIQUE KEY reward_event_key (event_key)
            ) ENGINE=InnoDB;

            CREATE TABLE reward_zone_touches_test (
              id INT NOT NULL AUTO_INCREMENT,
              event_key VARCHAR(128) DEFAULT NULL,
              boot_time INT DEFAULT NULL,
              zone_number INT DEFAULT NULL,
              touched_at INT DEFAULT NULL,
              toucher_pid INT DEFAULT NULL,
              group_size INT DEFAULT NULL,
              epic_value INT DEFAULT NULL,
              alignment_delta INT DEFAULT NULL,
              PRIMARY KEY (id),
              UNIQUE KEY reward_event_key (event_key)
            ) ENGINE=InnoDB;
            """
        )
    )

    mysql(
        dedent(
            """
            INSERT INTO reward_epic_gain_test
              (event_key,pid,time,type,type_id,epics)
              VALUES ('epic:pid100:quest42',100,NOW(),3,42,8)
              ON DUPLICATE KEY UPDATE event_key=event_key;
            INSERT INTO reward_epic_gain_test
              (event_key,pid,time,type,type_id,epics)
              VALUES ('epic:pid100:quest42',100,NOW(),3,42,8)
              ON DUPLICATE KEY UPDATE event_key=event_key;

            INSERT INTO reward_world_quest_test
              (event_key,pid,quest_giver,player_name,player_level,quest_target,reward_vnum,reward_desc)
              VALUES ('quest:pid100:target42','100',500,'Tester',51,42,1234,'Reward')
              ON DUPLICATE KEY UPDATE event_key=event_key;
            INSERT INTO reward_world_quest_test
              (event_key,pid,quest_giver,player_name,player_level,quest_target,reward_vnum,reward_desc)
              VALUES ('quest:pid100:target42','100',500,'Tester',51,42,1234,'Reward')
              ON DUPLICATE KEY UPDATE event_key=event_key;

            INSERT INTO reward_zone_touches_test
              (event_key,boot_time,touched_at,zone_number,toucher_pid,group_size,epic_value,alignment_delta)
              VALUES ('zone:pid100:zone300',111,222,300,100,6,25,1)
              ON DUPLICATE KEY UPDATE event_key=event_key;
            INSERT INTO reward_zone_touches_test
              (event_key,boot_time,touched_at,zone_number,toucher_pid,group_size,epic_value,alignment_delta)
              VALUES ('zone:pid100:zone300',111,222,300,100,6,25,1)
              ON DUPLICATE KEY UPDATE event_key=event_key;
            """
        )
    )

    assert_equal(
        mysql("SELECT COUNT(*), COALESCE(SUM(epics),0) FROM reward_epic_gain_test;"),
        "1\t8",
        "duplicate epic gain event_key must not double-count epics",
    )
    assert_equal(
        mysql("SELECT COUNT(*) FROM reward_world_quest_test;"),
        "1",
        "duplicate quest completion event_key must not create another completion",
    )
    assert_equal(
        mysql("SELECT COUNT(*) FROM reward_zone_touches_test;"),
        "1",
        "duplicate zone touch event_key must not create another audit row",
    )

    print("reward idempotency SQL checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
