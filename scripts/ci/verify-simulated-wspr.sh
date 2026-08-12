#!/bin/sh
set -eu

binary=./build/bin/wsprrypi_debug
trace=/tmp/wsprrypi-simulated-trace.json
first=/tmp/wsprrypi-wspr-trace-first.json
final=/tmp/wsprrypi-wspr-trace.json
syscall_trace=/tmp/wsprrypi-wspr-simulator.strace

run_wspr() {
    "$binary" \
        --backend simulated \
        --no-web \
        --no-offset \
        --no-system-clock-frequency-estimate \
        --terminate 1 \
        AA0NT EM18 20 20m
}

run_wspr
cp "$trace" "$first"

strace -f -e trace=openat,open \
    -o "$syscall_trace" \
    "$binary" \
        --backend simulated \
        --no-web \
        --no-offset \
        --no-system-clock-frequency-estimate \
        --terminate 1 \
        AA0NT EM18 20 20m
cp "$trace" "$final"

cmp "$first" "$final"

python3 - "$final" <<'PY'
import json
import math
import sys

trace_path = sys.argv[1]
with open(trace_path, encoding="utf-8") as trace_file:
    trace = json.load(trace_file)

assert trace["schema_version"] == 1
assert trace["backend"] == "simulated"
assert trace["mode"] == "WSPR"
assert trace["plan_id"] == 1
assert trace["request_id"] == 1

events = trace["events"]
kinds = [event["kind"] for event in events]
for required in ("configure", "configured", "execution_start", "complete", "cleanup"):
    assert kinds.count(required) == 1, (required, kinds.count(required))
for forbidden in ("cancelled", "configure_failure", "execution_failure", "cleanup_failure"):
    assert forbidden not in kinds, forbidden

symbols = [event for event in events if event["kind"] == "event"]
assert len(symbols) == 162, len(symbols)
assert [event["event_index"] for event in symbols] == list(range(162))
assert all(event["rf_on"] is True for event in symbols)

logical_times = [event["logical_ns"] for event in symbols]
assert logical_times == sorted(logical_times)
symbol_periods = [
    later - earlier for earlier, later in zip(logical_times, logical_times[1:])
]
assert len(set(symbol_periods)) == 1, set(symbol_periods)
symbol_period_ns = symbol_periods[0]
assert symbol_period_ns > 0

complete = next(event for event in events if event["kind"] == "complete")
cleanup = next(event for event in events if event["kind"] == "cleanup")
assert complete["logical_ns"] == logical_times[-1] + symbol_period_ns
assert 110_000_000_000 < complete["logical_ns"] < 111_000_000_000
assert complete["rf_on"] is False
assert cleanup["rf_on"] is False

tones = sorted({event["frequency_hz"] for event in symbols})
assert len(tones) == 4, tones
assert all(math.isfinite(tone) and tone > 0 for tone in tones)
spacings = [later - earlier for earlier, later in zip(tones, tones[1:])]
assert max(spacings) - min(spacings) < 1e-6, spacings
assert all(1.4 < spacing < 1.5 for spacing in spacings), spacings

# TYPE1 AA0NT / EM18 / 20 dBm from the pinned
# WSPR-Reference/test_vectors/wspr_golden_vectors.json.
expected_symbols = (
    "132000023020111000302321113000022230012300222012112033210003103022"
    "213010303230030032332021321030223220203223221310310213010221110002"
    "210302110200222310303320011002"
)
actual_symbols = "".join(
    str(min(range(4), key=lambda index: abs(tones[index] - event["frequency_hz"])))
    for event in symbols
)
assert actual_symbols == expected_symbols, actual_symbols

print(
    "Complete simulated WSPR trace is deterministic: "
    f"{len(symbols)} events, {complete['logical_ns'] / 1e9:.9f} logical seconds."
)
PY
