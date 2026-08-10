# Phase 6H prompt: install and clock-disabled provider validation

Continue Issue #399 from the completed Phase 6G source/build gate. Work on
`wspr5.local` using the existing `/home/pi/rpi-linux-phase6g` tree and the
current `codex/issue-399-rp1-gpclk` repositories. Do not commit or push unless
separately instructed.

This phase is limited to the Raspberry Pi OS 64-bit BCM2712 `rpi-2712` kernel
for Raspberry Pi 5, Pi 500, and CM5. Do not add generic ARM64, Pi 4-or-earlier,
32-bit, or cross-build support.

Install the exact Phase 6G kernel, modules, and RP1 GPCLK provider overlay using
a recoverable Raspberry Pi boot layout. Preserve the currently bootable kernel
and record every installed path, hash, boot configuration change, and rollback
procedure. Reboot `wspr5`, confirm it returns on the network, and verify the
running release and provider probe state.

Keep the output path clock-disabled throughout this phase:

- do not select the GPIO4 GPCLK alternate function;
- do not prepare or enable GPCLK0;
- do not change GPIO4 to output;
- do not apply a pad-drive setting;
- do not connect the provider to WsprryPi scheduling;
- do not transmit or perform RF qualification.

With those prohibitions enforced, execute the real provider UAPI and in-kernel
tests. Validate version/size negotiation, single-owner acquisition, invalid
request rejection, generation monotonicity, lease exclusion against conflicting
clock operations, descriptor start/STOP/draining/completion behavior, final
divider readback, early/mid/near-end cancellation, close-during-active cleanup,
and repeatability. Demonstrate that every case returns to an idle provider with
no active DMA/tick transaction while GPIO4 remains input and GPCLK0 remains
unprepared and disabled. Run the compiled KUnit suite and capture its result.

If installation, boot, probe, or a test fails, preserve the failure evidence,
restore the prior bootable configuration when necessary, and stop without
energizing GPIO4. Do not work around a provider failure by exposing an RP1
divider address to userspace or device tree.

Produce durable Phase 6H evidence and a concise report separating implemented
behavior, runtime validation, remaining live-output/RF qualification, system
changes, rollback status, documentation impact, repository status, and the
exact next gate.
