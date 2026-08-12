# Raspberry Pi kernel build worker bootstrap research

## Purpose

WsprryPi has a validated Debian ARM64 kernel-build worker and a maintained
record of what that worker can produce. It does not yet have a single,
ordered procedure that can recreate the worker from a fresh Debian virtual
machine.

This report defines the content, evidence model, and validation required for
that missing bootstrap guide. It is research and documentation planning only.
It does not install packages, create another virtual machine, rebuild a
kernel, or alter a Raspberry Pi.

The future maintained guide should be named
`docs/raspberry-pi-kernel-build-worker-setup.md`. The existing
[Raspberry Pi kernel build worker](raspberry-pi-kernel-build-worker.md) report
should remain the capability and reference-build record; the new guide should
explain how to recreate the environment that produced those results.

## Existing evidence

The following worker-local records are authoritative for the current
environment:

- `/home/pi/kernel-work/README.md`, SHA-256
  `f107d8f11d0bfb8e1676b905e683c1e1bb4b2a7ddba13a8a2b578d1e9e106f37`,
  defines directory roles, repeat-run rules, cleanup, and safety.
- `/home/pi/kernel-work/BASELINE_SOURCE_PIN.txt`, SHA-256
  `487ef0eac2e95c9cd09d402324fc316e9a204b4e4786d1d555e4fe1d8a5a39ed`,
  records the authenticated Raspberry Pi OS stock-source provenance.
- `step5-corrected-wspr5-6.18.34-rpt-rpi-2712-olddefconfig.log` and the
  corresponding `IMPORT_RECORD.txt` record configuration import and initial
  normalization.
- `step6-stock-build-20260811T181449Z.log`, SHA-256
  `e5d35f2935e298d6cee333655c3f9ca1a961469c6314a66884390661d7a858d0`,
  records the successful stock build contract and timing.
- `step7-stage-20260811T185801Z.log` and its staged `BUILD_RECORD.txt` record
  module staging and release identity.
- The stock archive SHA-256
  `1ba4ad7c216488ff961aa20cd1eccd071c90bb03ef3acb1ae45e0698f29e18bb`
  and its 2,312-entry manifest provide the reference acceptance artifact.
- [Raspberry Pi kernel build worker](raspberry-pi-kernel-build-worker.md)
  records the later custom-kernel reproduction and proves that the workspace
  convention supports immutable development lanes as well as the stock lane.

These records are sufficient to design the guide. They are not a substitute
for testing the guide from a fresh VM.

## Validated host and guest profile

The current worker uses this allocation:

| Property | Validated value |
| --- | --- |
| Host | Apple Silicon Mac with 10 logical CPUs and 32 GiB RAM |
| Hypervisor | VirtualBox |
| Guest type | Debian 13 Trixie, ARM64 |
| Firmware | EFI |
| vCPUs | 4 |
| Guest memory | 8,192 MB |
| Virtual disk | 64 GiB, fixed-size VDI |
| Guest filesystem observed | 59 GiB with 43 GiB free after builds |
| Network observed | Bridged adapter with SSH and mDNS reachability |
| VM frontend during operation | Headless |

The future guide must label this as the validated profile, not a proven
minimum. Evidence shows that four-way clean builds saturated the four vCPUs,
used less than 0.5 GiB maximum resident memory for the timed build process,
and used no swap. The complete worker footprint, source trees, build outputs,
staging bundles, logs, and operating system require substantially more space
than the compiler's peak resident set alone suggests.

The guide should require at least the validated 64 GiB disk allocation unless
a smaller allocation is separately tested. Networking should be specified by
outcome—SSH access from the controlling host and access to the official source
repository—not by requiring bridged networking or a particular Mac interface.
mDNS is convenient but an SSH host alias or fixed address is equally valid.

## Kernel-build package set

The retained apt history shows that the VM preparation and kernel-worker
transactions explicitly installed this combined package set:

```text
bc
binutils-arm-linux-gnueabihf
bison
build-essential
ccache
debhelper
device-tree-compiler
dpkg-dev
dwarves
fakeroot
flex
gcc
gcc-arm-linux-gnueabihf
git
kmod
libc6-dev
libelf-dev
libncurses-dev
libssl-dev
make
perl
pkg-config
python3
rsync
time
xz-utils
zstd
```

The future guide should use one explicit `apt-get install` command containing
this reviewed set. It should not copy the VM's complete installed-package
inventory: Apache, PHP, Chromium, Node.js, WsprryPi application dependencies,
Codex tooling, and desktop conveniences are incidental to kernel compilation.

The guide should distinguish three groups:

1. Core kernel build and staging dependencies: all packages above except
   `ccache`, the ARM32 cross tools, and Debian packaging helpers.
2. Conditional compatibility dependencies:
   `gcc-arm-linux-gnueabihf` and `binutils-arm-linux-gnueabihf`, required when
   the imported ARM64 configuration enables the 32-bit compatibility VDSO.
