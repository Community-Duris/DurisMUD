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


def assert_order(text: str, before: str, after: str, message: str) -> None:
    before_at = text.index(before)
    after_at = text.index(after, before_at)
    if before_at > after_at:
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
    exists_read = section(
        artifact,
        "static bool artifact_data_exists_sql(int vnum, P_arti adata)",
        "// Returns TRUE iff arti vnum has timer ticking already.",
    )
    data_read = section(
        artifact,
        "bool get_artifact_data_sql(int vnum, P_arti adata)",
        "void artifact_feed_sql",
    )
    feed_min = section(
        artifact,
        "void artifact_feed_to_min_sql(P_obj arti, int min_minutes)",
        "void artifact_switch_check",
    )
    location_update = section(
        artifact,
        "void artifact_update_location_sql(P_obj arti)",
        "// Returns TRUE iff arti vnum has timer ticking already.",
    )
    remove_owned = section(
        artifact,
        "bool remove_owned_artifact_sql(P_obj arti, int pid)",
        "// This is used for when a character is deleted.",
    )
    poof_check = section(
        artifact,
        "void event_artifact_check_poof_sql",
        "// Looks through list, and adds entry to the end of list.",
    )
    syncdb = section(
        artifact,
        "void arti_syncdb_sql(P_char ch)",
        "send_to_char_f(ch, \"Cleared %d, updated %d artifact ownerships",
    )

    for needle in (
        "static P_arti artifact_state_cache = NULL;",
        "static bool artifact_state_cache_get",
        "static void artifact_state_cache_store_values",
        "static void artifact_state_cache_forget",
        "static void artifact_state_cache_clear",
        "static bool artifact_data_exists_sql",
    ):
        assert_contains(
            artifact,
            needle,
            f"artifact local state cache should include {needle}",
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
    for name, block in (
        ("object artifact update", object_update),
        ("vnum artifact update", vnum_update),
        ("artifact feed missing-row insert", section(artifact, "if (!get_artifact_data_sql(vnum, &artidata))", "// If we're tyring to feed over the limit.")),
    ):
        assert_contains(
            block,
            "artifact_state_cache_store_values",
            f"{name} should refresh local artifact state after successful SQL writes",
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
    assert_contains(
        exists_read,
        "artifact_state_cache_get(vnum, adata)",
        "artifact existence reads should consult the local state cache before querying SQL",
    )
    assert_order(
        exists_read,
        "artifact_state_cache_get(vnum, adata)",
        "SELECT owned, locType, location",
        "artifact data cache should be checked before the SQL SELECT",
    )
    assert_contains(
        exists_read,
        "artifact_state_cache_store(&fetched);",
        "artifact existence reads should populate the local state cache after SQL fetch",
    )
    assert_contains(
        data_read,
        "artifact_data_exists_sql(vnum, &artidata)",
        "legacy artifact data reads should share the cached existence helper",
    )
    assert_contains(
        location_update,
        "get_artifact_data_sql(OBJ_VNUM(arti), &artidata);",
        "artifact movement updates should read state through the cached helper",
    )
    assert_contains(
        remove_owned,
        "ON DUPLICATE KEY UPDATE owned='Y'",
        "artifact corpse removal path should upsert corpse ownership without a preflight SELECT",
    )
    assert_not_contains(
        remove_owned,
        "SELECT owned, UNIX_TIMESTAMP(timer)",
        "artifact corpse removal path should not preflight the artifact row",
    )
    for name, block in (
        ("poof sweep", poof_check),
        ("syncdb command", syncdb),
    ):
        assert_contains(
            block,
            "artifact_state_cache_clear();",
            f"{name} should clear local artifact state after broad table mutation",
        )
    assert_contains(
        feed_min,
        "artifact_data_exists_sql(vnum, &artidata)",
        "artifact feed-to-min should use the cached artifact existence helper",
    )
    assert_not_contains(
        feed_min,
        "select owned, UNIX_TIMESTAMP(timer) from artifacts",
        "artifact feed-to-min should not issue its own artifact timer SELECT",
    )
    assert_contains(
        feed_min,
        "location = world[location].number;",
        "artifact feed-to-min should persist room vnums, not room indexes",
    )

    print("artifact SQL lag source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
