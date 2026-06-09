#!/usr/bin/env python3
"""Source-level guard for runtime schema drift repair.

The persistence branch writes newer item columns from latency-sensitive save
paths. Older production databases may have only part of the migration history,
so boot-time schema repair and the handoff migration both need to cover the
same write surface.
"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="latin1")


def section(text: str, start: str, end: str) -> str:
    start_at = text.index(start)
    end_at = text.index(end, start_at)
    return text[start_at:end_at]


def assert_contains(text: str, needle: str, message: str) -> None:
    if needle not in text:
        raise AssertionError(message)


def assert_order(text: str, before: str, after: str, message: str) -> None:
    before_at = text.index(before)
    after_at = text.index(after, before_at)
    if before_at > after_at:
        raise AssertionError(message)


def main() -> int:
    sql = read("src/sql.c")
    migration = read("migrations/schema_migration_v16_schema_drift.sql")
    setup = read("migrations/run_this_one.sql")

    runtime = section(sql, "static bool sql_ensure_runtime_schema(MYSQL *db)\n{", "static void sql_reward_event_key")
    item_repair = section(
        sql,
        "static bool sql_schema_ensure_obj_uid_columns",
        "static bool sql_schema_ensure_corpse_tables",
    )

    assert_contains(
        runtime,
        "sql_schema_ensure_locker_tables(db)",
        "runtime schema repair should cover player/locker item drift",
    )
    assert_order(
        runtime,
        "sql_schema_ensure_corpse_tables(db)",
        "sql_schema_ensure_locker_tables(db)",
        "locker repair should run after corpse repair and before auction repair",
    )

    for marker in (
        '"player_items"',
        '"locker_items"',
        '"chest_id"',
        '"wear_flags"',
        '"item_type"',
        '"bitvector1"',
        '"bitvector2"',
        '"bitvector3"',
        '"bitvector4"',
        '"bitvector5"',
        '"obj_uid"',
        '"item_condition"',
        '"idx_obj_uid"',
        '"idx_locker_chest"',
        "private_chests",
        "private_chest_log",
    ):
        assert_contains(item_repair, marker, f"runtime item repair should include {marker}")

    for marker in (
        "CREATE PROCEDURE ensure_runtime_schema_drift",
        "ALTER TABLE player_items ADD COLUMN wear_flags",
        "ALTER TABLE player_items ADD COLUMN item_type",
        "ALTER TABLE locker_items ADD COLUMN chest_id",
        "ALTER TABLE locker_items ADD COLUMN wear_flags",
        "ALTER TABLE locker_items ADD COLUMN item_type",
        "ALTER TABLE locker_items ADD INDEX idx_locker_chest",
        "CREATE TABLE IF NOT EXISTS private_chests",
        "CREATE TABLE IF NOT EXISTS private_chest_log",
        "ALTER TABLE auctions ADD COLUMN obj_info_text",
        "ALTER TABLE epic_gain ADD COLUMN event_key",
        "ALTER TABLE world_quest_accomplished ADD COLUMN event_key",
        "ALTER TABLE zone_touches ADD COLUMN event_key",
    ):
        assert_contains(migration, marker, f"v16 migration should include {marker}")

    for marker in (
        "ALTER TABLE player_items ADD COLUMN wear_flags",
        "ALTER TABLE player_items ADD COLUMN item_type",
        "ALTER TABLE locker_items ADD COLUMN wear_flags",
        "ALTER TABLE locker_items ADD COLUMN item_type",
        "ALTER TABLE locker_items ADD COLUMN bitvector1",
        "ALTER TABLE corpse_items ADD COLUMN item_type",
    ):
        assert_contains(setup, marker, f"fresh setup should include {marker}")

    print("runtime schema drift source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
