# Phase 6F prompt: RP1 clock-provider implementation and clock-disabled integration

Continue Issue #399 from the uncommitted Phase 6E work. Preserve every existing
change and inspect parent, submodule, and wspr5 state before acting. Use the Mac
for source work and wspr5 as the Linux build and hardware harness.

Implement the provider side of the RP1 GPCLK backend in the RP1 clock-driver
ownership domain, plus the concrete WSPR-Transmitter client for the versioned,
address-independent UAPI defined by that provider. The provider must exclusively
own divider addressing and packing, clock-provider locking, DMA/tick resources,
generation checks, finite 66,792-write symbol completion, timeout handling,
stable final-divider verification, restoration, and teardown. The userspace
client must contain no RP1 register addresses and must use the Phase 6E abstract
backend contract.

Preserve the validated 20 m constants: 50 MHz xosc parent, integer divider 3,
66,792 writes per symbol, tick divider 511, and `DIV_FRAC` packed into the upper
16 bits. Preserve symbol-boundary cancellation: once submitted, the current
descriptor drains; no immediate mid-symbol abort is permitted. Carry the
persisted 2/4/8/12 mA selection through the concrete provider request, default
to 2 mA, and do not apply pad drive in this clock-disabled phase.

Add tests for UAPI version/size rejection, address independence, exact packing,
all four tones, every drive profile, invalid drive rejection, exclusive access,
generation invalidation, cancellation at early/mid/near points, timeout drain,
injected failures, stable restore, cleanup, and channel reuse. Build against the
exact wspr5 kernel source/configuration.

This phase is clock-disabled only. Do not mux GPIO4 to GPCLK, prepare or enable
clk_gp0, energize RF, use `/dev/mem`, touch companion-owned clock registers,
install a kernel or module, change boot configuration or services, reboot,
reset a controller, or unbind a driver. If the exact provider cannot be loaded
without installation/reboot, finish source/build/test validation and stop at a
clearly documented installation gate.

Retain evidence in a new Phase 6F evidence directory, write the Phase 6F report,
run final diff/whitespace and repository-state checks, and render the exact next
prompt. Do not commit or push.
