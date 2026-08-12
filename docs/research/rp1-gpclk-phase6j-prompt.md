# Phase 6J prompt: continuous WSPR sequence and scheduler integration

Continue Issue #399 from the passing Phase 6I provider-owned live-output gate.
Work on `wspr5.local` and the current `codex/issue-399-rp1-gpclk`
repositories. Do not commit or push unless separately instructed.

Keep the kernel scope limited to the Raspberry Pi OS 64-bit BCM2712 `rpi-2712`
kernel for Raspberry Pi 5, Pi 500, and CM5. Use `nproc - 1` for ordinary
compiles; native Pi 5/Pi 500/CM5 kernel compiles may use the full processor
count.

Design and implement the smallest provider/UAPI extension that can execute one
complete 162-symbol WSPR divider sequence without gaps at symbol boundaries.
Prefer a bounded single submission over userspace timing or a chain whose next
descriptor depends on scheduler latency. Preserve versioned structure-size
validation, generation ordering, single ownership, STOP-to-safe drain semantics,
close-active deferred cleanup, lease exclusion, and provider-owned clock/pinctrl
restoration. Keep all RP1 register addresses inside `clk-rp1`; do not bypass
pinctrl, gpiolib, the common clock framework, or the GP0 DMA lease.

Add focused portable, KUnit, static-contract, and clock-disabled runtime tests
for exact 162-symbol acceptance, malformed or oversized sequences, divider
ordering, uninterrupted descriptor cadence, early/middle/near-end STOP,
close-active cleanup, repeatability, and lease conflicts. Establish timing from
kernel/DMA evidence before any live output. Fail closed if any regression or
symbol-boundary gap appears.

After the provider contract passes in clock-disabled mode, connect the existing
`Rp1GpclkLinuxProvider` path in `src/WSPR-Transmitter` to WsprryPi's scheduler
for the Pi 5/500/CM5 backend only. Carry an operator-selectable GPIO drive value
through the complete configuration lifecycle, limited to 2, 4, 8, and 12 mA,
with 2 mA as the default. Preserve existing valid configurations and keep Pi 4
and earlier GPIO and Si5351 behavior unchanged. Do not add or change the web UI
in this phase unless separately authorized; if the setting cannot be exposed
without UI work, implement the backend/configuration contract and record the
exact UI and operator-documentation follow-up.

This phase does not authorize a full live WSPR transmission or product RF
qualification. If a bounded live carrier is necessary to prove continuity,
stop at the clock-disabled evidence and identify the exact proposed live gate
for separate transmission authorization. Do not use calibrated power claims;
retain relative observations only.

Produce durable Phase 6J evidence and a concise report separating provider/UAPI
changes, continuous-sequence timing evidence, scheduler/configuration wiring,
power-selection behavior, compatibility, tests, cleanup, documentation impact,
repository state, and the exact remaining live full-frame qualification gate.
