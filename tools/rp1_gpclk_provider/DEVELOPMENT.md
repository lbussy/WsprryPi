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
sudo apt-get install flex bison
```

This is developer-build documentation only. These packages are not runtime or
operator dependencies and must not be added to the WsprryPi runtime installer.
