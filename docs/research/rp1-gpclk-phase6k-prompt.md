# Phase 6K prompt: minimum-drive live full-frame qualification

Continue Issue #399 on `wspr5.local` and the current
`codex/issue-399-rp1-gpclk` repositories. Do not commit or push unless
separately instructed.

Use the passing Phase 6J exact 162-symbol provider/UAPI and scheduler contract.
Keep the kernel scope limited to the Raspberry Pi OS 64-bit BCM2712 `rpi-2712`
kernel for Raspberry Pi 5, Pi 500, and CM5. Preserve all existing work and
verify the exact intended parent, submodule, kernel, provider, and patch tips
before hardware activity.

The GPIO4 radiator and attached SDR are authorized for this bounded live test.
Use the minimum 2 mA RP1 pad-drive selection and one complete 162-symbol WSPR
frame. Do not make calibrated power claims; use only relative SDR observations.
Confirm once that the configured frequency, GPIO4 path, SDR capture, duration,
and finite cleanup match the established harness, then proceed without asking
again for already-provided transmission details.

First run clock-disabled preflight and confirm exact-frame acceptance, KUnit and
static contracts, `live_output=N`, GPIO4 input, zero GPCLK prepare/enable counts,
and no active owner. Then load the exact provider with its explicit live-output
gate, execute one scheduler-originated full frame at 2 mA, and capture provider,
kernel, GPIO/pinctrl, clock-summary, and SDR evidence across the complete frame.
Demonstrate uninterrupted four-tone cadence at every symbol boundary using the
SDR recording and independently verify total frame timing. Record relative
carrier/tone levels and spurious behavior only.

Exercise one bounded STOP request during a separate minimum-drive frame only if
needed to verify scheduler-to-provider drain semantics; STOP must drain the
already-linked frame and finish with GPIO4 input, GPCLK disabled, ownership
released, and the provider repeatable. On any provider, DMA, clock, GPIO,
continuity, or cleanup regression, stop further live work and restore the safe
state.

Do not claim WSPR decode or product RF qualification in this phase. Do not run
three decode frames unless separately authorized. Do not change the web UI.
Finish by disabling live output, restoring GPIO4 input at the safe 2 mA state,
confirming zero clock counts and normal SDR service state, and producing durable
Phase 6K evidence. Report the live cadence result, relative SDR findings,
STOP/cleanup behavior if exercised, compatibility, remaining decode gate,
documentation impact, and exact repository state.
