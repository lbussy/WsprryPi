# RP1 GPCLK Phase 6G: in-tree provider source/build gate

## Outcome

Phase 6G produced an in-tree RP1 GPCLK provider candidate and compiled it with
the exact Raspberry Pi OS 64-bit BCM2712 `rpi-2712` kernel configuration used by
`wspr5`. The full native `Image modules` build passed. Nothing was installed or
loaded, and GPCLK0 remained disabled.

This work is intentionally scoped to Raspberry Pi 5, Pi 500, and CM5 systems
using the Raspberry Pi OS 64-bit BCM2712-optimized kernel. It does not establish
support for generic ARM64 kernels, Pi 4 or earlier, 32-bit kernels, or cross
builds.

## Kernel ownership model

The first patch adds a GP0 DMA lease to `clk-rp1`. The clock driver derives the
divider DMA target from its own resource and does not publish that address to
device tree or userspace. While leased, ordinary GP0 prepare, parent, and rate
changes are rejected, and the lease also holds a common-clock exclusive-rate
claim.

The second patch adds the provider, its versioned UAPI, and KUnit contract
tests. The provider owns divider-word packing and the deterministic update
buffer. Its overlay supplies only tick and DMA resources plus the clock
phandle; it contains no divider-register address.

The provider implements the Phase 6F lifecycle contract, including generation
tracking, STOP-to-draining behavior, completion cleanup, stable divider
readback, and deferred release when a descriptor is still active. It does not
prepare or enable GPCLK0, select GPIO4's alternate function, or apply pad drive.

## Validation performed

- Applied both patches in sequence to Raspberry Pi Linux `rpi-6.18.y` commit
  `89586905b8603e545cce9089a81f5f35d65bc998` in a clean worktree.
- Built the modified `clk-rp1`, provider, and KUnit objects in the official
  source tree.
- Built the exact-config ARM64 kernel `Image` and all modules natively on
  `wspr5` with its explicitly permitted Pi 5 full-processor count (`nproc=4`,
  `-j4`); an incremental confirmation build exited successfully. Other native
  compiles use one fewer job than the available processor count.
- Built the provider and KUnit modules against the exact installed kernel
  headers.
- Compiled the uninstalled provider overlay.
- Passed the portable provider tests and static ownership checks.
- Passed whitespace checks in both the official kernel source tree and this
  repository.

The detailed hashes, sizes, versions, and final hardware state are recorded in
[`rp1-gpclk-phase6g-evidence/summary.txt`](rp1-gpclk-phase6g-evidence/summary.txt).

## System change made for the build

`flex` and `bison` were installed on `wspr5` with their Debian support
dependencies `m4` and `libfl-dev`. They are developer-only kernel-build
dependencies, not WsprryPi runtime dependencies. The supported build scope and
dependency command are recorded in
[`tools/rp1_gpclk_provider/DEVELOPMENT.md`](../../tools/rp1_gpclk_provider/DEVELOPMENT.md).

## Remaining gate

Phase 6G does not validate driver probe, UAPI calls, lease exclusion, DMA
completion, cancellation, or cleanup in the running kernel. Those require a
separately authorized install/reboot phase. In-kernel KUnit execution also
remains pending because no Phase 6G module was loaded.

The next phase must first install the exact built kernel, modules, and overlay,
then reboot and validate the provider while GPCLK0 and GPIO4 output remain
disabled. Live pin muxing, clock output, pad-drive application, and RF remain
outside that phase.
