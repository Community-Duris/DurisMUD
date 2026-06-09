#!/usr/bin/env python3
"""Source-level guard for artifact SQL lag reductions.

Artifacts still require careful synchronous reads in a few ownership checks, but
plain state writes should avoid SELECT-before-UPDATE patterns when the schema
already has primary keys that support atomic upserts.
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


def main() -> int:
    artifact = read("src/artifact.c")
    sql = read("src/sql.c")
    sql = sql[sql.index("#else"):]

    object_update = section(
        artifact,
        "void artifact_update_sql(P_obj arti, char owned, time_t timer)",
        "// This function just updates/creates a new entry",
    )
    vnum_update = section(
        artifact,
        "void artifact_update_sql(int vnum, bool owned, int locType, int location, time_t timer, int type)",
        "// Remove the artifact data from the DB.",
    )
    bind_update = section(
        sql,
        "void sql_update_bind_data(int vnum, int *owner_pid, int *timer)",
        "bool sql_clear_zone_trophy()",
    )
    bind_read = section(
        sql,
        "void sql_get_bind_data(int vnum, int *owner_pid, int *timer)",
        "void sql_update_bind_data",
    )

    for name, block in (
        ("object artifact update", object_update),
        ("vnum artifact update", vnum_update),
        ("artifact bind update", bind_update),
    ):
        assert_contains(
            block,
            "ON DUPLICATE KEY UPDATE",
            f"{name} should write with an atomic upsert",
        )
        assert_not_contains(
            block,
            "SELECT owned, location",
            f"{name} should not preflight the artifact row before writing",
        )

    assert_contains(
        object_update,
        "owned=CASE WHEN '%c'='Y' THEN 'Y' WHEN '%c'='N' THEN 'N' ELSE owned END",
        "object artifact update should preserve existing owned flag unless caller explicitly changes it",
    )
    assert_contains(
        object_update,
        "location=CASE WHEN VALUES(locType)=%d THEN location ELSE VALUES(location) END",
        "object artifact update should preserve corpse owner location for existing corpse rows",
    )
    assert_not_contains(
        bind_update,
        "mysql_store_result",
        "artifact bind update should not read before writing",
    )
    assert_contains(
        bind_read,
        "SELECT owner_pid, timer FROM artifact_bind WHERE vnum = %d",
        "artifact bind reads should fetch only the needed columns",
    )

    print("artifact SQL lag source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
