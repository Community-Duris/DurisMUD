from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
limits = (ROOT / "src" / "limits.c").read_text(encoding="utf-8", errors="replace")
new_events = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")

assert "else if (GET_STAT(ch) > STAT_INCAP)" in limits
assert "if (IS_FIGHTING(ch) || IS_DESTROYING(ch))" in limits
assert "event->deferral_count >= NEVENT_MAX_DEFERRALS;" in new_events
assert "normal events cannot starve" in new_events
print("nevent regen/death and bounded-deferral contracts passed")