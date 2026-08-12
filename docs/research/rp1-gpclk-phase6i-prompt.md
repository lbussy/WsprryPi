# Phase 6I prompt: provider-owned minimum-power live-output gate

Continue Issue #399 from the passing Phase 6H clock-disabled runtime gate. Work
on `wspr5.local` and the current `codex/issue-399-rp1-gpclk` repositories. Do
not commit or push unless separately instructed.

Keep the kernel scope limited to the Raspberry Pi OS 64-bit BCM2712 `rpi-2712`
kernel for Raspberry Pi 5, Pi 500, and CM5. Use `nproc - 1` for ordinary
compiles; native Pi 5/Pi 500/CM5 kernel compiles may use the full processor
count.

Implement the smallest provider-owned output lifecycle needed to validate real
GPCLK0 on GPIO4. The provider must apply the drive strength requested through
the existing UAPI, select GPIO4's GPCLK0 function, prepare/enable the clock only
for an acquired active program, and restore the prior safe pin/clock/drive state
on normal completion, STOP/drain completion, close-during-active cleanup,
failure, module removal, and reboot. Do not bypass pinctrl, gpiolib, the common
clock framework, or the clk-rp1 lease. Do not expose any RP1 register address to
userspace or device tree.

Before live output, repeat the clock-disabled KUnit, ownership, cancellation,
cleanup, and lease-exclusion checks. Fail closed if any regression appears.

The attached GPIO4 radiator and SDR are the authorized test harness. Perform a
bounded live-output validation at the minimum selectable GPIO drive level. Use
relative SDR on/off and transition evidence only; do not attempt calibrated
power measurement. Validate carrier presence, four-symbol divider transitions,
early/middle/near-end STOP behavior, repeatability, and final cleanup. Confirm
after every case that GPIO4 is no longer muxed to GPCLK0, GPCLK0 has zero
prepare/enable counts, DMA/tick activity is idle, and the requested drive level
was applied during output and restored afterward.

Do not connect the provider to the WsprryPi scheduler or claim WSPR/RF product
qualification in this phase. Preserve installed-file hashes, boot/rollback
details, kernel and SDR evidence, exact relative observations, and all failure
evidence.

Produce durable Phase 6I evidence and a concise report separating implementation,
clock-disabled regression results, live-output results, power-selection
behavior, cleanup/restoration, remaining scheduler integration and RF
qualification, documentation impact, repository state, and the exact next gate.
