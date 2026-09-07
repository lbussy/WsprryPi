# Slice 6 implementation and adversarial review

Baseline: `72f882aba8411f78e61f86a717cfb279292d2303` on
`codex/phase10-wtp-slice1`, retaining the RP1 recovery merge
`a5239049f01bd8741e2f101b71d46e81a15ca28a`. The
[execution prompt](wtp-slice6-prompt.md) was saved before implementation.

## Implemented scope

The dedicated parent WTP scheduler accepts the real shared TransmissionRequest
and ScheduledSlot, freezes the request, checks signed UTC arithmetic, computes
an early dispatch window and runs the compiler/controller/backend lifecycle.
A final backend admission callback checks the host clock and remaining submission
allowance immediately before requesting ARM, after status reconciliation. It
supplements rather than replaces device clock admission. Pending reload and
preparation compete atomically; a committed request is immutable. Failed cleanup
latches Blocked until explicit same-session reconnection and successful recovery.

The legacy scheduler, production factory, persisted configuration, UI and RP1
runtime recovery are unchanged. Production status/recovery and configuration
selection must later dispatch through this dedicated scheduler. There is no new
shell-facing timing control or implicit hardware selection.

## Adversarial findings and repairs

| Finding | Repair and regression evidence |
| --- | --- |
| Reload invalidation could arrive between the waiting flag check and preparation, allowing invalidated work to commit. | Both transitions compete on the atomic scheduler phase. An actual notification thread fires during clock observation after the flag check and before commit; no CLAIM/LOAD occurs when invalidation wins. |
| A rejected extra submission could leave a stale error on a later successful run. | Clear run-local diagnostics when accepting a runnable pending request. Frozen-request test rejects an overlapping submission, then verifies successful execution with an empty error. |
| The shared WSPR compiler selects one frame from a multi-frame payload; accepting such a payload here would silently omit work. | Require one explicitly extracted frame per scheduled request before compilation/mutation. Full 162-event WSPR execution preserves the one-second offset and 110591999892 ns duration; multi-frame submission is rejected without an additional LOAD. |
| A stop during preparation could be reported as generic failure even when owned cleanup succeeded before ARM. | Map that bounded preparation cancellation explicitly while retaining Blocked if cleanup fails. Stop tests cover waiting, CLAIM, LOAD and running boundaries. |

The initial preparation-overrun fixture advanced time beyond Session's five-second
idle timeout. Its correct outcome was Blocked, not an ordinary clean failure.
Tests now separately cover a shorter overrun rejected by the final ARM reserve
check and the longer transport-observation timeout that preserves blocked state.

After these repairs, another adversarial assessment reviewed all scheduler exits,
backend callback placement, clock conversion/overflow, immutable request binding,
thread notification boundaries, duplicate job IDs, cleanup outcomes, explicit
recovery, component isolation and production exclusion. No actionable Slice 6
finding remained. Tests include host UTC forward/backward changes before and
during preparation, unavailable UTC, host/device clock disagreement, late wakeup,
stalled/regressing monotonic time, post-handoff host clock change, whole WSPR
jobs/renewal, repeated fresh jobs, pending/deferred reload, unknown ARM and changed
boot recovery rejection. No host timing or close is accepted as RF-off evidence.

## Validation

Local host: macOS. Parent commands run from `src` unless otherwise stated.

- `make wtp-scheduler-test SUDO=`: **50,700 checks passed**, using injected host
  and device clocks, scripted peer, actual framing/Session/backend/compiler/controller,
  and concurrent stop/reload notifications. No physical devices were opened.
- `make wtp-scheduler-test SUDO= WTP_SCHEDULER_BUILD_DIR=build/wtp-scheduler-sanitized WTP_SCHEDULER_CXXFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' WTP_SCHEDULER_LDFLAGS='-fsanitize=address,undefined'`:
  the same 50,700 checks passed under ASan/UBSan.
- `make wtp-scheduler-interop-test SUDO= PICO_SOURCE=WTP-Client/build/pico-reference`:
  verified reference pin `40812e7438f180c5e8d8ad75d4eb227271152b10` and 14 source
  files. Actual Pico endpoint/job service in separate translation units with a
  software clock/engine: two early scheduled executions and one pending
  invalidation passed. Its prerequisites also passed the existing component
  replay/reconnect and backend/controller interoperability tests.
- `make wtp-backend-test wtp-protocol-test wtp-plan-test wtp-usb-test SUDO=`:
  5,472 backend checks; 10,373 protocol checks; 946 independent codec cases,
  one framing vector and three pinned snapshots; 2,924 Session checks;
  1,033 conversion checks; 1,937 independent frequency vectors; 2,644 USB
  fake/sysfs/PTY checks passed.
- From `src/WSPR-Transmitter/src`:
  `make simulated-backend-test transmission-controller-contract-test`: passed.
- `make semantics-test-portable SUDO=`: passed the explicit simulated-only
  application profile. The expected submake jobserver warning and negative-test
  diagnostics remain. This does not establish the full Linux physical profile.
- `make semantics-make-regression-test SUDO=`: passed.
- From the repository root:
  `c++ --analyze -std=c++20 -Isrc -Isrc/WTP-Client/include -Xanalyzer -analyzer-output=text src/wtp_integration/scheduler.cpp src/wtp_integration/backend.cpp`:
  passed with no analyzer diagnostics.
- Final whitespace and new/changed local Markdown targets checked. The unchanged
  root README `LICENSE.MIT.md` link remains a pre-existing out-of-scope defect.

CI adds the ordinary scheduler suite to existing macOS/Linux jobs. These jobs
trigger for devel pushes or devel-targeted pull requests; this feature push alone
is not an exact-commit CI pass. Linux runtime, ThreadSanitizer and real host clock
provider integration were not tested locally; the injected concurrency tests and
ASan/UBSan are not broader platform qualification.

## Documentation impact and boundaries

Updated the scheduling contract, prompt, review evidence, root/component progress
links and earlier adapter/backend next-step references. Component paths modified
are `src/WTP-Client/README.md` and `src/WSPR-Transmitter/README.md` only. New runtime
code and the backend admission callback live in parent `src/wtp_integration`.
No dependency or implementation change was made inside either component.

Reviewed the separate Wsprry_Pi_Docs backend and timing guidance read-only; it is
unchanged because production operator behavior is unchanged. Future documentation
must explain early preparation versus RF start, host/device clock prerequisites,
pending versus committed reload, missed slots and blocked recovery, alongside
endpoint selection and permissions. Future UI retains the user-requested
UI-level development toggle and mandatory Impeccable desktop/mobile review.

No physical USB, RF, GPIO, SSH, installation, services, reboot, dependency
installation or independent-repository mutation occurred. Actual CDC/DTR/unplug,
firmware hardware execution, production process/service lifecycle and RF
qualification remain outstanding. No release, tag, production readiness or
Phase 10 completion is claimed.
