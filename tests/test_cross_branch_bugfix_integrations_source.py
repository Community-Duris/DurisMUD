#!/usr/bin/env python3
"""Source-level guard for bugfix branches integrated into persistence work."""

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
    actobj = read("src/actobj.c")
    artifact = read("src/artifact.c")
    epic = read("src/epic.c")
    epic_h = read("src/epic.h")
    epic_skills = read("src/epic_skills.c")
    guild = read("src/guild.c")
    specializations = read("src/specializations.c")
    files = read("src/files.c")
    memorize = read("src/memorize.c")
    sql_player = read("src/sql_player.c")

    give = section(actobj, "void do_give(P_char ch, char *argument, int cmd)", "/*\n\t * added by DTS")
    assert_not_contains(
        give,
        "artifact_switch_check(ch, obj);",
        "giving an artifact should not immediately force a bind-switch check",
    )
    assert_contains(
        artifact,
        "preserving existing timer",
        "artifact feed-timer safety check should preserve existing bind timers",
    )
    assert_contains(
        artifact,
        "skipping immediate merge",
        "artifact bind timer future-skew path should skip forced immediate merge",
    )

    assert_contains(
        epic,
        "static void apply_pending_epic_zone_completions(void)",
        "epic zone alignment changes should be delayed before DB application",
    )
    assert_contains(
        epic,
        "get_property(\"epic.showCompleted.delaySecs\", (30 * 60))",
        "epic zone completion visibility should default to a 30 minute delay",
    )
    assert_contains(
        epic,
        "UPDATE zones SET alignment = %d WHERE number = %d AND alignment > %d",
        "epic zone max alignment clamp should be scoped to the touched zone",
    )
    assert_contains(
        epic_h,
        "bool applied;",
        "epic zone completion records should remember whether pending alignment was applied",
    )

    assert_contains(
        epic_skills,
        "validate_epic_skills_for_spec(P_char ch)",
        "spec-restricted epic skill validation should be present",
    )
    assert_contains(
        guild,
        "validate_epic_skills_for_spec(ch);",
        "guild skill updates should validate spec-restricted epic skills",
    )
    assert_contains(
        specializations,
        "validate_epic_skills_for_spec(ch);",
        "specialize/unspecialize flows should validate spec-restricted epic skills",
    )

    for name, source in (
        ("flat-file item load", files),
        ("SQL item load", sql_player),
    ):
        assert_contains(
            source,
            "auto-fixed corrupted spellbook",
            f"{name} should repair spellbooks with used pages but no spell data",
        )
    scribe_check = section(memorize, "int ScriberSillyChecks(P_char ch, int spl)", "if ((GetSpellPages")
    assert_contains(
        scribe_check,
        "o1->value[3] > 0 && !d",
        "spell scribing should detect used pages with no spell description",
    )
    assert_contains(
        scribe_check,
        "o1->value[3] = 0;",
        "spell scribing should reset corrupted used-page count before capacity checks",
    )

    print("cross-branch bugfix integration source checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
