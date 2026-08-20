# Issue #414 Slice 39 — installer integration for age

## Objective

Make Debian's `age` package an explicit WsprryPi runtime dependency and require
safe fixed `/usr/bin/age` and `/usr/bin/age-keygen` executables after package
handling. Preserve installer dry-run, failure, and uninstall behavior.

## Scope and safety

Add `age` to the existing APT dependency list. After successful package handling,
fail closed unless both fixed executable paths are regular non-symlink files,
root-owned, executable, and not group- or world-writable. Provide a separately
named rooted test seam for deterministic filesystem fixtures. Dry-run reports the
validation without inspecting or mutating the host. Uninstall does not remove the
distribution package.

Do not add PATH discovery, Homebrew, source builds, downloaded or vendored
binaries, fallback cryptography, keys, manifests, UI or runtime feature changes.
Do not run the installer, package-manager mutations, services, reboot, hardware,
GPIO, I2C, transmitter, or RF operations.

## Validation

Cover package-list retention, post-package fail-closed wiring, executable absence,
symlink and non-regular nodes, ownership, write permissions, executable mode,
valid pairs, invalid test roots, and uninstall non-removal. Run shell syntax,
focused dependency tests, relevant runtime-installer tests where portable, diff
checks, and isolated non-mutating `wspr4` validation. Review the complete diff
adversarially, correct all actionable findings, then commit and push only this
slice to the current Issue #414 integration branch.
