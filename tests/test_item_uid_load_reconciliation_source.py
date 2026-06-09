#!/usr/bin/env python3
"""Source-level guard for UID current-owner reconciliation on load.

The anti-duplication rule is that a saved item row/blob may only materialize if
the current-owner table does not already say that UID belongs somewhere else.
Missing current-owner rows are treated as legacy/unknown so old saves still
load, but known mismatches must be skipped and logged.
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
    after_at = text.index(after)
    if before_at > after_at:
        raise AssertionError(message)


def main() -> int:
    sql_h = read("src/sql.h")
    sql = read("src/sql.c")
    sql_player = read("src/sql_player.c")
    auction = read("src/auction_houses.c")
    schema = read("src/duris.sql")

    helper = section(
        sql,
        "static bool sql_persistence_item_owner_matches_locked(",
        "static bool sql_persistence_write_scalar_event_line_locked",
    )
    player_load = section(sql_player, "bool sql_load_player_items", "bool sql_load_player_witnesses")
    locker_load = section(
        sql_player,
        "static P_obj sql_load_locker_items_filtered",
        "static P_obj sql_load_locker_items(",
    )
    private_chest = section(sql_player, "P_obj sql_load_private_chest_items", "// migration helpers")
    corpse_save = section(sql_player, "static int sql_save_corpse_item", "bool sql_save_corpse")
    corpse_load = section(sql_player, "bool sql_load_all_corpses", "extern struct shop_data")
    auction_pickup = section(auction, "bool auction_pickup", "bool auction_help")
    auction_finalize = section(auction, "bool finalize_auction", "bool insert_money_pickup")
    auction_schema = section(sql, "static bool sql_schema_ensure_auction_schema", "static bool sql_ensure_runtime_schema")

    assert_contains(
        sql_h,
        "sql_persistence_item_owner_matches",
        "owner reconciliation helper should be public to loaders",
    )
    assert_contains(
        helper,
        "FROM persistence_items_current WHERE item_uid=%llu LIMIT 1",
        "helper should consult current item ownership",
    )
    assert_contains(
        helper,
        "return TRUE",
        "missing SQL/current-owner state should allow legacy loads",
    )
    assert_contains(
        helper,
        "LOG_WIZ",
        "known stale-owner skips should alert staff",
    )

    assert_contains(
        player_load,
        'sql_persistence_item_owner_matches(saved_uid, "player", owner_ref, "sql_load_player_items")',
        "player item load should skip rows whose UID belongs elsewhere",
    )
    assert_order(
        player_load,
        "unsigned long saved_uid",
        "items[idx]         = obj;",
        "player load should check ownership before adding item to load map",
    )

    assert_contains(
        locker_load,
        'sql_persistence_item_owner_matches(obj->obj_uid, "locker", owner_ref, "sql_load_locker_items")',
        "locker load should skip rows whose UID belongs elsewhere",
    )
    assert_contains(
        private_chest,
        'sql_persistence_item_owner_matches(obj->obj_uid, "locker", owner_ref, "sql_load_private_chest_items")',
        "private chest load should use locker ownership checks",
    )

    assert_contains(
        corpse_save,
        'persistence_record_item_event("owner_corpse"',
        "corpse save should record corpse ownership for contained item UIDs",
    )
    assert_contains(
        corpse_load,
        "ci.obj_uid, ci.item_condition",
        "corpse loader should select saved UIDs and condition",
    )
    assert_contains(
        corpse_load,
        'sql_persistence_item_owner_matches(saved_uid, "corpse", owner_ref, "sql_load_all_corpses")',
        "corpse restore should skip already-looted or moved item UIDs",
    )
    assert_contains(
        corpse_load,
        "skipped_item_id",
        "corpse restore should ignore additional affect rows for skipped stale items",
    )

    assert_contains(
        auction_schema,
        "source_auction_id",
        "runtime schema should add source auction id to pickup rows",
    )
    assert_contains(
        schema,
        "`source_auction_id` int(10) unsigned default NULL",
        "base schema should include source auction id",
    )
    assert_contains(
        auction_finalize,
        "source_auction_id",
        "auction close should preserve the source auction id on pickup rows",
    )
    assert_contains(
        auction_pickup,
        'sql_persistence_item_owner_matches(tmp_obj->obj_uid, "auction", owner_ref, "auction_pickup")',
        "auction pickup should refuse UID blobs no longer owned by that auction",
    )
    assert_order(
        auction_pickup,
        "sql_persistence_item_owner_matches",
        "UPDATE auction_item_pickups SET retrieved = 1",
        "auction pickup should check ownership before marking a row retrieved",
    )

    print("item UID load reconciliation source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
