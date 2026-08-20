# Issue #414 Slice 39 — installer integration for age

## Outcome

The standard installer now includes Debian's `age` package in its required APT
set. After package handling succeeds, installation fails closed unless the fixed
production executables `/usr/bin/age` and `/usr/bin/age-keygen` are regular,
non-symlink, root-owned, executable, and not group- or world-writable.

Dry-run reports that the safety validation would occur without inspecting or
changing the host. Uninstall retains the distribution-managed package. No PATH
fallback, alternate binary, key, identity, manifest, or provider behavior was
added.

## Validation

The focused dependency fixture covers valid executables, missing tools, symlinks,
non-regular paths, wrong ownership, unsafe write permissions, missing execution
permission, and invalid roots. The installer source regression retains `age` in
the package list and the fail-closed post-package call while prohibiting explicit
package removal during uninstall.

Local shell syntax and focused Make targets passed. The existing support-bundle
runtime-installer test remains non-portable on macOS because its pre-existing
fixture uses GNU `stat -c`; this slice does not alter that test or runtime
provisioner. The age round-trip passed locally with Homebrew age 1.3.1.

An isolated `wspr4` Debian snapshot passed the focused dependency tests, the
runtime-installer fixture, and the real age round-trip with distribution age
1.2.1. The production fixed-path safety check accepted the installed root-owned
`0755` regular executables and `dpkg-query` reported `age` installed. The expected
build-metadata warning occurred because the isolated snapshot intentionally had
no `.git` directory. The snapshot was removed, and `/home/pi/WsprryPi` remained
clean on `devel`. No package-manager mutation, application installation, service,
reboot, hardware, GPIO, I2C, transmitter, or RF operation occurred.

## Documentation impact

The private-intake plan now identifies Slice 39 and distinguishes implemented
installer support from pending production qualification. The separate operator
documentation repository was reviewed but remains unchanged because
cross-repository modification was not authorized. Its support-bundle guide still
requires a later end-user workflow update, and a maintainer runbook remains due.

## Remaining boundary

This slice does not install or upgrade a host. Production installation and the
complete signed-intake, encryption, signed-out Dropbox, receipt, inspection, and
promotion exercise remain the next qualification gate.
