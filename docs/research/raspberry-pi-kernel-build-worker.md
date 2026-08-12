# Raspberry Pi kernel build worker

For the ordered, clean-room-validated procedure to create or operate this
environment, use the canonical
[Raspberry Pi ARM64 kernel build worker setup](../raspberry-pi-kernel-build-worker-setup.md)
guide. This report records demonstrated capability and evidence; it is not a
substitute for the setup and safety contract.

## Outcome

The Debian 13 ARM64 `wsprmac` worker can reproducibly build and stage Raspberry
Pi ARM64 kernels without changing the target Pi. Its validated reference build
completed successfully, and the two custom kernel modules and device-tree
overlay matched the artifacts installed on `wspr5` byte-for-byte.

This is source-provenance and build-system validation. It does not qualify a
new kernel deployment, hardware behavior, GPIO output, transmission, or RF
performance. The corresponding target qualification for the validated
reference build remains documented in
[Packaged RP1 GPCLK executor qualification](rp1-gpclk-phase6y-packaged-executor-qualification.md).

## Reproducible inputs

The validated reference build used a detached checkout of the official
Raspberry Pi Linux tree with these immutable inputs:

- Raspberry Pi Linux base commit:
  `89586905b8603e545cce9089a81f5f35d65bc998`
- Target configuration SHA-256:
  `0e06e43ff262adad8074d8fa3fb607cbd151b834ed49d37ef9743518dfdfb8f7`
- Patch 1 SHA-256:
  `01b27ef41b636e4a87e737cb4ec7b420bbd686db517875e33926223143d115b0`
- Patch 2 SHA-256:
  `c7d65d198d11eac857576d243c47be5e9f684db5c0521c8d2cb55d53263c56e5`
- Patch 3 SHA-256:
  `73dc07eb893e84790c063f2811e3f0608568015dc188b6769d83cc0889ebcfd0`
- Patch 4 SHA-256:
  `214bb9fa6ce253bb321b617f53ffccf9cf7eea812f22b1a37a26acb7dfbb050d`
- Overlay source SHA-256:
  `6511249139ab1e1af72e9c4b9e08fabdaa6f9f3030de133256519119f9b6fd0f`

The four patches passed `git apply --check` and applied in order. A subsequent
comparison confirmed that all eight modified or added paths matched the
retained reference source tree on `wspr5`:

- `drivers/clk/Kconfig`
- `drivers/clk/Makefile`
- `drivers/clk/clk-rp1.c`
- `drivers/clk/rp1-gpclk-contract.h`
- `drivers/clk/rp1-gpclk-provider-kunit.c`
- `drivers/clk/rp1-gpclk-provider.c`
- `include/linux/rp1-gpclk-lease.h`
- `include/uapi/linux/rp1_gpclk.h`

The preserved target configuration was normalized without
`CROSS_COMPILE_COMPAT` and remained byte-identical. Supplying the worker's
optional ARM32 compatibility toolchain would have added
`CONFIG_COMPAT_VDSO` and `CONFIG_THUMB2_COMPAT_VDSO`; that host-only divergence
was rejected for the validated reference build.

## Isolated worker layout

The reference build used a separate development namespace beneath
`/home/pi/kernel-work` on `wsprmac`:

- Inputs: `src/issue399-phase6x-inputs`
- Source: `src/linux-rpi-issue399-phase6x`
- Out-of-tree build: `build/issue399-phase6x`
- Staging: `stage/issue399-phase6x-20260812T005812Z`
- Build log: `logs/step12a-phase6x-build-20260812T003900Z.log`
- Stage log: `logs/step12a-phase6x-stage-20260812T005812Z.log`

The pinned 6.18.34 stock source, stock build output, stock staging bundle, and
earlier worker evidence were not reused or modified.

## Build result

The native ARM64 build used `make -j4` with the targets `Image modules dtbs`.

