# RP1 installation route restoration report

Implemented on `devel`, based on `37d6dadeb96a26dc84ae88445e3b56e3dda94d28`.
The approved brief is [the implementation prompt](rp1-install-route-restoration-prompt.md).

## Result

Setup first establishes the existing neutral DKMS runtime, then invokes an
application-owned restoration step for an explicit saved `rp1-gpclk` GPIO4 or
GPIO20 selection. Restoration uses the installed provider's public, digest-bound
route transaction. Success requires matching requested/configured/persisted/active
routes, the owned binding and artifacts, quiescent clock/GPIO/DMA, no provider
owner or lease, disabled output, and unchanged configuration and service state.
Refusal propagates to setup with retained failure diagnostics.

Completed reboot checkpoints no longer carry across a replaced binding.
Installer restoration inspects current state even on retry; its completed
checkpoint is written only after final verification. The installer waits up to
30 seconds for the shared reconciliation lock. A startup worker launched during
an existing restoration reports that reconciliation is already in progress.

The setup form now retains an unavailable Si5351 address as a disabled
`not detected` option, including during discovery. The payload uses the native
select value because jQuery excludes disabled options. Unrelated RP1 updates
therefore retain the saved address. Si5351 validation still requires a detected
address; there is no automatic replacement with another device.

The only reusable component modified is `WsprryPi-UI`. Other changes are parent
installer/reconciliation code, parent tests and documentation. No DKMS-provider
or reusable `src/` component implementation was modified.

## Calibration investigation

Read-only wspr5 evidence showed Si5351 `[Calibration] PPM = 0.0` and GPIO
`Manual PPM = 5e-324`, with the latter also returned by `/config`. The first
retained nonzero startup log is September 8 at 07:52:33 CDT, after setup began
at 07:50; earlier startup logs show zero. The available migration backup already
contains the subnormal value, while the stock INI contains zero. This bounds
the observed transition but does not identify the writer or prove causation.

No path was found connecting absent Si5351 discovery to a nonzero PPM assignment.
The reproduced discovery defect concerns the address selector. Added browser
checks cover blank/zero and custom PPM payloads; parent tests use exact equality
for initialized zero and INI-to-runtime-to-JSON calibration round trips. All
pass. No calibration clamping, formatting workaround or live configuration
mutation was introduced. The user reported resetting the value to zero; the
last read still showed the GPIO value above, so a rejected form save remains a
possible explanation for an unsaved reset, not a confirmed cause of its origin.

## Validation

Commands from `src` unless otherwise noted:

- `make rp1-gpclk-dkms-installer-test rp1-gpclk-route-service-test SUDO=`:
  passed; 123 installer tests, C++ route-service tests, 9 route-companion tests
  and 20 reconciliation tests. Coverage includes both routes, other backends,
  drift/refusal, stale binding, stopped/masked service, ownership, plan/final-state
  mismatches, interrupted retries, lock contention and shell failure propagation.
- `make calibration-roundtrip-test i2c-bus-selection-test BACKENDS=simulated ANCILLARY_GPIO=0 SUDO=`:
  passed. The WTP candidate uses inert fixture identities solely for configuration
  validation in the portable build; no endpoint is opened.
- `make semantics-test-portable SUDO=`: passed, including the new calibration
  target. This is the explicit portable subset, not full Linux physical-backend
  semantics coverage. It emitted the existing Make jobserver warning and expected
  missing-device-tree/negative-test diagnostics on macOS.
- `npm test` from `WsprryPi-UI`: passed.
- `WSPRRYPI_CONDITIONAL_GPIO_SCREENSHOT_DIR=/tmp/rp1-route-review node tests/conditional_transmit_gpio_integration_test.js`
  from `WsprryPi-UI`: passed; isolated PHP/Chromium fixtures, 32 GPIO matrix cases,
  address/discovery regression and calibration checks. The new address regression
  failed against the original UI before repair.
- Impeccable review: inspected desktop 1440-pixel light and mobile 390-pixel dark
  renders, including unavailable-address feedback. The saved value, disabled and
  invalid states, nearby explanation and responsive stacking remain clear.
- `bash -n scripts/install.sh`, `shellcheck -x scripts/install.sh`, Python
  compilation of the changed scripts/tests and `git diff --check`: passed.

Initial loopback fixture runs were blocked by the local sandbox and passed when
rerun with approved local execution. Intermediate failures exposed the disabled
option serialization issue and stale test fixtures; final commands above passed.

## Adversarial assessment

First assessment findings and closure:

1. A retained disabled option was still omitted by jQuery `.val()`. Payload
   serialization now reads the native value; the browser regression passes.
2. Route restoration restarts the application while holding reconciliation
   ownership. Startup-worker contention now defers cleanly; installer waiting is
   bounded and lock tests cover deferral, timeout and successful retry.
3. A completed checkpoint from a previous binding could suppress current work.
   Checkpoint reuse now requires the current binding; replacement/retry tests pass.
4. Final route claims needed explicit quiescence and disabled-output evidence.
   Verification and negative fixtures now require those observations.
5. The portable suite still expected the removed `#wtp_visible` selector in an
   unrelated source assertion. Updated that assertion to the current committed
   UI; mode-control exclusions remain checked, and the portable suite passes.

A fresh second assessment of production changes, provider-plan compatibility,
failure propagation, tests and documentation found no remaining actionable
issue in the changed scope. Live installation and hardware qualification remain
separate outstanding validation, not a passed review claim.

## Documentation Impact

- Updated `docs/rp1-gpclk-dkms-installation.md`, the implementation prompt and
  this report.
- Reviewed sibling `Wsprry_Pi_Docs` read-only. Its
  `docs/User_Interface/Setup/Transmitter/index.md` needs the GPIO installation
  paragraph changed to distinguish restoration of saved RP1 intent from neutral
  first setup, and its I2C Address section changed to describe the retained
  unavailable value. Review `docs/Advanced_Operations/rp1_gpclk.md` alongside it.
- Cross-repository edits were not authorized; that operator-documentation
  follow-up remains outstanding. No operator screenshots were replaced.
- PPM operational semantics are unchanged; no calibration-documentation change
  is required for these regression tests.

No live installation, service/module/GPIO mutation, reboot or RF operation was
performed. wspr5 still requires a separately authorized installation and idle
route check to qualify this change on that host.
