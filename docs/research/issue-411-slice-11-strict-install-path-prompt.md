# Issue 411 Slice 11 execution prompt: strict Si5351 install path

## Objective

Provide a supported, explicit way to build and install the strict
Si5351-only executable on ordinary Linux without invoking Raspberry Pi service
management, UI deployment, system configuration, or hardware. Preserve the
existing full Raspberry Pi installer and default build behavior unchanged.

## Verified starting point

- Slice 10 is integrated into `devel` at
  `dd041d8eb48bced8a49cdb9f8c62283fe4057a97` and pushed to GitHub.
- `BACKENDS=si5351 ANCILLARY_GPIO=0` already builds the canonical strict I2C
  executable, passes capability and file-access audits, and has no libgpiod
  dependency.
- The existing Make `install` target stops, enables, and starts a systemd
  service. It is therefore not an appropriate Ubuntu x86 candidate-install
  path.
- `scripts/install.sh` is the full Raspberry Pi product installer. It manages
  packages, configuration, web assets, services, and libgpiod and must not be
  generalized in this slice.
- `scripts/make_executables.sh` stages debug and release artifacts for the
  established Raspberry Pi workflow; it does not provide a destination-prefix
  installation contract.
- Issue 411 is open and must remain unchanged.

## Scope

1. Add a Make target that builds the selected release profile and installs only
   its executable into an explicit `PREFIX`.
2. Keep `BACKENDS` and `ANCILLARY_GPIO` as the authoritative profile inputs; do
   not introduce a second capability-selection mechanism.
3. Make the binary-only target perform no service, package, UI, configuration,
   device, or runtime action.
4. Preserve the existing `install`, `debuginstall`, `uninstall`, and full
   installer behavior.
5. Add an isolated regression proving the strict profile installs the exact
   built binary with executable permissions and reports only `si5351` with
   ancillary GPIO disabled.
6. Prove the regression cannot invoke `systemctl` and installs only beneath a
   disposable prefix.
7. Add the regression to the strict Ubuntu 24.04 CI job that intentionally
   omits libgpiod development packages.
8. Update the compile-time backend-selection research record with the exact
   candidate build/install commands and boundaries.
9. Run focused and representative non-hardware validation in isolated Linux
   environments, review the complete diff, then commit and push only the Slice
   11 branch.

## Required behavior

- The supported candidate workflow is equivalent to:

  ```sh
  cd src
  make release BACKENDS=si5351 ANCILLARY_GPIO=0 SUDO=
  make install-binary BACKENDS=si5351 ANCILLARY_GPIO=0 \
    PREFIX=/chosen/bin SUDO=
  ```

- `install-binary` depends on the release artifact for the selected profile and
  copies that exact artifact to `$(PREFIX)/wsprrypi` with mode `0755`.
- `install-binary` may create `PREFIX`, but no other destination.
- The default backend set and default `ANCILLARY_GPIO=1` remain unchanged.
- Omitted backend selection continues to fail closed with no fallback.
- The full `install` target retains its existing service lifecycle semantics.
- A user requiring privilege for a system prefix may use the existing `SUDO`
  Make variable; tests use `SUDO=` and a disposable unprivileged prefix.

## Constraints and non-goals

- Do not run a host installation target during this slice.
- Do not install or upgrade packages.
- Do not start, stop, enable, disable, or modify a service.
- Do not modify the production installer, UI deployment, configuration layout,
  systemd unit, permissions policy, or web behavior.
- Do not access I2C, GPIO, mailbox, MMIO, DMA, RP1, Si5351 hardware, or RF.
- Do not claim CP2112, adapter-driver, electrical, frequency, or RF
  qualification.
- Do not publish end-user Issue 411 instructions until this candidate is
  separately reviewed and integrated into `devel`.
- Do not open a pull request or mutate Issue 411.

## Adversarial review

Attempt to disprove that the installed file is the selected strict-profile
artifact; that profile-key isolation prevents reuse of a default/GPIO binary;
that `install-binary` cannot invoke systemd or write outside `PREFIX`; that the
strict installation has no libgpiod dependency; and that existing installation
targets and defaults remain byte-for-byte behaviorally unchanged. Correct every
actionable finding within scope and rerun affected checks.

## Validation

- Shell syntax and whitespace/error checks.
- Strict Ubuntu 24.04 build without libgpiod development packages.
- Existing strict-profile capability and non-root regressions.
- Existing strict I2C file-access audit.
- New disposable-prefix install-path regression with a failing `systemctl`
  sentinel.
- Backend-capability generator and Make integration regressions.
- Representative hardware-disabled parent semantics suite.
- Workflow YAML parse.
- Final staged and complete-diff review.

## Exit criteria

- The strict Si5351-only candidate has an explicit, tested binary-only install
  path suitable for a later Ubuntu x86 tester handoff.
- Default Raspberry Pi build and installation behavior is unchanged.
- All required non-hardware validation passes.
- No host installation, service, hardware, or RF action occurs.
- Prompt, implementation, tests, and research record are committed and pushed
  on `codex/issue-411-slice-11-strict-install-path`.
- `devel` is not advanced in this slice.
- Issue 411 remains open and otherwise unchanged.
