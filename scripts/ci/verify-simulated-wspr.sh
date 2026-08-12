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
expected_kinds = (
    ["configure", "configured", "execution_start"]
    + ["event"] * 162
    + ["complete", "cleanup"]
)
assert kinds == expected_kinds, kinds

symbols = [event for event in events if event["kind"] == "event"]
assert len(symbols) == 162, len(symbols)
assert [event["event_index"] for event in symbols] == list(range(162))
assert all(event["rf_on"] is True for event in symbols)

symbol_period_ns = 682_666_666
frame_duration_ns = 110_591_999_892
logical_times = [event["logical_ns"] for event in symbols]
assert logical_times == [index * symbol_period_ns for index in range(162)]
assert all(later > earlier for earlier, later in zip(logical_times, logical_times[1:]))

complete = next(event for event in events if event["kind"] == "complete")
cleanup = next(event for event in events if event["kind"] == "cleanup")
assert complete["logical_ns"] == frame_duration_ns
assert complete["logical_ns"] == 162 * symbol_period_ns
assert complete["rf_on"] is False
assert cleanup["rf_on"] is False
assert all(event["logical_ns"] == 0 for event in events[:3])
assert cleanup["logical_ns"] == 0

tones = sorted({event["frequency_hz"] for event in symbols})
assert len(tones) == 4, tones
assert all(math.isfinite(tone) and tone > 0 for tone in tones)

# The canonical 20 m WSPR contract is a 14,095,600 Hz dial frequency plus
# one 1,500 Hz audio offset. WSPR tones are centered on that RF frequency and
# separated by 12,000 / 8,192 Hz. The JSON writer emits decimal doubles, so a
# 1e-6 Hz absolute tolerance covers serialization rounding without masking an
# RF-planning error.
dial_frequency_hz = 14_095_600.0
audio_offset_hz = 1_500.0
rf_center_hz = 14_097_100.0
tone_spacing_hz = 12_000.0 / 8_192.0
frequency_tolerance_hz = 1e-6
assert dial_frequency_hz + audio_offset_hz == rf_center_hz
expected_tones = [
    rf_center_hz + (tone_index - 1.5) * tone_spacing_hz
    for tone_index in range(4)
]
assert all(
    math.isclose(actual, expected, rel_tol=0.0, abs_tol=frequency_tolerance_hz)
    for actual, expected in zip(tones, expected_tones)
), (tones, expected_tones)

# TYPE1 AA0NT / EM18 / 20 dBm from the pinned
# WSPR-Reference/test_vectors/wspr_golden_vectors.json.
expected_symbols = (
    "132000023020111000302321113000022230012300222012112033210003103022"
    "213010303230030032332021321030223220203223221310310213010221110002"
    "210302110200222310303320011002"
)
actual_symbols = "".join(
    str(min(range(4), key=lambda index: abs(expected_tones[index] - event["frequency_hz"])))
    for event in symbols
)
assert actual_symbols == expected_symbols, actual_symbols
assert all(
    math.isclose(
        event["frequency_hz"],
        expected_tones[int(expected_symbol)],
        rel_tol=0.0,
        abs_tol=frequency_tolerance_hz,
    )
    for event, expected_symbol in zip(symbols, expected_symbols)
)

print(
    "Complete simulated WSPR trace is deterministic: "
    f"{len(symbols)} events, {frame_duration_ns / 1e9:.9f} logical seconds."
)
PY
