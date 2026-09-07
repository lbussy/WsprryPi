# Slice 5 implementation and adversarial review

Scope: parent WTP complete-plan backend, shared backend identity/qualification,
controller integration tests and developer documentation. Baseline:
`0c6f43fc1d89fc2c0cb8e215bfedecd2531659c0` on
`codex/phase10-wtp-slice1`, preserving the merged RP1 reboot recovery baseline.
The [execution prompt](wtp-slice5-prompt.md) was saved before implementation.

## Review findings and closure

The implementation was assessed against the protocol/session invariants and
actual shared-controller behavior, then revised and reassessed.

| Finding | Resolution and regression evidence |
| --- | --- |
| LOAD acknowledgment alone could leave an executable job after later adjustment validation failed. | Separate successful-configuration admission flag; direct configure/execute failure test prevents ARM before cleanup. |
| A fixed two-second completion allowance was too short for the pinned endpoint's deliberately fragmented status and advisory reconciliation. | Use the transaction budget plus two seconds after estimated job end. The actual pinned endpoint/controller completion test now passes; RF duration is unchanged. |
| A fault arriving during RELEASE could escape an earlier cleanup safety check. | Recheck fault, uncertainty, fresh status, owner, output and terminal state after RELEASE. Injected DEVICE_FAULT during RELEASE fails cleanup and blocks new work. |
| Concurrent stop could mislabel Missed as cancellation, or a repeated execute could reuse previous terminal evidence. | Cancellation requires matching Complete or Aborted evidence. Missed remains failure, and unprepared/repeated execute is rejected before cancellation handling. Completion winning an ABORT race is reported as completion. |
| The production factory test's Si5351 INI fixtures assumed Si5351 existed in a simulated-only profile. | Retain backend omission and GPIO no-op checks; run the remaining Si5351 fixtures only when compiled. Explicit WTP factory rejection is tested in the portable profile. |

Additional attacks covered missing/changed prepared identity, nonuniform per-event
adjustments, stale clock observations, leap/holdover/uncertainty limits, delayed
ARM rejection, foreign ownership and active foreign output, lost mutation replies,
missing records, device/boot replacement, lease renewal, failed ABORT, active
terminal state, repeated jobs, stopped/regressing host time and bounded waits.

After repairs, another source-level adversarial assessment found no remaining
actionable defect within this slice. It checked mutation admission, every exit
from configure/execute/cleanup, same-session reconnect, job identity retention,
monotonic arithmetic, final output evidence, component dependency boundaries and
production-selection exclusion. The frozen plan retains RF semantics and IDs;
per-event adjustment evidence is separate from the controller's uniform-shift
mechanism. No protocol, USB transport or RP1 lifecycle implementation was changed.

## Validation evidence

Local host: macOS, C++20. Commands below passed on the final implementation
unless their narrower scope is explicitly stated. Run parent commands from `src`.

- `make wtp-backend-test SUDO=`: scripted faults through real framing, Session,
  converter, compiler and controller, using virtual time only: 5,472 checks.
- `make wtp-backend-test SUDO= WTP_BACKEND_BUILD_DIR=build/wtp-backend-sanitized WTP_BACKEND_CXXFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' WTP_BACKEND_LDFLAGS='-fsanitize=address,undefined'`: same suite under ASan/UBSan.
- `make wtp-backend-interop-test SUDO= PICO_SOURCE=WTP-Client/build/pico-reference`:
  provenance-verified actual Pico endpoint/job service at
  `40812e7438f180c5e8d8ad75d4eb227271152b10`, separate translation units,
  software clock/engine. Includes component replay/reconnect interoperability
  plus parent controller completion and cancellation, with one local execution.
- `make wtp-protocol-test wtp-plan-test wtp-usb-test SUDO=`: standalone WTP
  component, conversion and USB fakes/synthetic sysfs/PTY regression coverage.
- From `src/WSPR-Transmitter/src`:
  `make simulated-backend-test transmission-controller-contract-test`.
- `make backend-profile-factory-test BACKENDS=simulated ANCILLARY_GPIO=0 SUDO=`:
  explicit WTP production rejection and existing backend selection behavior.
- `make semantics-test-portable SUDO=`: explicit simulated-only application
  semantics subset. The expected jobserver warning and negative-test diagnostics
  remain; this is not the full Linux physical-backend semantics suite.
- `make semantics-make-regression-test SUDO=`.
- From the repository root:
  `c++ --analyze -std=c++20 -Isrc -Isrc/WTP-Client/include -Xanalyzer -analyzer-output=text src/wtp_integration/backend.cpp`:
  no analyzer diagnostics.
- Final whitespace and new/changed local Markdown-link checks passed. The
  unchanged root README license link still targets missing `LICENSE.MIT.md`;
  that pre-existing documentation defect is outside the Slice 5 diff.

The initial interoperability candidate failed its completion-observation deadline;
that failure was repaired and retested. The initial simulated-only factory test
exposed its Si5351 fixture precondition and passed after the fixture correction.
Early test-fixture compilation/JSON errors were corrected before acceptance.

## Boundaries and documentation impact

Components modified: `src/WSPR-Transmitter` (identity, conservative qualification,
production factory rejection and README); `src/WTP-Client` (README only).
Implementation lives in parent `src/wtp_integration`; no SDK or parent dependency
was added to the portable client. CI adds the ordinary backend suite to existing
macOS/Linux WTP jobs. These jobs trigger on devel pushes and devel-targeted PRs,
so a feature-branch push alone is not an exact-commit CI result.

Updated developer prompt, backend contract, review record, README progress links
and USB guide. Reviewed the separate Wsprry_Pi_Docs backend and timing guidance
read-only; operator configuration/UI are unchanged. Their later follow-up is
explicit endpoint/permission selection, device UTC prerequisites, adjustments,
finite-job constraints and unresolved-output recovery. UI work remains subject
to the requested development toggle and mandatory Impeccable visual review.

No physical USB, GPIO, RF, SSH, installation, service operation, reboot or
cross-repository mutation occurred. Native Linux runtime, physical CDC/DTR,
firmware hardware behavior, process/service lifecycle and RF qualification were
not run. Early scheduler preparation, production configuration/status/UI and
release readiness remain outside this slice. No Phase 10 completion is claimed.