3. Optional or workflow dependencies: `ccache` is installed but was not part
   of the validated build invocation; `debhelper`, `dpkg-dev`, and `fakeroot`
   support packaging investigation but were not required for the successful
   direct kernel build.

For reproducibility, the guide should record installed versions after package
installation rather than permanently pin transient Debian repository versions
in the installation command. The validated environment used GCC 14.2.0,
binutils 2.44, Make 4.4.1, device-tree compiler 1.7.2, dwarves 1.30, and zstd
1.5.7. A generated package manifest should accompany every bootstrap
validation run.

## Required guide structure

The bootstrap guide should be executable in order and contain the following
sections.

### 1. Scope and safety

State that the worker builds and stages kernels but never installs them on
itself. Target access, copying, installation, boot configuration, `depmod` on
the target, and reboot require separate authorization. The guide must not
include deployment commands as part of environment creation.

### 2. VM creation

Record the validated VirtualBox allocation and Debian ARM64 requirement.
Define observable acceptance checks for architecture, OS version, CPU count,
memory, disk, EFI boot, network reachability, SSH, and headless startup.
Host-specific VirtualBox commands may be an appendix; the normative contract
should be hypervisor-neutral where practical.

### 3. Debian preparation

Require package-index refresh, the explicit package installation, verification
that no packages were removed unexpectedly, and a captured version manifest.
Include exact checks for `gcc`, `ld`, `make`, `git`, `dtc`, `depmod`, `zstd`,
`tar`, `/usr/bin/time`, and—when needed—`arm-linux-gnueabihf-gcc`.

### 4. Workspace creation

Create `~/kernel-work/{src,build,stage,logs}` without placing generated files
in either WsprryPi repository. Reproduce the directory roles and invariants
from the worker README, but replace its obsolete issue-specific wording with a
general maintained-worker contract.

The commands must be idempotent: repeating directory creation must preserve
existing contents, while source, build, and staging operations must refuse
unreviewed collisions.

### 5. Source acquisition and pinning

Parameterize:

- official source URL;
- target Raspberry Pi OS binary and source package versions;
- Raspberry Pi source branch;
- exact commit;
- target model and architecture;
- source provenance record and checksum.

The stock acceptance example should clone
`https://github.com/raspberrypi/linux.git`, detach at
`c8c7494100e99ee05b11aaa4f0588a223a63d1af`, verify that commit, and refuse a
dirty source tree. Fetching or switching source must not be an implicit build
step.

The guide must explain that the commit came from installed Raspberry Pi OS
package metadata and changelog. It must retain the warning that downloaded
historical source archives were not used because their `.dsc` signer could
not be authenticated with the available trusted keyrings.

### 6. Configuration import and normalization

Import the configuration from the actual target kernel when available. The
stock reference source was `/boot/config-6.18.34+rpt-rpi-2712`, with SHA-256
`d5ba966d17d456a6f29e53baf53464e1fd53f9f8e31481da18f2221f1da2593d`.
The guide should support `/proc/config.gz` only when the target exposes it and
must record which source was used.

Preserve an immutable raw copy, place a working `.config` only in the
out-of-tree build directory, run `olddefconfig` against the pinned source, and
save a setting-level diff and both hashes. For the stock acceptance build, use
`CROSS_COMPILE_COMPAT=arm-linux-gnueabihf-`; the resulting normalized config
SHA-256 was
`a5dde4cec5f4b9526d8c5e55a308d94fd49d2912145891f469212f650075ac6a`.

The guide must not make the compatibility compiler unconditional for every
future input. The later exact development configuration intentionally omitted
it because enabling it changed the target configuration. The correct rule is
to compare normalization results and accept only reviewed differences.

### 7. Stock validation build

Use an out-of-tree build and a distinct worker release suffix. The validated
stock command contract was equivalent to:

```sh
make -C "$SOURCE" O="$BUILD" \
  ARCH=arm64 \
  CROSS_COMPILE_COMPAT=arm-linux-gnueabihf- \
  LOCALVERSION=-wsprmac-stock \
  -j4 Image.gz modules dtbs
```

Run it through `/usr/bin/time -v`, capture a complete timestamped log, and
record source commit, config hash, compiler versions, parallelism, calculated
release, start/end time, warning/error count, and exit status. The reference
result is build-environment evidence, not a requirement that later builds
produce the same image hash or duration.

### 8. Staging and archive construction

Run `modules_install` only with `INSTALL_MOD_PATH` beneath a new staging
bundle. Remove generated `build` and `source` links that point back into the
worker. Collect:

- kernel image;
- Broadcom DTBs;
- overlays and overlay README;
- compressed module tree and generated dependency metadata;
- raw and normalized configurations plus their diff;
- source pin;
- build log and artifact checksums;
- machine-readable build record.

Generate module metadata with `depmod -b`, then create a relative-path
`MANIFEST.sha256`. Verify every entry before creating the `.tar.zst` archive.
Create a portable `SHA256SUMS` beside the closed archive and verify it from the
directory containing the archive. Checksums for logs must be generated only
after the logging stream closes; the earlier self-hash sequencing defect is a
specific regression the guide must prevent.

