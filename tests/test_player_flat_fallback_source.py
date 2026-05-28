#!/usr/bin/env python3
"""Source-level guard for player flat-file fallback saves.

SQL is the primary persistence path, but save failures must still leave a
recoverable player snapshot on disk and alert staff.  This test keeps that
fallback attached to the SQL failure path instead of becoming documentation
only.
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
    files = read("src/files.c")

    fallback = section(
        files,
        "static int persistence_write_character_flat_fallback",
        "void delete_knownShapes",
    )
    writer = section(files, "int writeCharacter", "#endif")

    for marker in (
        "ADD_BYTE(buf, (char)SAV_SAVEVERS)",
        "writeStatus(buf, ch",
        "writeSkills(buf, ch, MAX_SKILLS)",
        "writeWitness(buf, ch->specials.witnessed)",
        "writeAffects(buf, ch->affected)",
        "writeItems(buf, ch)",
        "fopen(Gbuf1, \"wb\")",
        "rename(Gbuf1, Gbuf2)",
        "rename(Gbuf2, Gbuf1)",
        "fallback_saved",
    ):
        assert_contains(
            fallback,
            marker,
            f"flat fallback should preserve binary pfile behavior: {marker}",
        )

    assert_contains(
        writer,
        "if (!sql_save_locker(ch, owner_pid, owner_assoc_id))",
        "locker SQL save failure path should still be present",
    )
    assert_contains(
        writer,
        "if (!sql_save_player(ch, type, room))",
        "player SQL save failure path should still be present",
    )
    assert_contains(
        writer,
        "persistence_write_character_flat_fallback(ch, type, room)",
        "SQL save failures should attempt a flat-file fallback snapshot",
    )
    assert_order(
        writer,
        "sql_save_player failed",
        "persistence_write_character_flat_fallback(ch, type, room)",
        "player flat fallback should run after SQL save failure is detected",
    )
    assert_contains(
        writer,
        "flat_fallback_failed",
        "fallback failures should alert staff explicitly",
    )

    print("player flat-file fallback source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
