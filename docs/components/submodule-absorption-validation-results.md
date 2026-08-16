# Issue 415 Hardware-Free Validation Results

Date: 2026-08-16

Branch: `codex/issue-415-submodule-absorption`

Validated commit: `cc12d8dcf44d2ba216bd63ad6f0b55619dec1962`

This record applies the
[validation matrix](submodule-absorption-validation-matrix.md) to the completed
snapshot imports and Phase B adaptations. It supplements, rather than rewrites,
the per-slice evidence in [provenance.md](provenance.md). The earlier deferred
statements in those component entries describe the state when each slice was
completed.

## Repository and export checks

- The local checkout was clean, on the branch above, synchronized with its
  upstream, and had no `.gitmodules`, registered submodules, mode `160000`
  entries, or nested `.git` administration in the ten component roots.
- All ten component roots were present as ordinary tracked trees. Their current
  subtree OIDs matched the recorded Phase B OIDs. Mailbox additionally includes
  the later behavior-neutral component-variable terminology change recorded by
  commit `f6d9bd2`.
- `git diff --check`, shell syntax checks for `scripts/install.sh` and
  `scripts/sync_all_branches.sh`, and external-bytecode Python compilation of
  `scripts/copy_ui.py` and
  `scripts/research/websocket_thread_memory_rig.py` passed.
- A Git-free tree export contained all ten component roots, no `.gitmodules`,
  and no nested `.git` administration. Its parent build was correctly deferred:
  the build requires Git-derived `build/generated/build_metadata.hpp`, which a
  Git-free archive cannot generate. A self-contained local clone at the exact
  commit supplied the checkout-equivalent build and test evidence below. This
  was not the required remote fresh-clone acceptance test.
- In a disposable repository, the pre-commit hook rejected an added merge
  conflict marker, the pre-push hook rejected `main`, and the pre-push hook
  allowed a feature branch.

## Debian hardware-free validation

The canonical Debian 13 validation ran in a local Podman container with
`WSPRRYPI_DISABLE_HARDWARE_ACCESS=1`. It passed:

- parent debug build, semantics regression, startup-quiesce regression, and RP1
  non-hardware scheduler tests;
- UI dependency installation, all six unit tests, and both Chromium browser
  integration tests;
- simulated backend, real-time simulator, transmission-controller,
  startup-quiesce, and Si5351 transition contract tests;
- standalone WSPR-Reference configure/build, major regressions, staged install,
  package-consumer build, and consumer execution;
- deterministic QRSS simulation and complete 162-event WSPR simulation; and
- `strace` audits showing no prohibited transmitter-device access.

The remaining standalone component checks also passed:

- INI-Handler, LCBLog, MonitorFile, PPM-Manager, Signal-Handler, and Singleton
  built and ran their bounded hardware-free `test` targets;
- Mailbox completed its debug/build-only gate. Its live test was not run and no
  `/dev/vcio`, `/dev/mem`, or mailbox ioctl operation occurred.

## Local macOS and target-host results

The local UI unit suite passed. Chromium was not installed on the Mac, so the
browser integrations were run in the canonical Debian environment instead.

The designated `/home/pi/WsprryPi` checkout on `wspr4` was clean and on the
correct branch, but remained at `0bb9600`. A normal `git pull --ff-only` fetched
the current remote branch and then stopped without changing the checkout
because ordinary imported files would overwrite the old initialized-submodule
working trees. No reset, clean, deinitialization, overwrite, build, test,
service, hardware, GPIO, I2C, mailbox, Si5351, or RF action was performed there.
Target-host validation therefore remains blocked pending a separately reviewed
checkout transition.

## Remaining acceptance gates

- Perform a disposable clone from the published remote, without
  `--recurse-submodules`, and repeat the supported structure/build/test checks.
- Transition `wspr4` from its pre-absorption initialized-submodule checkout by a
  separately reviewed, preservation-safe procedure, then run its approved
  compile and explicitly hardware-free tests.
- Review and update the separate `Wsprry_Pi_Docs` repository under an explicit
  cross-repository authorization.

No installation, deployment, service operation, reboot, GPIO, MMIO, mailbox
device, I2C, Si5351, physical transmitter, or RF validation was performed.
Software-only results do not qualify those boundaries.
