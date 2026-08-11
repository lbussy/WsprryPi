# RP1 GPCLK kernel development dependencies

## Supported kernel build target

Kernel and kernel-driver development for this backend is intentionally limited
to the Raspberry Pi OS 64-bit BCM2712-optimized kernel used by Raspberry Pi 5,
Pi 500, and CM5 systems. Use the target system's installed `rpi-2712`
configuration and native AArch64 toolchain.

Generic ARM64 distribution kernels, Raspberry Pi 4-and-earlier kernels, 32-bit
kernels, and cross-architecture build instructions are outside this workflow.

For normal native builds, use one fewer job than the number of available
processors. Full processor count is explicitly permitted for native kernel
builds on Pi 5, Pi 500, and CM5; for example, `wspr5` reports four processors
and the Phase 6G kernel build used `make -j4 Image modules`.

The Phase 6G exact-kernel build requires the normal Raspberry Pi kernel build
toolchain plus Kconfig's generated-parser dependencies:

- `flex` (validated with 2.6.4)
- `bison` (validated with 3.8.2)
- `m4` and `libfl-dev` (installed by Debian as Flex support dependencies)
- GCC and binutils matching the target kernel architecture
- the exact target kernel configuration

On Debian Trixie, the additional packages can be installed with:

```sh
sudo apt-get install flex bison m4 libfl-dev build-essential bc kmod \
  libssl-dev device-tree-compiler
```

This is developer-build documentation only. These packages are not runtime or
operator dependencies and must not be added to the WsprryPi runtime installer.

## Exact source and configuration

The qualified engineering build used the official `raspberrypi/linux`
`rpi-6.18.y` tree at commit
`89586905b8603e545cce9089a81f5f35d65bc998`. That commit identifies the
Issue 399 evidence; it is not permission to substitute a different kernel for
the kernel selected at boot.

Start with a clean source tree and record all identities before patching:

```sh
git clone --branch rpi-6.18.y https://github.com/raspberrypi/linux.git rpi-linux
cd rpi-linux
git checkout 89586905b8603e545cce9089a81f5f35d65bc998
git status --short
uname -r
sha256sum /boot/config-"$(uname -r)"
```

Copy the target system's `rpi-2712` configuration to `.config`, apply
`kernel/0001-clk-rp1-add-gp0-dma-lease.patch` followed by
`kernel/0002-clk-rp1-add-gpclk-provider.patch` and
`kernel/0003-rp1-gpclk-add-finite-event-executor.patch`, followed by
`kernel/0004-rp1-gpclk-enable-live-finite-events.patch`, in that order, and
enable:

```text
CONFIG_KUNIT=y
CONFIG_RP1_GPCLK_PROVIDER=m
CONFIG_RP1_GPCLK_PROVIDER_KUNIT_TEST=m
```

Run `make olddefconfig`, then verify that the resulting configuration retains
the BCM2712 target and 16 KiB ARM64 page size. Preserve the final `.config`
hash with the source commit and built artifacts. Do not reuse modules from a
different kernel release, configuration, or source revision.

## Build and source-level validation

On Pi 5, Pi 500, or CM5, full processor count is permitted:

```sh
make -j"$(nproc)" Image modules
```

On other build hosts, use one fewer job than the available processor count.
The Issue 399 build is native AArch64; cross-architecture instructions are not
part of this contract.

Before installation, run the hardware-independent contracts from the WsprryPi
checkout:

```sh
make -C tools/rp1_gpclk_provider test
tools/rp1_gpclk_provider/kernel/static_contract_test.sh
make -C src -j"$(nproc)" \
  rp1-gpclk-planner-test \
  rp1-gpclk-lifecycle-test \
  rp1-gpclk-transition-test \
  rp1-gpclk-backend-test \
  rp1-gpclk-linux-provider-test \
  rp1-gpclk-transmit-backend-test
```

These tests do not qualify GPIO, DMA timing, RF output, or a kernel
installation. KUnit execution requires the exact matching kernel and module;
building KUnit is not equivalent to loading it.

The third patch packages the version-2 finite-event request and clock-disabled
executor contract. The fourth patch precomputes one bounded divider DMA stream
and uses an absolute soft-hrtimer deadline chain only for start/event-boundary
gating and state. While the exclusive GPCLK0 DMA lease is active, the boundary
gate reads and writes only GPCLK0's dedicated control register through raw MMIO.
It does not use regmap, the clock manager's shared `regs_lock`, the shared GPCLK
output-enable register, pinctrl, or the common-clock API. The normal clock path
keeps the shared output-enable bit prepared until process-context cleanup
disables the lease clock and restores the safe GPIO pin state.

