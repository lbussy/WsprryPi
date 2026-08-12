# Phase 6P prompt: three-frame Pi 5 GPIO WSPR decode qualification

Continue Issue #399 on `wspr5.local` and the current
`codex/issue-399-rp1-gpclk` repositories. Do not commit or push unless
separately instructed.

Use the passing Phase 6O persistent-provider, fail-closed, KUnit, and
clock-disabled production-frame evidence. Keep scope limited to three
independent scheduler-originated WSPR frames through the Raspberry Pi
5/BCM2712 RP1 provider on GPIO4 at the minimum 2 mA pad drive. Do not implement
CW, change the web UI, update operator documentation, change the available RP1
drive selections, calibrate absolute power, or make unrelated kernel changes.

The GPIO4 radiator and attached SDR are authorized for these bounded live
transmissions. Use relative SDR findings only. Do not ask again for the
already-established transmission authorization, load, receiver, or absolute
power calibration. Verify once that the parent and transmitter source match
the intended Phase 6O tips, the boot-selected kernel and installed provider
match the recorded persistent artifacts, the submit UAPI is `0x4130b701`, the
provider starts with `live_output=N`, and GPIO4 and GPCLK counts are safe.

Before enabling output, run the focused fail-closed, portable-provider,
static-contract, KUnit, timing-window, and cleanup preflight. Confirm the
intended WSPR identity, 20 m frequency path, finite three-frame schedule, SDR
capture path, decoder path, and unconditional cleanup once. Then enable provider
live output only for the bounded qualification and proceed without pausing for
permission.

Transmit and capture three separate complete 162-symbol frames from the normal
WsprryPi scheduler at 2 mA. Independently decode each capture with a standard
WSPR decoder or the repository reference decoder path appropriate to real IQ
samples. Require all three frames to decode the expected callsign, locator, and
reported WSPR power. For each frame retain provider terminal state, enforced
cadence result, all-boundary continuity, relative four-tone/spectrum evidence,
decoder output, GPIO/pinctrl restoration, clock counts, and service state. A
failed decode may be investigated from the captured evidence, but do not
substitute encoded-symbol comparison for an over-the-air decode.

Stop further live work on any provider, DMA, cadence, continuity, GPIO, lease,
cleanup, scheduler, or fail-closed regression. Ensure cleanup runs after every
success, failure, timeout, signal, or decoder error. Finish with
`live_output=N`, GPIO4 input in the safe 2 mA state, zero GPCLK prepare/enable
counts, and both WsprryPi and SoapyRemote services active.

Produce durable Phase 6P evidence and report each frame and decode separately,
relative SDR findings, timing and continuity, cleanup, compatibility,
documentation impact, repository state, and whether Pi 5 GPIO WSPR has met the
three-independent-decode qualification gate. Keep CW qualification and the
later operator power-selection workflow explicitly listed as future work.
