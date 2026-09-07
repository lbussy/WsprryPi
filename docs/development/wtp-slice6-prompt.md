# Phase 10 Slice 6 execution prompt

Implement early WTP scheduling and the absolute-UTC handoff in WsprryPi, on the
existing `codex/phase10-wtp-slice1` branch. Start from Slice 5 commit
`72f882aba8411f78e61f86a717cfb279292d2303`, retaining ancestry from RP1 recovery
merge `a5239049f01bd8741e2f101b71d46e81a15ca28a`. Inspect repository instructions,
status, staged/unstaged changes, remote refs and affected source contracts.
Preserve existing work. Only this parent repository may be modified.

The current physical scheduler wakes two seconds before a WSPR boundary, then
waits until the boundary before executing the backend. Non-WSPR scheduled launches
prepare at their boundary. Neither is a valid WTP early-preparation path.
Implement a dedicated parent WTP scheduler over the actual shared
TransmissionRequest/ScheduledSlot, ExecutionPlanCompiler, TransmissionController
and WtpTransmitBackend. Keep this explicit typed construction separate from
production CLI/INI/UI selection, whose clock, endpoint and status contracts are
later slices. Do not route remote jobs through host symbol callbacks, local GPIO
assertion, legacy start-time waits or generic automatic retransmission.

Save this prompt before implementation. Add the scheduler in src/wtp_integration.
Provide checked conversion of ScheduledSlot.start_time plus its signed
start_offset to positive WTP UTC nanoseconds. Preserve subsecond offsets, reject
overflow/pre-epoch results and avoid floating-point epoch arithmetic. Freeze each
complete request, schedule, fresh job ID, uncertainty tolerance and timing policy
by value before waiting. Do not infer a new slot, truncate multi-frame plans,
change the requested instant, or silently run late. Repeats require explicit new
requests and job identities after successful cleanup.

Require a negotiated selected backend, valid host UTC observations and an
independently provisioned device UTC clock. GET_CLOCK cannot set time. Compute an
early dispatch instant from negotiated minimum ARM lead, an explicit preparation
allowance and an ARM submission allowance. Require that interval to fit the
advertised maximum horizon; these allowances are host policy, not measured USB
latency guarantees. Reject requests already inside their dispatch deadline and
bound maximum pending wait. Detect host UTC jumps relative to monotonic time
while waiting and recheck immediately before ARM submission. A small configured
clock-step tolerance must not allow late dispatch. Reject incomplete clock
observations, regressing/stalled monotonic time and checked-arithmetic failures.
Once ARM is handed off, host wall-clock changes must not retime the remote job.
Keep the backend's authoritative GET_CLOCK and ARM-acknowledgment checks intact.

Implement a single-owner run path without detached threads. Only stop and pending
reload invalidation may be signalled from another thread. Pending invalidation
cancels an uncommitted request; after preparation commits, preserve that frozen
request and report that reload remains deferred until cleanup. Stop must prevent
an unstarted job or perform bounded owned cleanup for a submitted job. Handle
stop races around the backend's schedule reset. A failed/unknown cleanup must
latch the scheduler blocked; later submissions and generic reset cannot erase it.
Explicit same-session reconnect followed by reconciliation/cleanup is the only
recovery surface, with no automatic LOAD/ARM replay. Final outcomes must use
backend evidence and cleanup, never an elapsed host deadline as proof of RF off.

Test with injected host/device clocks and the actual wire/Session/backend/controller
chain: future waiting, early LOAD and ARM with exact slot+offset, frozen requests,
repeat identity, late admission, insufficient horizon, timing arithmetic, clock
steps before/during preparation and after ARM, host/device clock disagreement,
stop at waiting/preparing/execution boundaries, pending and deferred reload,
lease behavior, unknown ARM, blocked subsequent jobs, explicit recovery, and
no duplicate execution. Add actual pinned Pico endpoint interoperability where
practical using the existing test-only separate-translation-unit bridge. The
normative pin remains 40812e7438f180c5e8d8ad75d4eb227271152b10.

Add a focused parent Make target and existing macOS/Linux CI coverage. Run the
scheduler suite normally and with ASan/UBSan, affected backend/interoperability
and WTP regression tests, shared controller/simulator tests, Make regression and
the explicit macOS portable semantics subset. Inspect recipes first. No physical
USB open, GPIO/RF, SSH, firmware changes, installs, services, reboots, dependency
installation or independent-repository writes are authorized.

Update developer contracts and progress links. Review separate Wsprry_Pi_Docs
backend/timing guidance read-only and identify later operator follow-up. No UI is
in scope; future UI requires Impeccable and the requested temporary development
toggle. Perform adversarial review, repair actionable findings, rerun affected
checks and reassess until no Slice 6 findings remain. Review the complete staged
diff, commit and push only this feature branch, verify clean remote parity and
report exact validation, limitations, component paths and Documentation Impact.
Do not claim production selection, target USB/RF qualification or Phase 10 completion.
