# RP1 GPCLK clock-disabled DMA probe

This research-only kernel probe tests whether RP1's DMA0 tick can pace a
preloaded sequence targeted at GPCLK0's fractional-divider register.
It acquires `clk_gp0` with exclusive rate ownership but deliberately never
prepares or enables it. The overlay declares no GPIO or pinctrl configuration.

This is not a production register-access interface. A production backend must
move DMA setup and divider-register ownership into the RP1 clock provider so
that DMA writes participate in that driver's locking and lifecycle contract.

The checked-in fixture uses 66,792 transfers at the slowest 511-cycle DMA tick.
That hardware-sized descriptor completed within 4.2 us of one WSPR symbol in
four clock-disabled runs on `wspr5`. A subsequent DMA readback returned zero
after a known nonzero divider was set through the clock framework, so the
prototype intentionally fails closed and does not claim that divider writes
were accepted. The next proof belongs inside the RP1 clock provider.
