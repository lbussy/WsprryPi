# Phase 6L: RP1 WSPR cadence verification

## Outcome

The Phase 6K timing discrepancy was a measurement-layer error, not an RP1 DMA
pacing error. The 110.699-second observation included the provider's deliberate
50 ms final-divider verification delay and the userspace harness's 100 ms poll
interval. Temporary kernel instrumentation measured directly from tick enable
to the DMA completion callback:

- 110.588567604 seconds;
- 110.588576882 seconds; and
- 110.588578178 seconds.

The mean is 110.588574221 seconds, 3.425779 ms fast relative to the nominal
110.592-second WSPR frame, or approximately -30.98 ppm. The tick divider and
DMA dwell settings therefore remain unchanged.

## Cadence contract

The provider now measures every production frame from tick enable to DMA
completion and marks the generation failed if it falls outside 110.592 seconds
plus or minus 6.75 ms. The tolerance is half of one 12 kHz WSPR reference
sample per symbol accumulated across 162 symbols:

`162 * 0.5 / 12000 = 0.00675 seconds`

This is approximately +/-61 ppm over a complete frame. The observed -30.98 ppm
cadence passes with about 3.32 ms margin to the nearer bound. Portable and
KUnit tests cover nominal timing, both inclusive bounds, and one nanosecond
outside each bound.

The RP1 datasheet defines `DMA0_CYCLES` as the number of `clk_tick` cycles
before the next tick and the DMA_TICK `DWELL` field as optional bus-clock idle
cycles in the handshake state machine. Direct callback timing established that
the existing 511-cycle tick and dwell value 19 already produce acceptable WSPR
cadence; changing them based on delayed UAPI observation would have introduced
a real pacing error.

## Validation

- Portable provider arithmetic and lifecycle tests: pass.
- Static ownership, single-submission, and cadence-enforcement contract: pass.
- KUnit provider contract: 2 pass, 0 fail, 0 skip.
- Native Pi debug build: pass.
- RP1 planner/backend, Linux provider, and scheduler backend tests: pass.
- Three production `live_output=N` exact frames: COMPLETE.
- Regenerated 0002 patch applies exactly after 0001 on kernel base
  `89586905b8603e545cce9089a81f5f35d65bc998`.

The clock-disabled terminal observations remained approximately 110.70 seconds
because they intentionally include verification and polling. They are no
longer treated as cadence measurements.

## Minimum-drive live repeat

One scheduler-originated 162-symbol frame ran through GPIO4 at 2 mA with the
SDRplay RSP1B capture path. The scheduler and capture both exited zero with no
SDR overflows. Offline relative analysis found:

- continuous amplitude across all 161 symbol boundaries;
- worst 20 ms boundary minimum: -0.265 dB;
- fifth-percentile boundary minimum: -0.215 dB;
- symbol amplitude spread: 0.107 dB;
- drift-separated tone spacing fit: approximately 1.418 Hz;
- equal-window live/baseline spectral contrast: 66.01 dB; and
- largest other in-band feature outside 20 Hz: -44.78 dBc.

The SDR cold start showed common-mode drift, so these remain relative
observations rather than calibrated frequency or power measurements.

## Cleanup and remaining gate

The run finished with `live_output=N`, GPIO4 input in the safe 2 mA state,
GPCLK0 prepare/enable counts zero, and both WsprryPi and SoapyRemote services
active. Pi 4-and-earlier GPIO behavior, Si5351, CW modes, the web UI, and
operator documentation were unchanged.

Continuous cadence, nominal timing, relative four-tone output, and cleanup now
pass for one Pi 5 GPIO WSPR frame. The remaining RF qualification gate is three
independent scheduler-originated frames decoded as the expected WSPR payload,
with the provider timing contract and safe cleanup passing each time.
