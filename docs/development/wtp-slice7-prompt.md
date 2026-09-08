# Phase 10 Slice 7 execution prompt

Implement WTP runtime status publication and explicit recovery reporting in
`/Users/lbussy/GitHub/WsprryPi` on `codex/phase10-wtp-slice1`, starting at
`f18c72f9a3cf4b773f7793032fc1503813813e5a`. Retain the RP1 reboot-recovery
baseline `a5239049f01bd8741e2f101b71d46e81a15ca28a`. Inspect instructions,
working tree, remote refs, affected parent/component contracts and current
status/recovery paths before editing. Preserve unrelated changes.

Slices 1–6 provide the portable client, pure plan conversion, USB adapter,
complete-plan backend and early scheduler. Slice 7 must integrate observation
with that real Session/backend/controller/scheduler chain, including while run()
is blocking. Add a coherent copyable status snapshot that another thread can
read without accessing owner-thread Session or backend state. Keep snapshot
publication bounded to owner-thread progress and lifecycle boundaries; readers
must neither poll transports nor mutate state. Include local schedule phase,
request/job/slot identity, local ARM handoff, remote session phase and verified
device/boot identity, negotiated capabilities, authoritative remote state/output,
tracked-job evidence, ownership/lease observations, uncertainty and safety-fault
latches, adjustments, diagnostics, and retained final execution/recovery results.
Record monotonic publication and STATUS observation times. Unknown output must
be represented as unknown, never false. Historical results must remain explicitly
separate from the current remote observation and from a new request.

ARM admission/acknowledgement is not running. Local Executing means the worker
is executing its protocol lifecycle. Only matching authoritative remote evidence
can establish running, completion or cancellation. A closed stream invalidates
current output evidence without erasing retained final reports. Preserve failed,
missed, foreign-owned, changed-identity, faulted and missing-terminal-record cases.
Provide exact JSON serialization suitable for later application consumption,
using decimal strings for 64-bit identities/times/frequencies and null for absent
observations. Do not route this through legacy callbacks that assert local GPIO,
convert starting into transmitting, repeat automatically, or clear soft faults.

Recovery remains explicit and single-owner: reconnect the same Session, inspect
status, then request bounded owned cleanup. Reconnect alone must not clear the
scheduler's Blocked state. Report each recovery attempt and matching evidence;
retain the original failed execution report. A failed recovery remains blocked;
changed boot/device or latched safety faults cannot be reset through this API.
Never automatically CLAIM, retry, reload, rearm, abort foreign/standalone work,
or infer safe retransmission from missing history. No process-restart journal or
cross-process adoption is introduced. Recovery does not retroactively turn the
original outcome into success.

Test actual wire/session/controller execution with virtual clocks and injected
peers: waiting, loaded, armed before start, running, complete, pending cancellation,
remote cancellation, missed start, lost mutation acknowledgements, reconnect,
recovery failures/success, foreign ownership, replaced identity, fault latches,
missing history, two consecutive jobs, stale evidence and concurrent readers.
Check serialized null/false distinctions and exact large integers. Extend the
existing pinned Pico endpoint interop test (40812e7438f180c5e8d8ad75d4eb227271152b10)
for status evidence. Add focused Make/CI coverage; run normal and ASan/UBSan
coverage, affected backend/scheduler/protocol tests, shared component contracts,
Make regression and the explicit macOS portable semantics subset. Inspect recipes
first. Independently adversarially review implementation, repair actionable
findings, rerun affected checks, and reassess before committing.

This slice completes the status/recovery boundary of the explicitly constructed
WTP runtime. Production selection, INI/CLI, endpoint/clock configuration and
websocket/UI wiring remain the next integration slice; do not claim production
activation. No UI files or workflows change here. Future UI work requires
Impeccable, desktop/mobile review and the user's temporary UI development toggle.
Keep all implementation in the parent integration directory; preserve portable
component boundaries. Do not revise the normative WTP protocol or other repos.

No physical USB, RF/GPIO, SSH, installation, services, reboot, dependency
installation or firmware modification is authorized. Update developer contracts
and progress links. Read operator backend/timing guidance in Wsprry_Pi_Docs and
report its follow-up without edits. Review the complete staged diff, commit and
push only this branch, verify clean origin parity, then report implemented scope,
exact validation and remaining production/target/USB/RF work, modified component
paths and Documentation Impact. Do not claim Phase 10 or RF qualification complete.
