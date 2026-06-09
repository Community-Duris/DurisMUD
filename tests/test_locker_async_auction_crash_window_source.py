#!/usr/bin/env python3
"""Source-level guard for locker async saves and auction crash windows.

Locker periodic saves can reuse the existing forked child-save pattern because
the locker contents are copied into the synthetic locker character before the
fork. Auction offer/pickup still need synchronous player saves until load-time
UID ownership reconciliation can prevent duplicated listed items or lost pickup
items after a crash.
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
    lockers = read("src/storage_lockers.c")
    auction = read("src/auction_houses.c")

    save_locker = section(lockers, "static int save_locker_char", "void StorageLocker::LockerToPFile")
    offer = section(auction, "bool auction_offer", "// syntax: auction list")
    pickup = section(auction, "bool auction_pickup", "bool auction_help")

    assert_order(
        save_locker,
        "pLocker->LockerToPFile();",
        "pid = fork();",
        "locker contents should be copied to the synthetic locker before forking a save child",
    )
    assert_contains(
        save_locker,
        "if (pid == 0)",
        "locker saves should run writeCharacter in a child process",
    )
    assert_contains(
        save_locker,
        "sql_create_child_connection()",
        "locker save child should use an isolated MySQL connection",
    )
    assert_contains(
        save_locker,
        "sql_reset_for_child(child_conn)",
        "locker save child should reset DB globals to the child connection",
    )
    assert_contains(
        save_locker,
        "writeCharacter(chLocker, bTerminal ? 3 : 0, NOWHERE)",
        "terminal and non-terminal locker saves should share the async child save path",
    )
    assert_contains(
        save_locker,
        "else if (pid < 0)",
        "locker saves should keep a synchronous fallback if fork fails",
    )
    assert_order(
        save_locker,
        "else if (pid < 0)",
        "writeCharacter(chLocker, bTerminal ? 3 : 0, NOWHERE)",
        "synchronous locker save should only be used after fork failure",
    )
    assert_contains(
        save_locker,
        "async_save_fork_failed",
        "fork fallback should alert staff before a synchronous locker save",
    )
    assert_contains(
        save_locker,
        "add_event(StorageLocker::event_resortLocker",
        "non-terminal locker saves should still resort the in-memory room after forking",
    )

    assert_order(
        offer,
        "persistence_record_item_event(\"owner_auction\"",
        "extract_obj(tmp_obj, FALSE)",
        "auction offer must record ownership before removing the in-memory item",
    )
    assert_order(
        offer,
        "extract_obj(tmp_obj, FALSE)",
        "writeCharacter(ch, 1, ch->in_room)",
        "auction offer should keep a crash-consistency save after item removal",
    )
    assert_order(
        pickup,
        "UPDATE auction_item_pickups SET retrieved = 1",
        "obj_to_char(tmp_obj, ch)",
        "auction pickup should mark the pickup row before handing the item to the player",
    )
    assert_order(
        pickup,
        "obj_to_char(tmp_obj, ch)",
        "writeCharacter(ch, 1, ch->in_room)",
        "auction pickup should keep a crash-consistency save after handing items to the player",
    )

    print("locker async and auction crash-window source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
