# RP1 GPCLK provider core

This directory contains the hardware-independent validation and state-machine
core for the version 1 RP1 GPCLK provider UAPI. It deliberately contains no RP1
register address. The provider validates logical divider words and owns their
conversion to the RP1 `DIV_FRAC` register representation.

The `kernel/` directory contains the Phase 6G in-tree lease patch, provider
driver, KUnit contract tests, static ownership test, and an uninstalled overlay
that supplies only tick/DMA resources. The overlay intentionally contains no
GPCLK divider address; that address is derived and leased inside `clk-rp1`.

The lease carries the CPU physical divider-resource address. This is the form
required by the RP1 DW AXI DMA engine interface; the DMA driver performs the
RP1 bus-address translation when it prepares the hardware descriptor. Passing
an already translated DMA address would translate it twice and is invalid.

The portable core remains independently testable and cannot enable, mux, or
manipulate GPCLK hardware.

Run its contract tests with:

```sh
make test
```

Phase 6G adds the missing clock-provider lease as a patch against the recorded
Raspberry Pi 6.18 source revision. It blocks GP0 enable, parent, and rate changes
while leased, holds a common-clock exclusive-rate claim, and keeps the DMA target
out of userspace and device tree. These artifacts are engineering candidates;
they must be reviewed and installed only in a separately authorized phase.
