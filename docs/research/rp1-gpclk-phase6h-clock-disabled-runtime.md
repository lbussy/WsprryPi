# RP1 GPCLK Phase 6H: clock-disabled runtime validation

## Outcome

Phase 6H passed after exposing and correcting one bounded DMA-address contract
defect. The exact Raspberry Pi OS 64-bit BCM2712 kernel and provider now probe
and complete real tick-paced DMA programs on `wspr5`, including early, middle,
and near-end STOP/drain cases. GPIO4 remained input and GPCLK0 remained
unprepared and disabled throughout. No RF output occurred.

This remains specific to Raspberry Pi 5, Pi 500, and CM5 using the Raspberry Pi
OS `rpi-2712` kernel. It does not qualify other kernels or Pi generations.

## Recoverable installation

The custom kernel, matching modules, and overlay were installed alongside the
packaged kernel rather than replacing it. Raspberry Pi's one-shot `tryboot.txt`
path selected the test image. Normal `config.txt` remained byte-for-byte
unchanged, and the packaged `kernel_2712.img` and `initramfs_2712` remain in
place. An ordinary reboot therefore returns to the packaged-kernel path.

The test system is currently running `6.18.44-v8-16k+`. The rollback path was
preserved but was not exercised after the successful fixed run.

## Runtime finding and fix

The first boot probed the provider and passed KUnit, but every real descriptor
ended in `FAILED`. The DW AXI DMA diagnostic reported destination
`0xffffffffffffffff`.

The lease had converted the CPU physical divider address with `phys_to_dma()`.
The RP1 DW AXI DMA driver then performed its own required translation while
preparing the descriptor, so the address was translated twice. The corrected
lease carries the CPU physical address derived inside `clk-rp1`; the DMA engine
alone translates it to the RP1 bus address. Neither device tree nor userspace
receives the divider address.

The corrected patches apply cleanly to the recorded Raspberry Pi kernel source
commit and reproduce the exact sources used for the passing rebuild.

## Validation

The fixed kernel passed:

- KUnit contract suite: 2 passed, 0 failed;
- valid and invalid UAPI version/size handling;
- drive-value and reserved-field validation;
- single-owner acquisition;
- invalid program and generation rejection;
- early, middle, and near-end STOP-to-drain completion;
- close during an active descriptor, deferred cleanup, and reacquisition;
- repeat descriptor completion; and
- conflicting common-clock rate and prepare/enable operations returning
  `-EBUSY` while the lease was active.

The temporary lease-conflict module was unloaded after use. Loading it marked
the test kernel tainted, as expected for an out-of-tree validation module; it
did not leave GPCLK prepared or enabled.

Exact hashes, return codes, installed paths, rollback details, and final state
are in [`rp1-gpclk-phase6h-evidence/summary.txt`](rp1-gpclk-phase6h-evidence/summary.txt).

## Remaining qualification

Phase 6H did not select the GPIO4 GPCLK alternate function, apply a pad-drive
setting, enable GPCLK0, connect the provider to WsprryPi scheduling, or perform
RF validation. Those remain separate gates.

The next phase should add provider-owned output activation and cleanup,
including explicit GPIO4 mux and requested drive application, and validate it
with a bounded minimum-power live-output run. The operator-selected drive value
must remain carried through the UAPI and must be demonstrably applied and
restored rather than merely validated.
