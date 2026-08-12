# Phase 6G prompt: in-tree RP1 divider-DMA lease and exact-kernel build

Continue Issue #399 from the uncommitted Phase 6F work. Preserve all changes and
verify the parent, WSPR-Transmitter, and wspr5 states before acting.

Using the official Raspberry Pi `rpi-6.18.y` kernel source matching wspr5, design
and implement an in-tree ownership mechanism that allows the RP1 GPCLK provider
to lease the GP0 fractional-divider DMA target while remaining synchronized with
the clock driver's regmap lock and ordinary common-clock operations. Coordinate
the RP1 clock, MFD/resource, tick, and DMA ownership boundaries explicitly. Do
not expose a physical register address to userspace or accept a device-tree
address supplied by a companion driver.

Integrate the Phase 6F versioned UAPI, provider core, and ioctl dispatch. The
kernel provider must own packing, exclusive acquisition, 50 MHz state capture,
finite 66,792-write descriptors, tick divider 511, generation validation,
symbol-boundary cancellation, timeout drain, stable final readback, restoration,
and teardown. Carry but do not apply the 2/4/8/12 mA request.

Add KUnit or equivalent kernel-side tests for version/size validation, exact
packing, every tone and drive, exclusive access, stale generations, early/mid/
near cancellation semantics, timeouts, injected failures, cleanup, and reuse.
Add static checks proving userspace contains no RP1 address and the provider
does not prepare or enable GPCLK in the clock-disabled configuration.

Build the complete kernel/provider artifacts against wspr5's exact config and
toolchain. Do not install a kernel or module, modify boot configuration, load an
overlay, reboot, bind or unbind drivers, mux GPIO4, prepare or enable clk_gp0,
apply pad drive, or energize RF. Stop at the reviewed installation gate.

Retain Phase 6G evidence, write the engineering report, verify final repository
and hardware state, run whitespace checks, and render the exact next prompt.
Do not commit or push.
