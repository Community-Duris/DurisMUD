#!/usr/bin/env python3
"""Source-level guard for fallback persistence replay.

Fallback logs are allowed to lag behind SQL, but they must not become a dead
end. Boot replay should repair SQL before gameplay starts, keep failed records
for later retry, and leave the hot command path as enqueue-only.
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


def assert_last_order(text: str, before: str, after: str, message: str) -> None:
    before_at = text.rindex(before)
    after_at = text.rindex(after)
    if before_at > after_at:
        raise AssertionError(message)


def main() -> int:
    utility = read("src/utility.c")
    comm = read("src/comm.c")
    utility_h = read("src/utility.h")
    prototypes = read("src/prototypes.h")

    replay = section(
        utility,
        "int persistence_replay_fallback_events(void)",
        "static int persistence_item_event_log_writer",
    )
    item_writer = section(
        utility,
        "static int persistence_item_event_log_writer",
        "int persistence_start_item_event_worker",
    )
    scalar_writer = section(
        utility,
        "static int persistence_scalar_event_log_writer",
        "int persistence_start_scalar_event_worker",
    )
    item_flush = section(
        utility,
        "int persistence_flush_item_events",
        "int persistence_flush_scalar_events",
    )
    scalar_flush = section(
        utility,
        "int persistence_flush_scalar_events",
        "static int persistence_line_has_prefix",
    )
    run_loop = section(comm, "fprintf(stderr, \"Entering game loop", "persistence_stop_scalar_event_worker();")

    assert_contains(
        replay,
        "sql_persistence_write_item_event_line(event_line)",
        "item fallback records should replay through the SQL item writer",
    )
    assert_contains(
        replay,
        "sql_persistence_write_scalar_event_line(event_line)",
        "scalar fallback records should replay through the SQL scalar writer",
    )
    assert_contains(
        replay,
        "failed++",
        "failed fallback records should be counted and retained for retry",
    )
    assert_contains(
        replay,
        "rename(tmp_path, LOG_EVENT)",
        "fallback log should be rewritten after successful replay attempts",
    )
    assert_contains(
        replay,
        "persistence_alert(",
        "fallback replay should alert staff about replay outcomes and failures",
    )
    assert_contains(
        utility,
        "static pthread_mutex_t persistence_fallback_log_mutex",
        "worker fallback log appends should be guarded by a shared mutex",
    )
    for name, block in (("item", item_writer), ("scalar", scalar_writer)):
        assert_order(
            block,
            "pthread_mutex_lock(&persistence_fallback_log_mutex);",
            "fopen(LOG_EVENT, \"a\")",
            f"{name} fallback writer should lock before opening the fallback log",
        )
        assert_last_order(
            block,
            "fclose(log_f)",
            "pthread_mutex_unlock(&persistence_fallback_log_mutex);",
            f"{name} fallback writer should unlock after closing the fallback log",
        )
    assert_contains(
        item_flush,
        "persistence_item_event_worker_running()",
        "item fallback flush should not drain the queue while the worker is active",
    )
    assert_contains(
        scalar_flush,
        "persistence_scalar_event_worker_running()",
        "scalar fallback flush should not drain the queue while the worker is active",
    )
    assert_contains(
        utility_h,
        "int persistence_replay_fallback_events(void);",
        "fallback replay should be declared in utility.h",
    )
    assert_contains(
        prototypes,
        "persistence_replay_fallback_events(void);",
        "fallback replay should be declared in prototypes.h",
    )
    assert_order(
        run_loop,
        "persistence_replay_fallback_events();",
        "persistence_start_item_event_worker();",
        "fallback replay should run before the item worker starts",
    )
    assert_order(
        run_loop,
        "persistence_replay_fallback_events();",
        "game_loop(port, sslport);",
        "fallback replay should run before gameplay begins",
    )

    print("persistence fallback replay source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
