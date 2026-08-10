# RP1 GPCLK provider core

This directory contains the hardware-independent validation and state-machine
core for the version 1 RP1 GPCLK provider UAPI. It deliberately contains no RP1
register address. The provider validates logical divider words and owns their
conversion to the RP1 `DIV_FRAC` register representation.

The `kernel/` directory contains the in-tree lease patch, provider driver,
KUnit contract tests, static ownership test, and an overlay that supplies the
tick/DMA resources and provider-owned GPIO4 pinctrl states. The overlay
intentionally contains no GPCLK divider address; that address is derived and
leased inside `clk-rp1`.

The lease carries the CPU physical divider-resource address. This is the form
required by the RP1 DW AXI DMA engine interface; the DMA driver performs the
RP1 bus-address translation when it prepares the hardware descriptor. Passing
an already translated DMA address would translate it twice and is invalid.

The provider fails closed by default. Loading it without the read-only
`live_output=1` module parameter exercises the real DMA and state-machine path
without enabling GPCLK0 or selecting its GPIO4 function. With that explicit
gate enabled, an acquired program selects the requested 2, 4, 8, or 12 mA
pinctrl state, enables the leased clock, and restores GPIO4 to a 2 mA input and
GPCLK0 to disabled on every terminal path. The provider uses pinctrl and the
common-clock lease rather than exposing or directly accessing RP1 register
addresses.

UAPI version 1 accepts exactly one complete 162-symbol WSPR frame. Userspace
submits four logical divider plans and the ordered 162-entry tone-index array;
the provider expands that bounded request into one contiguous DMA buffer and
prepares it as one finite DMA-engine submission. STOP changes the observable
state to draining but does not truncate the already-linked frame. Closing the
owner while active defers lease release until that same finite drain and
provider-owned cleanup complete.

The portable core remains independently testable and cannot enable, mux, or
manipulate GPCLK hardware.

Run its contract tests with:

```sh
make test
```

The kernel patch adds the missing clock-provider lease against the recorded
Raspberry Pi 6.18 source revision. It blocks GP0 enable, parent, and rate
changes by unrelated callers while leased, holds a common-clock exclusive-rate
claim, and keeps the DMA target out of userspace and device tree. Provider-owned
clock enable and disable are permitted only through the active lease owner.
These artifacts remain engineering candidates for the Raspberry Pi OS 64-bit
BCM2712 kernel; they are not a portable or upstream-qualified interface.
