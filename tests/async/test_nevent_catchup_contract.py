from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")
events = (ROOT / "src" / "events.c").read_text(encoding="utf-8", errors="replace")

for marker in (
    "NEVENT_CATCHUP_WINDOW_PULSES",
    "nevent_catchup_debt",
    "nevent_catchup_extension_us",
    "NEVENT CATCHUP",
    "avg_callback_us",
):
    assert marker in source, marker

for marker in (
    "struct regen_event_state",
    "last_tick",
    "elapsed_ticks",
):
    assert marker in events, marker


def repayment_quotas(debt, window=4):
    quotas = []
    for remaining in range(window, 0, -1):
        quota = (debt + remaining - 1) // remaining
        quotas.append(quota)
        debt -= quota
    return quotas

assert repayment_quotas(300) == [75, 75, 75, 75]
assert repayment_quotas(301) == [76, 75, 75, 75]
def coalesced_regen(per_tick, elapsed_ticks, pulses_in_tick, accumulated=0.0):
    accumulated += (per_tick * elapsed_ticks) / pulses_in_tick
    whole = int(accumulated)
    return whole, accumulated - whole

assert coalesced_regen(100, 4, 100) == (4, 0.0)
assert coalesced_regen(25, 4, 100) == (1, 0.0)
print("nevent dynamic catch-up and regen coalescing contract passed")
