# RP1 GPCLK clock-disabled DMA probe

This research-only kernel probe tests whether RP1's DMA0 tick can pace a
preloaded sequence targeted at GPCLK0's fractional-divider register.
It acquires `clk_gp0` with exclusive rate ownership but deliberately never
prepares or enables it. The overlay declares no GPIO or pinctrl configuration.

This is not a production register-access interface. A production backend must
move DMA setup and divider-register ownership into the RP1 clock provider so
that DMA writes participate in that driver's locking and lifecycle contract.

The checked-in fixture uses 66,792 transfers at the slowest 511-cycle DMA tick.
Phase 6C completed all four clock-disabled descriptors within 4.8 us of one
WSPR symbol. It also corrected the register packing: RP1 expects the logical 16-bit
fraction in bits 31:16 of `DIV_FRAC`. With that shift, exact DMA readback and
the existing provider's raw regmap reads both matched all four final words.

The optional `cancel_ms` research path must not be reused on a running harness.
Its first test disabled the tick before generic DMA termination, leaving the
DW AXI DMA channel non-idle. The checked-in probe now rejects nonzero
`cancel_ms` with `-EOPNOTSUPP`; the failed implementation remains visible only
to preserve the engineering evidence. Recover the controller under an
authorized reboot, then redesign termination before running another
cancellation test.
