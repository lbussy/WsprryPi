# Phase 10 Slice 7 implementation and adversarial review

Implemented on `codex/phase10-wtp-slice1`, beginning at
`f18c72f9a3cf4b773f7793032fc1503813813e5a`. The initial tree was clean and
matched origin. Fresh fetch confirmed the branch and `origin/devel` at RP1
reboot-recovery merge `a5239049f01bd8741e2f101b71d46e81a15ca28a`.
The [saved execution prompt](wtp-slice7-prompt.md) preceded implementation.

## Implemented scope

Parent `src/wtp_integration` now publishes owned, mutex-protected status copies
from the actual scheduler/Session/backend/controller path, including while run()
blocks. Snapshots separate local scheduling/ARM handoff from remote job state,
unknown output from inactive observations, and current observations from retained
execution/recovery reports. They expose publication/accepted-STATUS monotonic
times, identity, CAPS, ownership/lease observations, uncertainty/fault flags,
per-job adjustments and diagnostics. Parent JSON serialization preserves exact
64-bit decimal strings and explicit null/false distinctions.

Explicit same-session recovery retains the original failed execution report and
records its own request/job/device/boot identity and result. Reconnect alone does
not clear Blocked; missing history, foreign ownership, changed identities and
latched faults still prohibit unsafe continuation. No generic reset, replay,
reload or rearm was added. Production CLI/INI selection, websocket/UI wiring and
physical backends remain unchanged. This completes the development runtime's
status/recovery boundary, not production activation or Phase 10.

## Adversarial assessment and repairs

The first review examined identity retention during disconnect, stale observations,
publication concurrency, cross-job history, pending cancellation, terminal output
contradictions, lost mutation responses and the explicit recovery gate.

1. A first candidate attached frequency adjustments only while the backend held
   a prepared plan and copied them to reports only when current terminal evidence
   existed. That lost useful accepted LOAD evidence after cleanup or a disconnected
   RELEASE. Adjustments now have an explicit job identity independent of prepared
   state, survive cleanup/disconnect, and cannot appear on a new pending job.
2. Recovery history initially depended on its optional job evidence for identity.
   Missing evidence could leave an older recovery attempt unbound once a newer
   execution replaced the last report. Recovery and execution reports now retain
   verified device/boot plus request/job identity independently of output evidence.
3. Successful explicit cleanup retained an earlier backend failure diagnostic.
   Successful cleanup now clears that diagnostic; failed execution reports keep
   their original errors, and failed recovery updates the current diagnostic.

Deterministic tests cover all three repairs, including disconnected RELEASE,
a subsequent job, retained prior recovery identity, and successful recovery with
an unchanged original Blocked report. A further adversarial case places a foreign
running job beside this client's completed terminal record: job output is false,
device output is true, completion is not established, and no foreign ABORT occurs.

The second review rechecked the final changes, mutex ownership and publication
boundaries, all five lost mutation acknowledgements (CLAIM/LOAD/ARM/ABORT/RELEASE),
changed device/boot, missing terminal history, foreign startup/ownership, fault
persistence after an inactive label, cancellation/missed starts, exact serialization,
component isolation and unchanged production selection. No actionable Slice 7
finding remained. ThreadSanitizer also completed with no race findings.

Test-development corrections were kept separate from implementation findings:
an initial armed-state assertion incorrectly compared an old received observation
to current device time; a callback-only sampler missed same-poll transient states;
and an adjustment fixture initially omitted the required explicit adjustment
permission. Those fixtures were corrected to exercise their intended contracts.
The first compile also required explicit defaults on appended aggregate members
under the repository's warnings-as-errors policy.

## Validation

Commands ran from `src` unless noted. All are hardware-free; logs are outside the
checkout under `/tmp/wtp-slice7-*.log`.

