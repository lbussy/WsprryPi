# Phase 10 Slice 5 execution prompt

Implement only the WTP complete-plan backend in `/Users/lbussy/GitHub/WsprryPi`,
on the existing `codex/phase10-wtp-slice1` branch. The starting commit is
`0c6f43fc1d89fc2c0cb8e215bfedecd2531659c0`, descended from the RP1 reboot recovery
merge `a5239049f01bd8741e2f101b71d46e81a15ca28a`. Inspect status, staged and
unstaged diffs, root/nested AGENTS and current contracts; preserve unrelated work.

Use the existing portable WTP-Client Session, parent execution-plan converter
and USB CDC ByteStream. The normative Pico protocol is pinned at
`40812e7438f180c5e8d8ad75d4eb227271152b10`; do not modify or duplicate it.
Keep parent application dependencies out of WTP-Client. Implement the existing
ITransmissionBackend configure/execute/stop/cleanup/startup interface in the
parent integration directory, with explicit typed construction and connection
to an explicitly selected, already opened WTP stream. The USB adapter remains
responsible for USB identity and Console-interface exclusion; Session must
verify expected device and boot identity. Never enumerate or choose a fallback.

Add the shared BackendKind identity and conservative untested RF classification
needed by the shared controller. Test that controller with the actual backend.
Do not register WTP in the current production factory: it prepares at execution
time and lacks the early absolute-UTC handoff. CLI, INI, build-profile selection,
early scheduler preparation, repeats/reload integration, operator status and UI
are later slices. Existing production defaults and RP1 recovery must be retained.
A future UI must have the user-requested temporary UI-level development toggle
and must use Impeccable with desktop/mobile rendering.

Provide a typed scheduling handoff with a fresh job identity, absolute UTC start
and uncertainty tolerance, separate from ExecutionPlan. Require prior negotiation
and fresh idle status. Configure must validate the complete immutable finite
plan and unsupported local power/GPIO/calibration/envelope controls before CLAIM
and LOAD. Never issue per-symbol commands. Reject backend mismatch, changed
prepared jobs, oversized or unsupported plans and reused job IDs. Preserve exact
WTP per-event adjustment evidence separately from BackendCompileResult: the
shared controller currently applies its first adjustment uniformly and must not
rewrite this remote job. Validate adjusted frequencies against host policy too.

Execute must request GET_CLOCK, check synchronization, uncertainty, holdover age,
lead/horizon, elapsed host observation time, leap exclusion and overflow before
ARM. WTP cannot set UTC; the device needs an independently provisioned clock.
Check the ARM acknowledgment using Session. No late start, retry, reload or rearm
is automatic, including after transport loss, boot change or unknown outcomes.
RP2350 owns event timing after ARM. Report only authoritative matching-job
completion/cancellation with both job and current device output inactive.
Acknowledge that ARM acceptance is not execution progress; do not invent symbol
progress from host polling. Maintain leases during monitoring and bound waits.

Keep all session I/O on one owner thread. stop() may only signal cancellation
atomically; bounded cleanup must reconcile fresh STATUS, abort only this client's
tracked job, handle completion/abort races, and release only this client's idle
ownership. Foreign/standalone jobs and output-unknown/fault states must not be
reported safe. Disconnect is local closure, never cancellation. Keep Session
and immutable job evidence across explicit same-boot reconnect for cleanup;
missing records or changed identities block further work. No durable process
restart recovery is claimed. Provide deterministic injected clock/wait seams
and a steady-clock implementation; detect regressing or stalled clocks.

Add a parent make target and macOS/Linux CI coverage for this backend. Tests
must exercise real framing, codec, Session and converter through injected peers,
including full controller execution, repeated jobs, no duplicate ARM, unsupported
inputs before mutation, capabilities/qualification, adjustments, clock rejection,
lease renewal, stop before/during execution, cancellation races, lost mutation
acknowledgments, explicit reconnect, replacement identity, missing records,
foreign ownership, active terminal output, bounded timeout and cleanup failure.
Use pinned Pico endpoint interoperability when practical and label scripted-peer
coverage accurately. Run focused WTP tests, sanitizer coverage, shared component
controller/simulator tests and the explicit macOS portable semantics profile.
Inspect recipes first. No physical USB opens, RF, GPIO, SSH, installs, services,
reboots, dependency installation or independent-repository writes are authorized.

Update developer contracts and progress links. Review the separate
Wsprry_Pi_Docs backend and timing guidance read-only and list the precise later
operator follow-up. Perform an adversarial assessment after implementation,
repair every actionable finding, rerun affected tests and assess again. Review
component and full staged diffs, commit and push only the current parent feature
branch. Verify remote parity and working-tree state. Report behavior, exact test
commands, findings closed, limitations, component paths, documentation impact,
commit and push. Do not claim target USB/RF qualification or Phase 10 completion.
