# WTP production integration review

Implemented the [execution prompt](wtp-production-integration-prompt.md) on
`codex/phase10-wtp-slice1`, retaining the Slice 7 and RP1 reboot-recovery baseline.
The [current integration guide](../wtp-production-integration.md) describes the
implemented behavior and acceptance boundary.

## Scope and result

Production JSON/INI and CLI backend selection now reach a parent-owned WTP
worker, complete-job scheduler and verified USB selection path. The runtime
bridge routes preparation, cancellation, reload, shutdown and completion through
that owner. Read-only HTTP observations and explicit same-session reconciliation
are available through the existing protected application server and proxy.

The first-party UI adds endpoint settings and status/recovery behind **Show Pico
development controls**, default off and browser-local. Visibility is separate
from persisted selection and transmission authority. Hiding controls preserves
selection and drafts. No firmware or reusable transmitter/client code changed;
`src/WTP-Client/README.md` changed only to reflect parent integration. UI source,
tests and `WsprryPi-UI/DESIGN.md` are the component changes.

## Adversarial findings and closure

| Finding | Repair and confirming evidence |
| --- | --- |
| Existing backend whitelists could silently normalize Pico to GPIO. | Preserve WTP across configuration population, payload construction and Operations selection; real browser verifies saved payload after hide/show. |
| Visibility changes could enter generic autosave. | Exclude the development switch; unit and browser tests prove no configuration write, no recovery POST and retained invalid draft. |
| Missing status resembled explicit unselected state. | Distinguish null/failed requests from `selected:false`; HTTP 503 disables recovery and reports unknown; restoration is exercised. |
| Outline recovery action had inadequate dark-theme contrast. | Filled primary action and visible keyboard focus; desktop/mobile dark, hover and focus recaptured; the same Impeccable reviewer scored both UI fixes resolved. |
| Legacy amplifier normalization could silently remove an incompatible enabled request. | Reject enabled amplifier control before negative-pin normalization; failed patch leaves configuration unchanged. |
| Legacy start/reload/fault-reset routes could bypass WTP ownership. | Parent bridge dispatch, no local RF-start callback, main-thread completion, atomic pending invalidation and blocked generic resets; injected parent test exercises these boundaries. |
| Uncertain ARM must prevent replacement, clearing and replay. | Stop retains the latch; selection replacement is refused; explicit same-session reconciliation proves inactive output while retaining the historical blocked report. Exactly two distinct jobs produce two ARM operations. |
| Lost-session reconnect could erase identity/protocol fault meaning. | Reject latched identity/protocol faults before disconnect and reconnect; preserve session and tracked-job history for recoverable uncertainty. |
| Fixed preparation lead could ignore negotiated CAPS. | Calculate early dispatch from device minimum ARM lead plus preparation budget; WSPR frame and timing tests and existing scheduler deadline tests pass. |
| Source regression assertions still expected two backend choices. | Update the precise assertions for WTP inclusion and gated controls; portable UI/source and existing GPIO/Si5351 browser tests pass. |
| Formatter include ordering exposed a parent header dependency. | Include transmission request definitions explicitly before legacy state types; rebuilt actual application and portable suite pass. |
| The threaded test fixture incremented an unsynchronized assertion counter. | Use an atomic counter and a bounded sleeping completion wait; ThreadSanitizer rerun passes without warnings. |
| CLI failure reporting substituted an unrelated empty derived frequency list. | Preserve the actual WTP validation error; rebuilt executable rejects the incomplete endpoint with its specific diagnostic. |

The final assessment revisited configuration preservation, complete finite-job
admission, callback ownership, pending/committed reload boundaries, uncertain
cleanup, same-session recovery, HTTP write admission, status age/unknown/history,
UI persistence and physical-access exclusions. No known actionable finding
remains within this software integration scope. This is not a claim of physical
target or release readiness.

## Validation

Commands ran on macOS with Apple Clang. Unless shown otherwise, Make targets
ran from `src` with `SUDO=`. Loopback application/browser fixtures were permitted;
physical hardware access was not used.

