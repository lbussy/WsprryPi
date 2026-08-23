# Issue 412 hardware-free qualification

> Historical record: the WsprryPi-owned route manager and qualification-plan
> interfaces described below were removed. The current package-owned socket
> contract is documented in
> `issue-412-rp1-gpclk-v1.1.1-consumer.md`.

Date: 2026-08-21

Result: **PASS**

## Candidate identity

- Branch: `codex/issue-412-rp1-gpclk-consumer-integration`
- Baseline commit: `191ce4b1c91ae6589e5728ecda0b5082d0a5ecc1`
- Exact tested source tree: `4dcb176df9c2441893cc0d8a2b90854ec7fbf388`
- Environment: Debian Trixie ARM64 container built from
  `debian:trixie-slim@sha256:3a39a0592364683e6bab97937b72cad5a8fa6dcbbee90edb3bb48c7f8e94f258`
- Qualification image: `sha256:6ceba8b34e64696c607f233b4050e4052d8a7ed48d4c8e41cddc95f66b72e7c8`
- Hardware access guard: `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1` plus the
  checked-in `strace` transmitter-access audit

The candidate was copied into a disposable clean Git snapshot. Ignored build
and dependency directories were removed before compilation. The snapshot used
the canonical `WsprryPi` origin identity so executable naming matched CI.

## Canonical suite

The complete `non-hardware-validation` command sequence from
`.github/workflows/debian-non-hardware.yml` passed:

- multi-backend debug build, capability regressions, semantics, and startup
  quiesce;
- UI unit and browser integration regressions;
- RP1 scheduler backend regression;
- support-bundle, encryption, signing, intake, retention, installer dependency,
  and DKMS dependency regressions;
- transmitter simulated-backend, real-time, controller, startup-quiesce, and
  Si5351 transition contracts;
- WSPR-Reference build, major regressions, install, and package consumer;
- deterministic simulated QRSS and complete planned WSPR runs; and
- `assert-no-transmitter-hardware.sh` over every retained `strace`, which
  reported `No prohibited transmitter hardware access observed.`

The Issue 412 qualification tail also passed:

```text
rp1_gpclk_diagnostics_test passed
RP1 GPCLK privileged route manager tests passed
RP1 GPCLK v1.0.0 Linux provider client tests passed
RP1 GPCLK application orchestration tests passed.
collect-support-bundle tests: PASS
CANONICAL_NON_HARDWARE_SUITE_PASS
```

This tail exercises the fixed query and preflight contract, every injected
boot/persistence/journal/reboot failure, automatic and failed rollback,
generation mismatch, durable restart phases, requested/persisted/configured/
active mismatch, eligibility failure, provider-query failure, and explicit
no-fallback acquisition behavior.

## Adversarial findings and repairs

Two actionable product findings were repaired before the passing run:

1. Startup reconciliation did not recover durable `preflight` or
   `rollback_completed` journal phases. Both now take the idempotent bounded
   rollback-and-verification path, with explicit restart regressions.
2. The support collector assumed `configs/systemd` existed when the installed
   service was absent, and its isolated-path fixture could create an empty
   symlink for an unavailable optional tool. Required configuration directories
   are now created up front and the fixture skips unavailable optional tools.

No actionable findings remain from this review.

## Boundary

This evidence qualifies hardware-free software behavior only. It does not
qualify installation, DKMS loading, boot overlays, reboot behavior on a target,
GPIO electrical behavior, transmitter hardware, timing on Raspberry Pi 5, RF
output, or spectral performance. No installation, privileged route operation,
boot mutation, reboot, GPIO access, transmission, or RF activity occurred.

## Production route integration addendum

The subsequent production-binding slice connected the qualified manager and
orchestrator cores to fixed boot, journal, lock, configuration, controller-idle,
reboot, startup-reconciliation, same-origin HTTP, and UI preflight/apply
boundaries. The following hardware-free checks passed:

```text
make rp1-gpclk-route-manager-test rp1-gpclk-route-orchestrator-test \
  rp1-gpclk-route-runtime-wiring-test SUDO=
node WsprryPi-UI/tests/rp1_route_ui_test.js
make -j4 debug SUDO= BACKENDS=simulated ANCILLARY_GPIO=0
git diff --check
```

Desktop and 390-pixel mobile route states were rendered in light and dark
themes and reviewed with no actionable finding. The Impeccable detector
returned no findings for the changed UI files. The ordinary physical-backend
build was unavailable on the macOS host because its Linux GPIO and I2C
development headers are intentionally absent. No installation, boot mutation,
reboot, service operation, GPIO access, transmission, or RF activity occurred.

## Qualification-safe transition addendum

The later target-validation preparation slice separated permission to stage an
exact inactive route from permission to transmit on it. A root-owned,
short-lived, identity- and generation-bound qualification plan may authorize
only the next output-inhibited route transition. Provider reconciliation and
every acquisition remain fail-closed until the provider reports the exact route
live-eligible.

Focused hardware-free checks passed for the pure authorization policy,
inactive-provider staging, manager/orchestrator lifecycle, production wiring,
diagnostic capture, and both simulated and RP1 backend builds. No qualification
plan was installed and no target, boot, service, reboot, GPIO, transmission, or
RF action occurred.
