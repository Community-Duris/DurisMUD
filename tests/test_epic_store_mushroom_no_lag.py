#!/usr/bin/env python3
"""Source-level guard for rapid epic-store and level-mushroom persistence.

The edge case is a high-level player rapidly buying gift mushrooms, or an alt
rapidly eating level mushrooms. Those paths must schedule coalesced checkpoint
work instead of forcing a full save or level SQL update on every command.
"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="latin1")


def assert_contains(text: str, needle: str, message: str) -> None:
    if needle not in text:
        raise AssertionError(message)


def assert_not_contains(text: str, needle: str, message: str) -> None:
    if needle in text:
        raise AssertionError(message)


def assert_order(text: str, before: str, after: str, message: str) -> None:
    before_at = text.index(before)
    after_at = text.index(after)
    if before_at > after_at:
        raise AssertionError(message)


def section(text: str, start: str, end: str) -> str:
    start_at = text.index(start)
    end_at = text.index(end, start_at)
    return text[start_at:end_at]


def main() -> int:
    tradeskill = read("src/tradeskill.c")
    actobj = read("src/actobj.c")
    limits = read("src/limits.c")
    actoth = read("src/actoth.c")

    epic_store = section(tradeskill, "int epic_store", "int learn_tradeskill")
    mushroom_eat = section(actobj, "if (oaffect == 1337)", "act(\"$n eats $p.\"")
    advance_level = section(limits, "void advance_level", "void lose_level")

    assert_contains(
        actoth,
        "void persistence_schedule_character_save",
        "coalesced character save scheduler must exist",
    )
    assert_contains(
        actoth,
        "void persistence_schedule_level_checkpoint",
        "coalesced level checkpoint scheduler must exist",
    )
    deferred_event = section(
        actoth,
        "static void event_deferred_character_save",
        "static void persistence_schedule_checkpoint",
    )
    assert_contains(
        deferred_event,
        "int pid = data ? *((int *)data) : 0;",
        "deferred save event should carry the pid separately from the character pointer",
    )
    assert_contains(
        deferred_event,
        "pending = *slot;",
        "deferred save event should copy the pending slot before clearing it",
    )
    assert_order(
        deferred_event,
        "memset(slot, 0, sizeof(*slot));",
        "if (!IS_ALIVE(ch))",
        "stale deferred save slots must be cleared even if the character is no longer alive",
    )
    assert_contains(
        deferred_event,
        "deferred_save_character_missing",
        "deferred save event should alert and clear the slot if the character pointer is gone",
    )
    assert_contains(
        actoth,
        "&slot->pid, sizeof(slot->pid)",
        "deferred save scheduling should copy the pid into event data",
    )
    assert_contains(
        epic_store,
        'persistence_schedule_character_save(pl, 1, 5, "epic_store_purchase")',
        "epic-store purchases should schedule a coalesced checkpoint",
    )
    assert_not_contains(
        epic_store,
        "do_save_silent(pl, 1)",
        "epic-store purchases must not force a full save on each buy",
    )
    assert_contains(
        mushroom_eat,
        'persistence_schedule_level_checkpoint(ch, 1, 5, "level_mushroom_eat")',
        "level mushroom eating should schedule a coalesced level checkpoint",
    )
    assert_not_contains(
        mushroom_eat,
        "do_save_silent(ch, 1)",
        "level mushroom eating must not force a full save on each eat",
    )
    assert_contains(
        advance_level,
        'persistence_schedule_level_checkpoint(ch, 1, 5, "advance_level")',
        "level gains should defer and coalesce level SQL/checkpoint work",
    )
    assert_not_contains(
        advance_level,
        "sql_update_level(ch)",
        "advance_level must not synchronously update SQL for every level",
    )

    print("epic store and level mushroom no-lag source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
