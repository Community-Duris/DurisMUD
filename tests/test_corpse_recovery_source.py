from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read_source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8", errors="ignore")


def section(text: str, start: str, end: str) -> str:
    start_at = text.index(start)
    end_at = text.index(end, start_at)
    return text[start_at:end_at]


def assert_order(text: str, before: str, after: str) -> None:
    before_at = text.index(before)
    after_at = text.index(after)
    if before_at > after_at:
        raise AssertionError(f"expected {before!r} before {after!r}")


def test_restored_player_corpses_get_tripled_recovery_decay() -> None:
    files_c = read_source("src/files.c")

    assert "#define PERSISTENCE_CORPSE_RESTORE_TIMER_MULTIPLIER 3" in files_c
    assert "restore_corpse_timer_refreshed" in files_c
    assert 'persistence_record_item_event("owner_corpse_restored"' in files_c

    body = section(
        files_c,
        "void persistence_refresh_restored_corpse",
        "int writeItems(char *buf, P_char ch)",
    )

    assert "get_property(\"timer.decay.corpse.pc\", 120) * WAIT_MIN" in body
    assert "PERSISTENCE_CORPSE_RESTORE_TIMER_MULTIPLIER" in body
    assert_order(
        body,
        "affect_from_obj(corpse, TAG_OBJ_DECAY)",
        "set_obj_affected(corpse, restored_decay, TAG_OBJ_DECAY, 0)",
    )


def test_restore_corpses_refreshes_loaded_corpse_before_preserving_file() -> None:
    sql_player_c = read_source("src/sql_player.c")

    body = section(sql_player_c, "bool sql_load_all_corpses(void)", "skip_corpse_save = 0")
    assert body.count('persistence_refresh_restored_corpse(cur_corpse, "sql_load_all_corpses");') >= 2
    assert_order(
        body,
        "obj_to_room(cur_corpse, cur_room)",
        'persistence_refresh_restored_corpse(cur_corpse, "sql_load_all_corpses");',
    )


def test_corpse_looting_rewrites_corpse_file_after_money_or_item_transfer() -> None:
    actobj_c = read_source("src/actobj.c")

    body = section(actobj_c, "void get(P_char ch,", "int fight_in_room(P_char ch)")
    assert body.count("writeCorpse(corpse);") >= 2
    assert "obj_to_char(o_obj, ch);" in body
    assert "extract_obj(o_obj);" in body


def test_corpse_save_failures_alert_when_sql_write_fails() -> None:
    files_c = read_source("src/files.c")

    write_body = section(files_c, "void writeCorpse(P_obj corpse)", "int writeItems(char *buf, P_char ch)")
    assert "if (!sql_save_corpse(corpse))" in write_body
    assert '"sql_save_failed"' in write_body
    assert "persistence_alert(" in write_body


def main() -> int:
    test_restored_player_corpses_get_tripled_recovery_decay()
    test_restore_corpses_refreshes_loaded_corpse_before_preserving_file()
    test_corpse_looting_rewrites_corpse_file_after_money_or_item_transfer()
    test_corpse_save_failures_alert_when_sql_write_fails()
    print("corpse recovery source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