Submission leaves tick requests disabled, prepares the clock behind the safe
input pin, forces the hardware gate off, selects the transmitting pin state,
and arms a start epoch one millisecond in the future. The start callback alone
applies the first event gate, executes a write barrier, and then enables tick
requests. The RF gate therefore precedes possible consumption of divider word
zero, but the writes are not claimed to be simultaneous hardware edges. The
callback then advances the same timer to `start_epoch + first_event_duration`.
Later deadlines add durations to
that absolute chain rather than to callback arrival time. STOP, owner close,
and provider removal cancel an armed start before it can enable DMA or RF.

DMA boundaries use cumulative duration mapped onto the fixed WSPR tick grid.
Each accepted event advances by at least one tick, total duration is bounded by
the existing 110.592-second coherent buffer, and cumulative flooring bounds
representation error below one tick without per-event rounding accumulation.
Any observable gate failure at start, a later boundary, or terminal shutdown
produces a failed state, stops tick generation, schedules safe process-context
cleanup, and preserves an earlier terminal failure such as a missed deadline.
The live-output path remains unqualified until that exact executor has been
separately installed and exercised on matching hardware.

## Installation contract

Kernel, module, overlay, boot-file, and reboot changes require a separately
authorized maintenance window. Stop if the target kernel release or artifact
identity is ambiguous.

Install the engineered image alongside the packaged `kernel_2712.img`; do not
overwrite the packaged image or `initramfs_2712`. Install the matching provider
and KUnit modules under:

```text
/lib/modules/<exact-release>/kernel/drivers/clk/rp1-gpclk-provider.ko.xz
/lib/modules/<exact-release>/kernel/drivers/clk/rp1-gpclk-provider-kunit.ko.xz
```

Install the compiled overlay as:

```text
/boot/firmware/overlays/rp1-gpclk-provider.dtbo
```

Back up every destination before replacement, retain hashes of built and
installed artifacts, and run `depmod -a <exact-release>`. The qualified boot
selection used:

```text
auto_initramfs=0
kernel=kernel_2712_phase6h.img
dtoverlay=rp1-gpclk-provider
```

Use a one-shot boot selection for initial validation and retain a known-good
packaged-kernel rollback path. Do not make the engineered kernel persistent
until its image, modules, overlay, UAPI, and clock-disabled behavior have all
been verified.

`live_output` is a read-only load-time parameter and defaults to false. Do not
add `live_output=1` to persistent module configuration. Clock-disabled
validation must observe `live_output=N` before opening the provider.

## Post-boot identity checks

After reboot, prove that the running kernel selected the artifacts just built:

```sh
uname -r
/sbin/modinfo -F filename rp1_gpclk_provider
/sbin/modinfo -F srcversion rp1_gpclk_provider
/sbin/modinfo -F filename rp1_gpclk_provider_kunit
/sbin/modinfo -F srcversion rp1_gpclk_provider_kunit
cat /sys/module/rp1_gpclk_provider/parameters/live_output
stat /dev/rp1-gpclk0
grep -E '^(auto_initramfs|kernel|dtoverlay=rp1-gpclk-provider)=' \
  /boot/firmware/config.txt
```

Compare decompressed module hashes when the installed modules are compressed.
The loaded `srcversion`, installed module, running release, UAPI structure size,
and ioctl numbers must agree with the userspace client. A temporary `insmod`
success is not persistent-install evidence.

If `/dev/rp1-gpclk0` is absent, an ioctl returns `ENOTTY` or `EPROTO`, or the
loaded and installed identities differ, keep transmission inhibited. Restore
the backed-up modules or select the packaged kernel, run `depmod`, reboot, and
repeat the identity audit. Never fall back silently to another transmitter.

## Lease, repeat-run, and cleanup semantics

`ACQUIRE` establishes an exclusive provider lease and resets the lease-local
previous generation to zero. The first submitted frame for every new process or
owner therefore uses generation `1`. Generations must increase within one
lease, but they do not carry across owners. Stale-generation operations fail.

Only one complete 162-symbol WSPR frame is accepted per submission. STOP marks
the finite descriptor as draining; it does not truncate the already-linked
frame. Release remains busy until the descriptor reaches a terminal state.
Closing an active owner defers lease release until provider cleanup completes.

Every terminal path must leave GPCLK0 disabled and unprepared, return GPIO4 to
input at 2 mA, release the clock lease, and permit a new owner to acquire the
provider. Repeat-run validation must use separate owners as well as repeated
submissions within the applicable lease contract.

## Qualification boundary

The qualified RF cell is limited to Raspberry Pi 5, RP1 GPCLK0, GPIO4, 20 m
WSPR, and 2 mA drive. Phase 6R produced three independent decoded frames for
that cell. GPIO20, other bands, higher drive settings, CW, absolute output
power, spectral compliance, and other Pi or kernel configurations remain
unqualified.
