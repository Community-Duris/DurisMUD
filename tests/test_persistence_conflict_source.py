#!/usr/bin/env python3
"""Source-level guard for item ownership conflict auditing.

The anti-duplication rule is "one current owner per item UID."  When a replay
or stale fallback says the same UID exists somewhere else, SQL should keep one
winner and also leave a conflict audit record for staff follow-up.
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
    sql = read("src/sql.c")
    sql = sql[sql.index("#else"):]
    utility = read("src/utility.c")

    ensure = section(sql, "static bool sql_persistence_ensure_tables", "static bool sql_persistence_ensure_reward_tables")
    writer = section(
        sql,
        "\nstatic bool sql_persistence_write_item_event_line_locked(",
        "\nbool sql_persistence_write_item_event_line(",
    )
    recorder = section(
        utility,
        "void persistence_record_item_event",
        "void debug(const char *format, ...)",
    )

    assert_contains(
        ensure,
        "CREATE TABLE IF NOT EXISTS persistence_item_conflicts",
        "ownership conflict audit table should be created",
    )
    assert_contains(
        ensure,
        "resolution_lookup",
        "conflicts should be searchable by resolution",
    )
    assert_contains(
        writer,
        "INSERT INTO persistence_item_conflicts",
        "item writer should record owner conflicts",
    )
    assert_contains(
        writer,
        "owner_type <> '%s' OR owner_ref <> '%s'",
        "conflicts should only record when owner differs",
    )
    assert_contains(
        writer,
        "newest_incoming_wins",
        "conflicts should record when incoming owner wins",
    )
    assert_contains(
        writer,
        "existing_newer_wins",
        "conflicts should record when existing owner remains current",
    )
    assert_contains(
        writer,
        "LOG_WIZ",
        "owner conflicts should alert staff through wiz logging",
    )
    assert_order(
        writer,
        "INSERT INTO persistence_item_conflicts",
        "INSERT INTO persistence_items_current",
        "conflict should be recorded before current owner is changed",
    )
    assert_contains(
        utility,
        "static unsigned long long persistence_event_time_usec(void)",
        "item ownership events should use high-resolution event times",
    )
    assert_contains(
        recorder,
        "persistence_event_time_usec()",
        "item event records should avoid same-second ownership ordering ties",
    )

    print("persistence conflict source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