### 9. Verification criteria

The procedure should fail unless all of these conditions hold:

- guest is Debian ARM64 with the declared resources;
- required tools exist and their versions are captured;
- source is detached at the expected commit and clean before patching;
- raw target configuration hash matches the recorded input;
- normalization differences are reviewed and recorded;
- calculated release is distinct from the target package ABI unless package
  reproduction is explicitly intended;
- build exits zero and produces the requested image, modules, DTBs, overlays,
  `Module.symvers`, and `modules.order`;
- staged module directory exactly matches the calculated release;
- no bundle symlink points into the worker source or build directory;
- `depmod` validation succeeds against the staged root;
- all manifest entries and the archive checksum verify;
- `/boot`, `/lib/modules`, WsprryPi repositories, and any target Pi remain
  unchanged;
- retained evidence is sufficient to repeat or audit the build.

The stock acceptance reference produced 1,895 modules, eight BCM2712 DTBs,
368 overlays, a 2,312-entry manifest, and release
`6.18.34-v8-16k-wsprmac-stock`. These counts are acceptance values for that
exact pinned input, not universal constants.

### 10. Repeat runs, cleanup, and checkpoints

Carry forward the existing collision rules and narrow cleanup guards. Define
when incremental output may be reused, when a new immutable lane is required,
and how logs and staged evidence are retained. Checkpoint names should describe
the validated boundary, but VirtualBox snapshots are supplementary recovery
state—not substitutes for the versioned guide, package manifest, source pin,
and artifact checksums.

## Parameterization model

The guide should separate stable worker variables from per-build inputs.

| Stable worker contract | Per-build contract |
| --- | --- |
| Workspace root | Target model and host |
| Native `ARCH=arm64` | Source branch and exact commit |
| Out-of-tree build policy | Raw configuration and hash |
| Staging-only module installation | Compatibility compiler decision |
| Manifest and archive rules | Patch and overlay inputs |
| Safety and collision guards | Local version and release |
| Evidence retention | Expected artifact counts |

This separation allows the same bootstrap guide to create a stock validation
worker and later immutable development lanes without rewriting the environment
setup for each kernel revision.

## Proposed supporting artifacts

The final guide should define, and a validation run should preserve:

- `worker-package-manifest.tsv`: explicitly requested packages and resolved
  versions;
- `worker-tool-versions.txt`: compiler, linker, build, DT, compression, and
  module-tool versions;
- `worker-profile.txt`: OS, architecture, CPU, memory, disk, filesystem, and
  network acceptance observations;
- `SOURCE_PIN.txt`: source URL, branch, exact commit, target package provenance,
  and update policy;
- `IMPORT_RECORD.txt`: target identity, configuration source/hash, normalization
  command, and normalized hash;
- timestamped build and staging logs with external checksum sidecars;
- staged `BUILD_RECORD.txt`, relative `MANIFEST.sha256`, and portable
  `SHA256SUMS`.

These formats may remain simple text, but field names and required values
should be specified so another worker can validate them mechanically.

## Validation plan for the future guide

The guide cannot be called complete merely because its commands were
reconstructed from the working VM. It needs a clean-room validation:

1. Create a fresh Debian 13 ARM64 VM with the documented validated allocation.
2. Follow only the new guide, without copying the existing worker filesystem or
   relying on shell history.
3. Use the pinned 6.18.34 stock acceptance input.
4. Confirm the source/config/release hashes and expected artifact counts.
5. Build and stage without writing to guest `/boot` or `/lib/modules`.
6. Verify the full manifest and archive from a second directory or host.
7. Compare the resulting structural evidence with the existing reference,
   allowing documented compiler timestamp/build-identity differences.
8. Have an independent reviewer audit the guide, transcript, package manifest,
   source pin, configuration diff, archive, and safety checks.
9. Only then promote the guide from reconstructed procedure to validated
   environment bootstrap documentation.

## Findings and decisions

- The missing document should be one ordered, maintained setup guide, not a
  collection of issue comments and VM-local logs.
- The current worker provides enough evidence to draft that guide accurately.
- The validated VM allocation and package transaction are known.
- Source acquisition, configuration normalization, build, staging, and
  verification contracts are recoverable from retained evidence.
- Package versions should be captured as evidence, while installation should
  use reviewed Debian package names rather than stale permanent version pins.
- The ARM32 compatibility compiler must be conditional on the imported
  configuration and normalization audit.
- Workspace and staging commands must be repeat-safe and collision-resistant.
- Build results, snapshots, and the capability report do not by themselves
  prove that a fresh environment can be recreated.
- Fresh-VM execution and independent review remain the gates before the future
  bootstrap guide can be described as validated.

## Documentation impact

This research adds only the specification for a future developer bootstrap
guide. It does not change operator behavior or operator documentation. The
next documentation slice is to draft
`docs/raspberry-pi-kernel-build-worker-setup.md` from this specification, then
validate it on a fresh Debian ARM64 VM before treating it as authoritative.
