#!/usr/bin/env python3
"""Source-level guard for async reward ledger persistence.

Epic gains, world quest completions, and zone touches are gameplay-visible
reward events. They should enqueue scalar persistence jobs when the worker is
running instead of issuing SQL directly from the command path.
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

    quest = section(sql, "void sql_world_quest_finished", "int sql_world_quest_can_do_another")
    zone = section(sql, "void sql_zone_touch_finished", "const char *sql_select_IP_info")
    epic = section(sql, "void log_epic_gain_event", "/* The prepstatement_duris_sql table")
    shop = section(sql, "int sql_shop_sell", "int sql_shop_trophy")
    quest_trophy = section(sql, "int sql_quest_finish", "int sql_quest_trophy")
    writer = section(sql, "bool sql_persistence_write_scalar_event_line", "void send_to_pid_offline")

    assert_contains(
        sql,
        "static pthread_mutex_t persistence_sql_mutex",
        "background persistence writers should serialize access to the shared MySQL handle",
    )
    for writer_name in (
        "sql_persistence_write_item_event_line",
        "sql_persistence_write_scalar_event_line",
    ):
        wrapper = section(sql, f"\nbool {writer_name}(", "\n}")
        assert_contains(
            wrapper,
            "pthread_mutex_lock(&persistence_sql_mutex);",
            f"{writer_name} should lock the shared persistence SQL connection",
        )
        assert_contains(
            wrapper,
            "pthread_mutex_unlock(&persistence_sql_mutex);",
            f"{writer_name} should unlock the shared persistence SQL connection",
        )

    for name, block in (
        ("world quest completion", quest),
        ("zone touch", zone),
        ("epic gain", epic),
        ("shop trophy sale", shop),
        ("quest trophy finish", quest_trophy),
    ):
        assert_contains(
            block,
            "persistence_scalar_event_worker_running()",
            f"{name} should check for the scalar worker",
        )
        assert_contains(
            block,
            "persistence_scalar_event_queue_enqueue(line)",
            f"{name} should enqueue a scalar persistence event",
        )
        assert_order(
            block,
            "persistence_scalar_event_queue_enqueue(line)",
            "INSERT INTO",
            f"{name} should enqueue before falling back to direct SQL",
        )

    for event_name in (
        "epic_gain",
        "world_quest_finished",
        "zone_touch",
        "shop_trophy_sell",
        "quest_trophy_finish",
    ):
        assert_contains(
            writer,
            f'event_type, "{event_name}"',
            f"scalar worker should persist {event_name} events",
        )

    print("reward async source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
