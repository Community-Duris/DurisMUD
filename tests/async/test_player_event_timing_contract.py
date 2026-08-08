from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")
structs = (ROOT / "src" / "structs.h").read_text(encoding="utf-8", errors="replace")

assert "nevent_is_player_timed" in source
assert "NEVENT_PRIORITY_PLAYER" in source
assert "scheduled_tick" in structs
assert "event->priority" in source
assert "PLAYER EVENT TIMING:" in source
assert "nevent_link_schedule" in source
assert "last_player" in source
assert "deferral_count" in structs
assert "NEVENT_MAX_DEFERRALS      0U" in source
assert "nevent_promote_overdue_event" in source
assert "event_wait" in source
assert "func == event_wait" in source
print("player event timing priority contract passed")