| Command | Result |
| --- | --- |
| `make wtp-status-test SUDO=` | Passed: 20,959 checks, real wire/Session/backend/controller/scheduler and concurrent snapshot reader |
| `make wtp-status-test SUDO= WTP_STATUS_BUILD_DIR=build/wtp-status-sanitized WTP_STATUS_CXXFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' WTP_STATUS_LDFLAGS='-fsanitize=address,undefined'` | Passed: 20,959 checks; no sanitizer findings |
| `make wtp-status-test SUDO= WTP_STATUS_BUILD_DIR=build/wtp-status-tsan WTP_STATUS_CXXFLAGS='-O1 -g -fsanitize=thread' WTP_STATUS_LDFLAGS='-fsanitize=thread'` | Passed: 20,959 checks; no thread sanitizer findings |
| `make wtp-protocol-test wtp-plan-test wtp-usb-test wtp-backend-test wtp-scheduler-test SUDO=` | Passed: 10,373 protocol, 946 independent codec, one framing vector, three pin checks, 2,924 Session, 1,033 converter, 1,937 independent frequency, 2,644 USB fake/sysfs/PTY, 5,472 backend and 50,700 scheduler checks |
| `make wtp-backend-test wtp-scheduler-test SUDO=` | Passed again after reporting repairs: 5,472 and 50,700 checks |
| `make wtp-scheduler-interop-test SUDO= PICO_SOURCE=WTP-Client/build/pico-reference` | Passed component, backend/controller and extended scheduler interop, including final status/report evidence |
| `make simulated-backend-test transmission-controller-contract-test` from `src/WSPR-Transmitter/src` | Both passed |
| `make semantics-test-portable SUDO=` | Passed explicit macOS simulated-only subset; expected recursive Make jobserver warning |
| `make semantics-make-regression-test SUDO=` | Passed |
| `c++ --analyze -std=c++20 -Isrc -Isrc/WTP-Client/include -Xanalyzer -analyzer-output=text src/wtp_integration/scheduler.cpp src/wtp_integration/backend.cpp src/wtp_integration/status.cpp` from repo root | Passed, no diagnostics |

Pico interoperability verified all 14 pinned source files at
`40812e7438f180c5e8d8ad75d4eb227271152b10` before building the existing software
endpoint/job-service bridge. No independent repository was modified. Two scheduled
jobs completed and one waiting request was invalidated; reports were checked
against endpoint behavior. This uses a software clock/engine, not target firmware,
USB control, or measured RF.

The macOS and Linux CI workflow now includes `wtp-status-test`. Full Linux
physical-profile semantics were not run on this macOS host. The portable subset
excludes backend-specific runtime-semantics and cleanup-lifecycle executables
that require physical backend capabilities. Feature-branch push
alone does not trigger the existing devel/PR CI workflow; no exact-commit CI result
is claimed. No physical USB, GPIO, RF, SSH, installation, services, device clock
provisioning or reboot occurred. UI source and workflows did not change, so
Impeccable rendering is not applicable to this slice.

## Documentation Impact and component boundary

Updated this review, the execution prompt, `docs/wtp-status-recovery.md`, scheduling
contract and root/component progress links. The modified component paths are
`src/WTP-Client/README.md` and `src/WSPR-Transmitter/README.md` only; component
implementations/interfaces remain unchanged. Their standalone client and shared
controller/simulator tests passed. Parent implementation/test/build/CI changes are
separate from those README-only component diffs.

Reviewed Wsprry_Pi_Docs' `docs/Command_Line_Operations/transmitter_backends.md`,
`docs/Advanced_Operations/ini_configuration/transmitter_backends.md`,
`docs/Advanced_Operations/timing_calibration.md` and
`docs/User_Interface/Setup/Transmitter/index.md` read-only. No current operator
selection or UI behavior changed, so those pages remain unchanged. Separately
authorized follow-up must document selected endpoint/permissions, independent
clock prerequisites, early finite jobs, armed versus running, observation age,
output unknown, retained original failures, explicit recovery and process-restart
limits. The temporary UI toggle belongs in future developer guidance.

Next unfinished work: production endpoint/clock/configuration and runtime wiring,
operator status/recovery workflow, websocket/UI integration with the requested
temporary development toggle, then separately authorized target USB/functional
validation. No persistence across process restart, broad RF qualification or
release readiness is established here.
