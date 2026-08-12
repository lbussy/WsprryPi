# RP1 GPCLK Phase 6 clock-only transition probe

## Disposition

**Failed the clock-only timing gate. No GPIO output was enabled.**

The planner and generation-safe transition lifecycle passed their source-level
tests, and the RP1 common-clock driver safely selected every endpoint needed by
the four finite dither profiles while GPCLK0 remained disabled. The tested
general-workqueue/common-clock mechanism cannot provide the required update
cadence or deterministic timing, however. Under four-core CPU load, lateness
in a deliberately slow 20 ms transition sequence accumulated to 14.802 ms by
the fourth event.

Phase 6 therefore stops at the clock-only boundary. This result does not
authorize or support live divider transitions, WSPR symbols, or a production
RP1 backend.

## Transition contract

At 14.0971 MHz from the RP1 50 MHz `xosc`, the four WSPR tones cannot be
represented by four distinct nearest 16.16 divider words. The hardware-
independent contract consequently transitions among four finite two-word
dither profiles supplied by `Rp1GpclkPlan`, not among four single divider
words.

For the Phase 6 zero-PPM numerical fixture, the profiles were:

| Tone | Requested Hz | Lower word/count | Upper word/count | Average error |
|---:|---:|---:|---:|---:|
| 0 | 14,097,097.802734 | 232445 / 65065 | 232446 / 471 | -0.000303 Hz |
| 1 | 14,097,099.267578 | 232444 / 1112 | 232445 / 64424 | -0.000236 Hz |
| 2 | 14,097,100.732422 | 232444 / 2695 | 232445 / 62841 | -0.000165 Hz |
| 3 | 14,097,102.197266 | 232444 / 4278 | 232445 / 61258 | -0.000094 Hz |

All lower and upper words retain integer divider 3, so the plan requires no
live integer-divider change. Tone identity resides primarily in the relative
counts of the adjacent fractional words.

The new hardware-independent sequencer accepts a monotonic event schedule and
the exact planner tone profiles. Each active run has a generation number.
Cancellation, cutoff, transition failure, explicit stop, or destruction
invalidates the generation and calls the adapter's fail-closed operation
exactly once. Stale callbacks are rejected before reaching the adapter.

## Source changes and tests

WSPR-Transmitter adds:

- `src/rp1_gpclk_transition.hpp`;
- `src/rp1_gpclk_transition.cpp`; and
- `src/rp1_gpclk_transition_test.cpp`.

The parent `src/Makefile` adds `rp1-gpclk-transition-test` and excludes all
three RP1 test programs from production and integration object lists. The
exclusion fixes multiple-`main` linkage discovered by the parent WSPR tone
regression.

The transition test covers:

- all four ordered tone profiles and their exact words and counts;
- repeated tone selection;
- monotonic scheduling and catch-up of due events;
- cancellation before the first transition and during a sequence;
- stale-generation rejection;
- 100 concurrent cutoff/cancellation races;
- injected transition failure;
- exactly-once stop and destructor cleanup;
- no transition after cleanup; and
- invalid plan, schedule, and tone-index rejection.

Results:

- Mac planner test: passed;
- Mac lifecycle test: passed;
- Mac transition test: passed;
- Mac transition test with AddressSanitizer and UndefinedBehaviorSanitizer:
  passed;
- Mac parent WSPR tone regression: blocked by the existing Linux-only
  `-fmax-errors=10` flag being rejected by Apple Clang under `-Werror`;
- wspr5 planner test: passed;
- wspr5 lifecycle test: passed;
- wspr5 transition test: passed; and
- wspr5 parent `wspr-tone-regression-test`: passed after applying the narrow
  test-source exclusion through a temporary Makefile.

## Clock-only probe

The temporary GPL platform driver and runtime overlay acquired only
`clk_gp0`. The overlay contained no pinctrl or GPIO node. The probe held
exclusive rate ownership while the clock remained unprepared and disabled,
requested both endpoints for each profile, read each rate back, and restored
the original 50 MHz rate during every cleanup path.

Integer common-clock readback was within 1 Hz of each requested endpoint:

- word 232444 endpoint: requested 14,097,159 Hz, observed 14,097,158 Hz;
- word 232445 endpoint: requested and observed 14,097,098 Hz; and
- word 232446 endpoint: requested 14,097,038 Hz, observed 14,097,037 Hz.

This shows that the disabled common-clock path can select the three endpoint
rates needed by the four profiles. It does not expose the raw divider word or
provide a timed dither engine.

### Timing under load

With four SHA-256 workers saturating the four CPU cores, a four-profile test
used a nominal 20 ms spacing. Observed event lateness was:

| Tone | Lateness |
|---:|---:|
| 0 | 8 us |
| 1 | 2,804 us |
| 2 | 6,801 us |
| 3 | 14,802 us |

The work item scheduled its successor after completing the preceding rate
changes, so execution time and workqueue delay accumulated. Even an absolute
timer followed by sleep-capable common-clock work would retain uncontrolled
worker latency.

The current 65,536-entry profile would require approximately 96,000 divider
updates per second if materialized across one 0.682667-second WSPR symbol, or
about 10.417 us per update. The tested mechanism missed even a 20 ms synthetic
schedule by up to 14.802 ms and is therefore unsuitable by several orders of
magnitude.

### Cancellation, cutoff, and failure

- A 35 ms cutoff under CPU load allowed tones 0 and 1, restored 50 MHz,
  incremented the generation, and rejected the already queued tone-2 callback
  as stale.
- Explicit runtime-overlay removal during a sequence synchronously cancelled
  pending work and restored the clock; no later transition occurred.
- Twenty rapid overlay-removal cycles under load produced 20 starts and 20
  restorations with no completed sequence and no post-removal transition.
- An injected tone-2 failure restored the clock and invalidated the generation.

Every path finished with:

- GPIO4 input, pull-up, high;
- `clk_gp0` at 50,000,000 Hz from `xosc`;
- enable count 0;
- prepare count 0;
- rate-protection count 0;
- no runtime overlay; and
- no Phase 6 module loaded.

The kernel taint remained 4096 from the temporary out-of-tree probes. No
reboot was performed.

## Evidence

Evidence is retained on `wspr5` at:

```text
/home/pi/rp1-phase6-clock-only-validation/
```

Selected SHA-256 digests are:

```text
6a24d1fcb814888231005ea05cfa8b2be2bdd992d5c85362b70e2d5982d38059  rp1_phase6_clock_probe.c
b3d4724f74edd5fa1278a7140657f38148621ae2858743ea6b68d7799ebbfdb7  rp1_gpclk_transition_test.cpp
11827d972e90eea77b5f82f23b0505533258c02618b5136c38180e430c986973  four-profile-under-load.log
16ba4df4ade2f1e4d02c7bdf5ab972eeb152a8d14fcb6aed8f4010d61af34f95  cutoff-under-load.log
3bb64e8a2d1d9d5aee655ec912522f2615fcd2bb19a5d284b6516abe9d96bb56  rapid-cancellation-under-load.log
```

The complete manifest is `SHA256SUMS` in that directory.

## Supported conclusions

Implemented and demonstrated:

- hardware-independent finite-profile transition planning;
- deterministic generation and cleanup semantics;
- fail-closed cancellation, cutoff, failure, and destruction behavior;
- safe selection and readback of all required endpoint rates while disabled;
  and
- complete restoration after normal and abnormal clock-only paths.

Not implemented or qualified:

- a kernel-owned preloaded/batched divider-update engine;
- deterministic fractional-divider update cadence;
- live GPCLK divider transitions;
- GPIO or RF transition quality;
- WSPR symbol timing, tone spacing, decoding, or production integration; and
- operator-selectable 2, 4, 8, and 12 mA drive settings.

The later drive-strength control remains a separate production-backend and UI
requirement, with 2 mA as the safe default.

## Next engineering gate

Do not render or execute a live-output transition prompt yet. The next phase
must determine and clock-only validate a kernel-owned mechanism that can
preload and execute the finite divider sequence without sleep-capable
per-update common-clock calls. Candidate mechanisms require direct evidence
from RP1 clock, DMA, and PIO capabilities; no candidate is selected by this
report.