| Command/check | Final result |
| --- | --- |
| `make semantics-test-portable SUDO= -j4` | Passed the explicit portable subset, including WebSocket/HTTP lifecycle, UI/source and 23 UI-publication tests. Full Linux semantics were not run. |
| `make debug wtp-production-test BACKENDS=simulated ANCILLARY_GPIO=0 SUDO= -j4` | Actual debug application built; 1981 injected parent configuration/runtime checks and WTP UI tests passed. |
| `make semantics-make-regression-test privileged-network-policy-test backend-http-guard-test BACKENDS=simulated ANCILLARY_GPIO=0 SUDO= -j4` | Passed. |
| `make wtp-protocol-test wtp-plan-test wtp-usb-test wtp-backend-test wtp-scheduler-test wtp-status-test wtp-application-test SUDO=` | Passed: 10373 framing/wire, 946 codec, 2924 session, 1033 plan, 1937 reference vectors, 2644 USB, 5472 backend, 50700 scheduler, 20959 status and 38891 application checks. |
| `make wtp-scheduler-interop-test SUDO= PICO_SOURCE=WTP-Client/build/pico-reference` | Passed against firmware reference `40812e7438f180c5e8d8ad75d4eb227271152b10`; pinned source verification and endpoint/backend/scheduler interop passed. |
| `make wtp-application-test SUDO= WTP_APPLICATION_BUILD_DIR=build/wtp-application-asan WTP_APPLICATION_CXXFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' WTP_APPLICATION_LDFLAGS='-fsanitize=address,undefined'` | Passed 38891 checks, no sanitizer findings. |
| Same application target with `build/wtp-application-tsan` and `-fsanitize=thread` in both flags | Passed 38891 checks, no sanitizer warnings after fixture repair. |
| `make -C src/WSPR-Transmitter/src simulated-backend-test transmission-controller-contract-test SUDO=` from repository root | Both standalone component targets passed. |
| `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1 scripts/tests/backend_capability_reporting_test.sh src/build/bin/wsprrypi_debug simulated gpio disabled` from repository root | Passed: `simulated,wtp`, omitted-backend rejection and ancillary-GPIO reporting. The inspected executable is the new debug output, not the older release binary. |
| `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1 src/build/bin/wsprrypi_debug --backend wtp AA0NT EM18 20 20m` from repository root | Expected failure for missing explicit endpoint, before hardware access. |
| Node tests under `WsprryPi-UI/tests`: `wtp_ui_test`, `rp1_route_ui_test`, `network_safety_ui_test`, `gpio_correction_provenance_test`, `responsive_shell_logs_test`, `cw_timing_state_test` | All passed. |
| `node WsprryPi-UI/tests/conditional_transmit_gpio_integration_test.js` with bundled `ws` dependency | Passed existing actual-browser GPIO/Si5351 workflows. |
| Local PHP/Chrome WTP fixture | Default-off toggle, retained payload/draft, no toggle write, unknown output, one explicit recovery POST, failed/restored GET, four viewport/theme states and no page errors/overflow passed. |
| PHP syntax, installer shell syntax, new relative documentation paths and `git diff --check` | Passed. Installer was not executed. |

The focused backend/scheduler/status/application targets were rerun after the
shared test-counter repair. Compiler/API checks and portable tests emitted
expected non-Pi metadata messages and intentional fault-injection errors; they
did not exercise physical hardware. An initial ThreadSanitizer run hit the test
wait deadline, then exposed the fixture race; neither was treated as a pass.
The wider Markdown scan also found the pre-existing README license link to
`LICENSE.MIT.md`; it is unchanged and outside this integration's documentation
edits. All new relative documentation links resolve.

## UI evidence

Impeccable extended the incumbent Bench Instrument design, with fresh reviewer
and documenter handoffs. Desktop 1440×1100 and mobile 390×844 were reviewed in
light and dark themes, including recovery hover and keyboard focus. The detector
ran once; its twelve advisories belonged to unchanged incumbent CSS/JS, not the
new controls. The reviewer accepted the repaired findings. DESIGN.md records
only durable existing patterns.

Local screenshots, fixture script, results and logs are retained outside the
checkout at `/private/tmp/wsprrypi-wtp-production-evidence-20260907`. They are
development evidence, not distributed UI assets or installed-target evidence.

## Documentation Impact

- Updated: root README, production integration guide, USB/backend/scheduler/
  status/conversion contracts, WTP-Client README, UI DESIGN.md, execution prompt
  and this review.
- Considered but unchanged: the separate `Wsprry_Pi_Docs` repository and its
  instructions; no cross-repository write was authorized.
- Still required there: `docs/Command_Line_Operations/transmitter_backends.md`,
  `docs/Advanced_Operations/ini_configuration/transmitter_backends.md` and
  `docs/User_Interface/Setup/Transmitter/index.md` need WTP selection, endpoint
  fields, independent clock prerequisites, temporary visibility behavior and
  explicit recovery guidance. Operations guidance must identify unsupported
  continuous Tone and unknown-output handling before operator publication.

No physical USB, installation, service lifecycle, target reboot, firmware,
RP2350 timing, GPIO/electrical output or RF qualification was performed. Returning
to WsprryPico is appropriate for independently scoped firmware/clock prerequisites
and joint target acceptance. No release or merge is part of this task.