| Result | Value |
| --- | ---: |
| Kernel release | `6.18.44-v8-16k+` |
| Exit status | 0 |
| Elapsed time | 17:43.74 |
| CPU utilization | 396% |
| Maximum resident set | 478,644 KB |
| Swap activity | 0 |
| Warning or error lines | 0 |
| Modules | 1,896 |
| BCM2712 DTBs | 8 |
| Kernel-source and custom overlays | 378 |

The complete build log has SHA-256
`23fca92e342b1ba216453ed574a5e8c7ec142c630baacf6802288378521c8722`.

## Reference-build comparison with the target

The worker-produced custom artifacts matched their installed target
counterparts:

- Provider module SHA-256:
  `11850d087b6ab3ad80998b026cdcc61aff681925a7c2f65206847a5f10ae4cac`
- KUnit module SHA-256:
  `ebd52f879292b4a68fd8dcc0b8701e42e5cb660b8f0b62ebbb945f0df6eeb991`
- Device-tree overlay SHA-256:
  `d3d42232d3bbd43b9bf376ceb130b5c3607639e57d6569131c09fc4a37b83e30`

The module hashes match after decompressing the installed `.ko.xz` files.
Both modules report vermagic
`6.18.44-v8-16k+ SMP preempt mod_unload modversions aarch64`. The worker and
target module counts are both 1,896.

The complete kernel images are not byte-identical. The worker image SHA-256 is
`26349f8ffb03351ee41375113e32469096ede676943a000b1b1644e086a9db2e`;
the installed reference image SHA-256 is
`fc174f88b5208f27b7ff3ee9ae668f545d5b647efaf6f48114d7c074d111187c`.
Both identify the same release and Debian GCC 14.2.0/binutils 2.44 toolchain,
but contain different build host and time identities.

All eight worker-built 6.18.44 BCM2712 DTBs differ from the target's retained
packaged 6.18.34 boot DTBs. The qualified target already runs the reference
kernel with those retained DTBs. The worker bundle therefore preserves the
6.18.44 DTBs for inspection without implying that they should replace the
target files.

## Staged evidence bundle

The worker retained a manifest-backed bundle under
`/home/pi/kernel-work/stage/issue399-phase6x-20260812T005812Z`:

- Archive: `issue399-phase6x-20260812T005812Z.tar.zst`
- Archive SHA-256:
  `32ec947e31fcddbe00338c837a6fafb5fbf523981b37c97f2ca22d2458dfc885`
- Manifest entries: 2,325
- Manifest SHA-256:
  `42e0d7144074e5fd035197bdf2c5ae054fc6a567b72b28f8fbdeece3f1f5506e`
- Closed stage-log SHA-256:
  `4273428ddb2dc51d0818e6e69901c0eb26d0c831c58e5ad8218519d7f9027505`

The portable archive checksum and all manifest entries verified. The bundle
contains the distinct image `kernel_2712_issue399_phase6x.img`, compressed
modules, DTBs, overlays, exact configuration, ordered patches, overlay source,
and build provenance.

The independently reconciled worker evidence report has SHA-256
`a38ce2195bbd50008ed98083beb2c6ae779500b28d4f5dc413dc3c76c304a5c1`.
The durable eight-file comparison record has SHA-256
`5f34483a82bd33882929bad90250dcc73a22c4fb4e1036b2c203af9c63406ba7`.
The completed state is also recorded in
[Issue #402](https://github.com/WsprryPi/WsprryPi/issues/402).

## Preservation and reuse boundary

The reviewed worker state was preserved in VirtualBox snapshot
`wsprmac-step12a-reviewed-final-20260812`, UUID
`83b2f603-76df-4fef-8916-b9efea0008c8`.

This result proves that `wsprmac` can reproducibly build and package a custom
Raspberry Pi ARM64 kernel from checksum-pinned source, configuration, patches,
and overlay inputs. It does not authorize deployment. Every later kernel input
set must use new immutable input, source, build, staging, and evidence
identities rather than replacing this validated reference record.
