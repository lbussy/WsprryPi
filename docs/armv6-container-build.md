# Reproducible ARMv6 container builds

These containerized builds produce separately labelled 32-bit hard-float ARMv6
WsprryPi binaries for Raspberry Pi OS Bullseye and Bookworm on Raspberry Pi 1,
Pi Zero, and Pi Zero W. Docker can run the builds on macOS, Linux, or Windows
when its engine supports `linux/arm/v6` emulation.

Bullseye is the lower userspace baseline of the two artifacts. A binary built
against Bookworm or Trixie can require symbols or shared-library versions that
Bullseye does not provide. Building against Bullseye gives that artifact the
best chance of running on newer Raspberry Pi OS releases, but compatibility is
not assumed: Bookworm and Trixie must each pass a separate runtime dependency
and application validation.

The currently validated Bullseye build is not a single cross-release artifact.
It dynamically requires `libcrypto.so.1.1`, `libgpiodcxx.so.1`, and
`libgpiod.so.2`. Stock Bookworm provides OpenSSL 3 as `libcrypto.so.3` instead;
Trixie also moves libgpiod to a newer shared-library generation. A separately
linked artifact or an explicitly designed portable dependency policy is
required for those releases.

## Build

From the repository root, select the target Raspberry Pi OS release:

```sh
./scripts/build-armv6-bullseye-container.sh
./scripts/build-armv6-bookworm-container.sh
```

The scripts export files to `dist/armv6-bullseye` and
`dist/armv6-bookworm`, respectively. Pass a different empty directory as the
first argument to change the destination. Each script refuses to merge a new
result into a non-empty directory so stale files cannot be mistaken for current
output.

Each completed build environment is loaded into the local Docker image store as
`wsprrypi-build:armv6-bullseye` or `wsprrypi-build:armv6-bookworm`. These stable
names are visible in Docker Desktop and preserve the source snapshot and
compiled workspace used for each exported artifact. The export step creates a
stopped temporary container from the selected image, copies `/artifact`, and
removes the temporary container; it does not compile the source a second time.

The exported directory contains:

- `wsprrypi`: the dynamically linked ARMv6 executable
- `SHA256SUMS`: the executable digest
- `file.txt`: executable format identification
- `elf-header.txt`: ELF class, machine, and ABI metadata
- `elf-attributes.txt`: ARM architecture and floating-point attributes
- `shared-libraries.txt`: resolved build-environment shared libraries
- `build-packages.txt`: compiler and direct library package versions

Each Dockerfile pins its ARMv6 base image by digest. Package installation uses
the release repositories configured in that image. Rebuilding later may select
newer package revisions unless the package repository is also snapshotted.
`build-packages.txt` records the exact revisions used for a particular artifact.

## Compatibility boundary

Container compilation proves that the source builds for a 32-bit ARMv6
Raspbian userspace. It does not prove that the executable starts on Bullseye,
Bookworm, or Trixie, and it does not qualify installation, services, GPIO,
timing, frequency accuracy, transmitter hardware, or RF output.

Before distributing one binary for multiple releases, inspect its required ELF
symbol versions and shared-library names, then run non-transmitting startup and
application checks on each release. If a required library ABI differs, produce
a separately labelled artifact for that release.
