# RP1 GPCLK Phase 6F provider UAPI

## Disposition

**Passed the versioned UAPI, provider-core, and concrete Linux-client
clock-disabled gates. Stopped at the in-kernel RP1 divider-lease gate.**

No GPCLK output, GPIO mux, pad drive, or RF was enabled.

## Implemented

Phase 6F adds a version 1 ioctl contract for `/dev/rp1-gpclk0`. It contains no
physical address and carries logical divider words rather than RP1 register
values. The concrete WSPR-Transmitter client maps this contract onto the Phase
6E provider abstraction.

Packing moved out of WSPR-Transmitter. The provider core validates 16 fractional
bits and integer divider 3, then packs the fractional portion into the upper 16
bits. It also validates the exact 66,792-write and 511-tick contract, all four
drive selections, exclusive ownership, monotonic generations, stale requests,
finite draining, terminal completion/failure, and release ordering.

The UAPI has explicit version and structure-size fields. Unknown versions,
incorrect sizes, reserved flags, invalid drives, malformed programs, concurrent
ownership, stale generations, and release while active fail closed.

## Exact-kernel finding

The running kernel is `6.18.34+rpt-rpi-2712`. The corresponding official
`raspberrypi/linux` `rpi-6.18.y` source was inspected at
`89586905b8603e545cce9089a81f5f35d65bc998`.

`drivers/clk/clk-rp1.c` owns the GP0 divider regmap and its `regs_lock`, but it
does not expose a supported operation for another RP1 function to lease the
fractional-divider DMA target. Tick and DMA resources arrive through separate
device resources. An out-of-tree companion could repeat the research probe's
device-tree physical address, but that would bypass clock-provider ownership
and violate the Phase 6F requirement.

Accordingly, no loadable provider, kernel patch, or fake direct-address fallback
was produced. The provider-core code is ready to be incorporated once an
in-kernel lease API and ownership model are designed in the RP1 clock/MFD domain.

## Validation

Mac tests passed:

```text
make rp1-gpclk-backend-test rp1-gpclk-linux-provider-test \
  rp1-gpclk-planner-test rp1-gpclk-transition-test \
  rp1-gpclk-lifecycle-test
make -C tools/rp1_gpclk_provider test
```

The broad Mac build still encounters the existing GNU-only
`-fmax-errors=10`/clang incompatibility. This does not affect the focused tests.

The same backend, Linux-client, and provider-core sources compiled and passed on
wspr5 with `-Wall -Wextra -Werror`. The first wspr5 passes exposed two stricter
GCC warnings in test code; both were corrected before the final clean pass.

Final wspr5 state:

```text
GPIO4 input
clk_gp0 prepare_count=0 enable_count=0 rate=50000000
/dev/rp1-gpclk0 absent
```

No module, kernel, overlay, service, boot setting, controller, driver binding,
or persistent hardware state was changed. The synchronized wspr5 repository was
not edited; validation used `/tmp/wsprrypi-phase6f`.

## Validation not performed

- Kernel-provider compilation or installation: the required lease hook does not
  exist yet.
- Real ioctl execution: no provider device was installed.
- DMA submission and cancellation timing: retained from Phase 6D research, not
  repeated through the absent production provider.
- GPIO drive application: deliberately deferred.
- Clock enable, GPIO mux, and RF output: prohibited in this phase.

## Documentation impact

- Updated: engineering report, evidence summary, provider-core README, and next
  phase prompt.
- Considered but unchanged: operator documentation because Pi 5 GPIO output is
  still unavailable to operators.
- Still required: configuration and safety documentation after kernel and RF
  qualification.

## Next gate

Design and implement the in-tree RP1 clock-provider/MFD divider-DMA lease,
incorporate the validated provider core and ioctl dispatch, and compile an exact
wspr5 kernel without installing it. Installation and reboot remain a later,
separately authorized gate.
