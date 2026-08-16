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

The designated `/home/pi/WsprryPi` checkout on `wspr4` was initially clean and
on the correct branch at `0bb9600`. A normal `git pull --ff-only` fetched the
current remote branch and then stopped without changing the checkout because
ordinary imported files would overwrite the old initialized-submodule working
trees.

On 2026-08-16, the complete legacy checkout was preserved as
`/home/pi/WsprryPi.issue415-pre-absorption-20260816` and a normal clone of the
published Issue 415 branch was created at `/home/pi/WsprryPi`, without
`--recurse-submodules`. The new checkout was clean and synchronized at
`73ceaac3395b90ccf4189519c8f213846884aa00`. All ten ordinary subtree OIDs
matched the local checkout, and no `.gitmodules`, registered submodule,
mode-`160000` entry, or nested component Git administration remained.

With `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1`, wspr4 passed the parent debug build,
semantics and startup-quiesce regressions, RP1 non-hardware regression, UI unit
and Chromium browser integrations, transmitter component contract tests,
WSPR-Reference build and major regressions, all bounded standalone component
tests, and Mailbox's compile-only gate. The application-level simulator scripts
stopped before execution because their fixed singleton UDP port `1234` was
already occupied. No process or service was stopped or modified to free it;
those scripts had already passed in both isolated Debian runs.

## Published-remote clone acceptance

The post-commit acceptance gate passed on 2026-08-16. A new disposable clone of
the published `codex/issue-415-submodule-absorption` branch was created from
`https://github.com/WsprryPi/WsprryPi.git` without
`--recurse-submodules`. Its checked-out commit was
`2fd0999ceb3a07f3bd380a11372f9d9e343462b8`.

The clone contained all ten ordinary component trees with subtree OIDs equal to
the source checkout. It contained no `.gitmodules`, registered submodules,
mode-`160000` entries, or nested component Git administration. The complete
Debian hardware-free suite described above was repeated from that clone and
passed, including UI browser integration, component checks, WSPR-Reference
package consumption, deterministic simulation, and the prohibited-device
`strace` audit. The clone remained clean apart from ignored build products.

This closes the post-commit remote fresh-clone gate. It does not close the
separate `wspr4`, operator-documentation, hardware, service, installation, or RF
qualification gates.

## Remaining acceptance gates

- Review and update the separate `Wsprry_Pi_Docs` repository under an explicit
  cross-repository authorization.
- Optionally repeat the application-level simulator scripts on wspr4 when port
  `1234` is available through a separately authorized operational window. This
  target-specific limitation does not replace the two passing isolated Debian
  simulator and prohibited-device audits.

No installation, deployment, service operation, reboot, GPIO, MMIO, mailbox
device, I2C, Si5351, physical transmitter, or RF validation was performed.
Software-only results do not qualify those boundaries.
