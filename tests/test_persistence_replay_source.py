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
        utility_h,
        "int persistence_replay_fallback_events(void);",
        "fallback replay should be declared in utility.h",
    )
    assert_contains(
        prototypes,
        "int persistence_replay_fallback_events(void);",
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
