#!/usr/bin/env python3
"""Source-level guard for auction SQL lag reductions.

Auction escrow state still needs synchronous SQL, but non-state audit writes
and expensive listing patterns should avoid adding avoidable command latency.
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


def assert_not_contains(text: str, needle: str, message: str) -> None:
    if needle in text:
        raise AssertionError(message)


def assert_order(text: str, before: str, after: str, message: str) -> None:
    before_at = text.index(before)
    after_at = text.index(after, before_at)
    if before_at > after_at:
        raise AssertionError(message)


def main() -> int:
    auction = read("src/auction_houses.c")
    sql = read("src/sql.c")
    schema = read("src/duris.sql")

    helper = section(
        auction,
        "static void auction_record_bid_history",
        "// backfill state",
    )
    auction_list = section(auction, "bool auction_list", "// syntax: auction info")
    auction_bid = section(auction, "bool auction_bid", "// syntax: auction pickup")
    money_pickup = section(auction, "bool insert_money_pickup", "string format_time")
    scalar_writer = section(
        sql,
        "static bool sql_persistence_write_scalar_event_line_locked",
        "bool sql_persistence_write_scalar_event_line",
    )
    auction_schema = section(
        sql,
        "static bool sql_schema_ensure_auction_schema",
        "static bool sql_ensure_runtime_schema",
    )

    for needle in (
        '"idx_pid_retrieved", "(pid,retrieved,id)"',
        '"idx_status_end_id", "(status,end_time,id)"',
        '"idx_seller_status_end", "(seller_name,status,end_time)"',
        '"idx_auction_date", "(auction_id,date)"',
    ):
        assert_contains(
            auction_schema,
            needle,
            f"runtime schema should ensure auction index {needle}",
        )

    for needle in (
        "KEY `idx_pid_retrieved` (`pid`,`retrieved`,`id`)",
        "KEY `idx_status_end_id` (`status`,`end_time`,`id`)",
        "KEY `idx_seller_status_end` (`seller_name`,`status`,`end_time`)",
        "KEY `idx_auction_date` (`auction_id`,`date`)",
    ):
        assert_contains(schema, needle, f"base schema should include {needle}")

    assert_contains(
        auction_list,
        "int list_limit = MAX(1, AUCTION_LIST_LIMIT);",
        "auction list should respect the configured list limit",
    )
    assert_contains(
        auction_list,
        "order by end_time asc limit %d",
        "auction list should use indexed end_time ordering with a limit",
    )
    assert_contains(
        auction_list,
        "Showing the first %d auctions",
        "auction list should tell players when output was capped",
    )

    assert_contains(
        helper,
        "PERSISTENCE_SCALAR_EVENT|ts=%ld|event=auction_bid_history",
        "bid history should be represented as a scalar persistence event",
    )
    assert_contains(
        helper,
        "persistence_scalar_event_queue_enqueue(line)",
        "bid history should enqueue when the scalar worker is running",
    )
    assert_contains(
        helper,
        '"queue_full_flat_fallback"',
        "bid history should use flat fallback when the queue is full",
    )
    assert_order(
        helper,
        "persistence_scalar_event_queue_enqueue(line)",
        "INSERT INTO auction_bid_history",
        "bid history should try async persistence before direct SQL fallback",
    )
    assert_contains(
        sql,
        '!str_cmp(event_type, "auction_bid_history")',
        "scalar worker should persist queued auction bid history",
    )

    assert_contains(
        auction_bid,
        "auction_record_bid_history(auction_id, ch, bid_value);",
        "auction bid path should use queued bid-history helper",
    )
    assert_not_contains(
        auction_bid,
        "INSERT INTO auction_bid_history",
        "auction bid path should not issue direct bid-history SQL inline",
    )

    assert_contains(
        money_pickup,
        "ON DUPLICATE KEY UPDATE money = money + VALUES(money)",
        "money pickup should use one upsert instead of select-then-write",
    )
    assert_not_contains(
        money_pickup,
        "SELECT pid FROM auction_money_pickups",
        "money pickup should not do a preflight select",
    )

    print("auction SQL lag source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
