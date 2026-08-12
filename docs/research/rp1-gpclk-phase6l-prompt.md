# Phase 6L prompt: correct RP1 WSPR tick cadence

Continue Issue #399 on `wspr5.local` and the current
`codex/issue-399-rp1-gpclk` repositories. Do not commit or push unless
separately instructed.

Use the Phase 6K evidence showing continuous energy across all 161 WSPR symbol
boundaries at minimum 2 mA, but a measured provider frame duration of
110.699024 seconds instead of the nominal 110.592000 seconds. Treat the
approximately +107.024 ms / +967.74 ppm pacing error as the only approved
engineering target. Do not broaden this phase into decode qualification, CW,
web UI, operator documentation, or unrelated provider cleanup.

Determine the effective RP1 DMA tick-rate equation from the BCM2712/RP1 kernel
clock, tick-generator, and DW AXI DMA contracts. Correct or parameterize the
provider tick cadence so 66,792 divider writes occupy exactly one nominal WSPR
symbol interval of 8192/12000 seconds, without introducing userspace timing,
workqueue-dependent descriptor transitions, or gaps in the existing single
finite DMA submission. Preserve the exact 162-symbol UAPI, generation and
ownership rules, STOP-to-drain behavior, close-active deferred cleanup, GPCLK
lease, pinctrl ownership, 2/4/8/12 mA selection, and fail-closed live gate.

Add focused arithmetic, portable, KUnit, and static-contract tests for the
derived tick cadence and its rounding bounds. Rebuild the Raspberry Pi OS
64-bit BCM2712 `rpi-2712` provider using the permitted native processor count.
First validate exact full-frame timing repeatedly with `live_output=N`; require
the measured 162-symbol duration and per-symbol average to meet an explicitly
stated WSPR tolerance, with no DMA stall, warning, or cleanup regression.

Only after the clock-disabled timing gate passes, use the already-authorized
GPIO4 radiator and attached SDR for one minimum-2-mA live frame. Capture the
full frame and repeat the Phase 6K all-boundary, four-tone, relative-spectrum,
and safe-cleanup analysis. Use relative findings only and do not claim
calibrated power. Stop further live work if cadence, continuity, ownership, or
cleanup regresses.

Finish with `live_output=N`, GPIO4 input in the safe 2 mA state, zero GPCLK
prepare/enable counts, and both WsprryPi and SoapyRemote services active.
Produce durable Phase 6L evidence and report the derived cadence equation,
implementation, timing error before and after, boundary continuity, relative
SDR result, tests, compatibility, cleanup, documentation impact, repository
state, and the exact remaining three-decode qualification gate.
