# RP1 GPCLK provider core

This directory contains the hardware-independent validation and state-machine
core for the version 1 RP1 GPCLK provider UAPI. It deliberately contains no RP1
register address. The provider validates logical divider words and owns their
conversion to the RP1 `DIV_FRAC` register representation.

The core is suitable for inclusion in the eventual in-kernel provider after the
RP1 clock driver exposes a supported exclusive divider/DMA lease. It is not a
loadable driver and cannot enable, mux, or manipulate GPCLK hardware.

Run its contract tests with:

```sh
make test
```

The running Raspberry Pi 6.18 clock driver provides ordinary common-clock
operations but no interface for an external DMA client to lease the GP0 divider
register while remaining synchronized with its regmap lock. Reusing the Phase
6D device-tree address would violate the provider-ownership requirement. The
missing clock-provider lease must therefore be implemented and reviewed before
this core is connected to tick-paced DMA.
