#!/usr/bin/env python3
"""Source-level guard for locker item ownership events.

Storage lockers use temporary rooms, chest objects, and synthetic locker
characters. Persistence recovery should treat the locker itself as the durable
owner so those temporary representations do not create item-loss or duplication
ambiguity.
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
    lockers = read("src/storage_lockers.c")
    sql = read("src/sql.c")

    helper = section(
        lockers,
        "static void persistence_record_locker_item_event",
        "inline StorageLocker *GetChestList",
    )
    to_pfile = section(lockers, "void StorageLocker::LockerToPFile", "void StorageLocker::PFileToLocker")
    to_locker = section(lockers, "void StorageLocker::PFileToLocker", "static void check_for_artisInRoom")
    owner_parser = section(sql, "static void persistence_parse_owner", "bool sql_persistence_write_item_event_line")

    assert_contains(
        helper,
        '"locker:%s"',
        "locker ownership events should use locker:<name> owner refs",
    )
    assert_contains(
        helper,
        '"owner_locker"',
        "locker ownership events should use a distinct event type",
    )
    assert_contains(
        to_pfile,
        "persistence_record_locker_item_event(innerObj",
        "items moved from chest to locker pfile should retain locker ownership",
    )
    assert_contains(
        to_pfile,
        "persistence_record_locker_item_event(tmp_object",
        "loose locker-room items moved to pfile should retain locker ownership",
    )
    assert_contains(
        to_locker,
        "persistence_record_locker_item_event(tmp_object",
        "items loaded from locker pfile into room/chests should retain locker ownership",
    )
    assert_order(
        to_locker,
        "PutInProperChest(tmp_object)",
        "persistence_record_locker_item_event(tmp_object",
        "locker ownership should be recorded after temporary room/chest placement",
    )
    assert_contains(
        owner_parser,
        "colon = strchr(target, ':');",
        "SQL owner parser should accept locker:<name> owner refs",
    )

    print("locker persistence source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
