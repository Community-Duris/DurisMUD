#!/usr/bin/env python3
"""Source-level guard for queue-full persistence fallback behavior.

Persistence queues are intentionally bounded so command processing never waits
behind slow SQL. When a queue is full, item/reward intent still needs a durable
flat fallback before any direct SQL last resort can happen.
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


def assert_not_between(text: str, start: str, end: str, needle: str, message: str) -> None:
    start_at = text.index(start)
    end_at = text.index(end, start_at)
    if needle in text[start_at:end_at]:
        raise AssertionError(message)


def main() -> int:
    utility = read("src/utility.c")
    utility_h = read("src/utility.h")
    prototypes = read("src/prototypes.h")
    sql = read("src/sql.c")
    sql = sql[sql.index("#else"):]

    fallback = section(
        utility,
        "int persistence_write_fallback_event_line",
        "static const char *persistence_clean_field",
    )
    item_record = section(
        utility,
        "void persistence_record_item_event",
        "void debug(",
    )

    assert_contains(
        utility_h,
        "int persistence_write_fallback_event_line",
        "fallback append helper should be declared for persistence call sites",
    )
    assert_contains(
        prototypes,
        "int                   persistence_write_fallback_event_line",
        "fallback append helper should be visible through prototypes.h",
    )
    assert_order(
        fallback,
        "pthread_mutex_lock(&persistence_fallback_log_mutex);",
        "fopen(LOG_EVENT, \"a\")",
        "flat fallback writes should be serialized before opening the log",
    )
    assert_order(
        fallback,
        "fclose(log_f)",
        "pthread_mutex_unlock(&persistence_fallback_log_mutex);",
        "flat fallback writes should unlock only after close",
    )
    assert_order(
        item_record,
        "persistence_item_event_queue_enqueue(line)",
        "persistence_write_fallback_event_line(line,",
        "item ownership events should use flat fallback when enqueue fails",
    )
    assert_contains(
        item_record,
        "\"queue_full_flat_fallback\"",
        "item ownership fallback should identify queue-full flat fallback",
    )

    for name, start, end in (
        ("level checkpoint", "void sql_update_level", "/* Update money info */"),
        ("money checkpoint", "void sql_update_money", "/* Update playtime info */"),
        ("playtime checkpoint", "void sql_update_playtime", "/* Update player's epics"),
        ("epics checkpoint", "void sql_update_epics", "void manual_log"),
        ("world quest completion", "void sql_world_quest_finished", "int sql_world_quest_can_do_another"),
        ("zone touch", "void sql_zone_touch_finished", "const char *sql_select_IP_info"),
        ("epic gain", "void log_epic_gain_event", "/* The prepstatement_duris_sql table"),
    ):
        block = section(sql, start, end)
        assert_order(
            block,
            "persistence_scalar_event_queue_enqueue(line)",
            "persistence_write_fallback_event_line(line,",
            f"{name} should write flat fallback before any direct SQL fallback",
        )
        assert_contains(
            block,
            "\"queue_full_flat_fallback\"",
            f"{name} should label queue-full flat fallback",
        )

    for name, start, end in (
        ("world quest completion", "void sql_world_quest_finished", "int sql_world_quest_can_do_another"),
        ("zone touch", "void sql_zone_touch_finished", "const char *sql_select_IP_info"),
        ("epic gain", "void log_epic_gain_event", "/* The prepstatement_duris_sql table"),
    ):
        block = section(sql, start, end)
        assert_order(
            block,
            "persistence_write_fallback_event_line(line,",
            "sql_persistence_connection()",
            f"{name} should try flat fallback before direct persistence SQL",
        )

    for name, start, end in (
        ("level checkpoint", "void sql_update_level", "/* Update money info */"),
        ("money checkpoint", "void sql_update_money", "/* Update playtime info */"),
        ("playtime checkpoint", "void sql_update_playtime", "/* Update player's epics"),
        ("epics checkpoint", "void sql_update_epics", "void manual_log"),
    ):
        block = section(sql, start, end)
        assert_not_between(
            block,
            "persistence_scalar_event_worker_running()",
            "//",
            "sql_persistence_connection()",
            f"{name} should not use direct SQL for queue-full checkpoint fallback",
        )

    print("queue-full persistence fallback source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
