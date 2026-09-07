# WTP runtime status and recovery

Phase 10 Slice 7 publishes coherent observations from the parent WTP
Session/backend/controller/[early scheduler](wtp-scheduling.md) execution path.
`WtpScheduler::status()` returns an owned `WtpRuntimeStatus` copy that can be read
from another thread while `run()` blocks. `wtp_runtime_status_json()` serializes
that copy using the existing parent JSON library. Neither reads the transport,
invokes callbacks, changes ownership nor performs recovery.

This is the status/recovery boundary of the explicitly constructed development
runtime. The production factory, CLI/INI selection, legacy runtime bridge,
websocket messages and UI remain unchanged. They must be integrated with explicit
endpoint and clock configuration in the next slice. The legacy `starting` message
maps to transmitting and its callbacks can assert local GPIO; it must not be used
for WTP ARM handoff. No UI work or feature-toggle implementation occurs here.
The future UI must retain the requested temporary development toggle.

## Reading observations

The scheduler owner publishes during waiting, after session polling, when a
mutation invalidates observations, and at lifecycle boundaries. Snapshot copying
is mutex protected; JSON serialization occurs after the copy, outside the lock.
Only `status()`, `phase()`, `request_stop()` and `invalidate_pending()` are safe
from another thread. Signals appear in the next owner publication. All transport,
connect/submit/run/recover/disconnect operations and raw backend/session getters
remain on one owner thread. No worker thread is created by this API.

`revision` orders publications within this scheduler instance. `observed_ms` is
the host monotonic time of publication, and `status_observed_ms` is the time the
host accepted the most recent authoritative STATUS. Neither is a device sample
time or an electrical measurement. The reader can retain a snapshot indefinitely;
there is no automatic idle polling or freshness guarantee. An armed observation
can arrive after the requested start, and an inactive observation cannot guarantee
continued inactivity. Consumers must retain observation age and identity.

| Field | Meaning |
| --- | --- |
| `phase` | Local worker phase: idle, waiting, invalidated, preparing, executing or blocked |
| `session_phase` | Disconnected, negotiating, ready, changed identity or protocol fault |
| `request_id`, `job_id`, start/dispatch | Current request, or most recently finished request when none is pending |
| `arm_handed_off` | Local final ARM gate passed; does not establish bytes delivered, acknowledgement, start or RF |
| `identity` | Last verified device/boot/product/firmware identity; retained offline, not evidence a device is reachable |
| `session_id`, `owner_id` | This runtime's configured session and owner identities, not proof of ownership |
| `capabilities` | Current ready-session negotiated CAPS; absent while disconnected/negotiating |
| `remote` | Last reconciled STATUS for this session; absent while invalidated/disconnected |
| `job` | Authoritative evidence matching the displayed request's job and verified device/boot; otherwise absent |
| `owns`, `lease_valid` | Locally established ownership and lease at publication; false with unknown remote state is not proof of an unowned endpoint |
| `uncertain`, `safety_fault` | Independent protocol-outcome and remote safety latches |
| `recovery_required` | Scheduler Blocked latch, independent of reconnect and observed inactivity |
| `adjustments` | Accepted LOAD per-event frequency evidence bound to this job, retained through cleanup/disconnect |
| `last_report` | Historical execution result with request/job/device/boot, cleanup, evidence and adjustments |
| `last_recovery` | Historical eligible recovery attempt with request/job/device/boot, cleanup and evidence |
| `diagnostic`, `session_diagnostic` | Application failure and raw session diagnostic text; flags/evidence, not diagnostic text, determine state |

A local `executing` phase covers device clock checks, ARM, waiting and monitoring.
Only `job.state == running` supplies an authoritative running observation for the
selected request. `remote` may describe a different or foreign job and must not
be substituted for selected-job evidence. CAPS and mode/range labels do not
establish RF qualification or inhibition; WTP/1 does not advertise an inhibition
flag. No qualification claim is derived from this snapshot.

Completion/cancellation requires the backend's existing identity-bound terminal
and explicitly inactive job **and device** evidence. The snapshot retains the
last report independently: disconnect clears current `remote`/`job` observations
while historical evidence remains. A newly pending request cannot inherit the
previous job's terminal evidence or frequency adjustments. A cancelled pending
request has no remote job evidence or attempted cleanup; it is not an RF-off claim.
A missed start remains a failure even after successful inactive cleanup.

## Serialization

`wtp_runtime_status_json(snapshot)` emits `wsprrypi.wtp-runtime/1`. All unsigned
64-bit values, including request identity, monotonic times, UTC nanoseconds,
frequencies and revision, are decimal strings to avoid JavaScript integer loss.
Absent observations are JSON `null`; explicit inactive observations are `false`.
Local flags and small bounded counts remain booleans/numbers. CAPS are serialized
without treating their ranges as qualified. The `remote` JSON object contains the
current state/owner/job/output projection; selected terminal history is represented
in `job` and retained reports. The typed `remote` also retains the bounded wire
terminal-record collection. Diagnostics are escaped by the JSON library.

This serializer is an application data boundary, not a new WTP operation or a
public web endpoint. It has no side effects and is not linked into the production
application by this slice. A future UI must present unknown, blocked, pending and
historical states distinctly and avoid reporting ARM as transmitting.

## Explicit recovery

After a blocked execution, the owning worker may reconnect the **same** Session
through the explicitly selected transport, then call `recover()`. Reconnect
performs HELLO, identity verification, STATUS and CAPS. It leaves Blocked intact.
`recover()` uses bounded backend cleanup: fresh STATUS, ABORT only for this
client's tracked owned job when allowed, and RELEASE only for its ownership.
There is no automatic CLAIM, retransmission, LOAD, ARM or retry of uncertain bytes.
A successful cleanup allows a separately submitted new job with a new identity.
The original report remains Blocked; the recovery result is stored separately.

Failed recovery retains Blocked and records the failed attempt and any current
matching evidence. A missing/expired terminal record, unknown transaction or
foreign owner cannot be cleared by a local reset, generic stop or elapsed time.
Changed device/boot identity and safety faults remain latched. Later apparent
inactivity does not clear a safety fault. Eligible recovery calls are those made
while Blocked; calls in other phases return an error without replacing history.

Only the most recent execution and most recent recovery report are retained in
memory. There is no durable journal, process-restart adoption, automatic reconnect,
operator fault-reset mechanism or historical audit archive. Destroying the runtime
closes local state and cannot establish inactive output. Process interruption and
operator recovery beyond same-session reconciliation require separately designed
persistence and workflow; never reconstruct transmission authority from a status
label or a reused owner string.

## Validation and remaining work

From `src`, `make wtp-status-test SUDO=` exercises the actual wire/Session/backend/
controller/scheduler with injected clocks and peers, including concurrent readers,
mutation acknowledgement loss, changed identity, missing history, foreign ownership,
missed starts, cancellation and fault persistence. The existing scheduler interop
target also checks reports against the pinned Pico endpoint implementation with a
software clock/engine. See [Slice 7 review](development/wtp-slice7-review.md) for
commands, findings, repairs and validation boundaries.

Production endpoint/clock/configuration wiring, websocket/UI integration and its
temporary toggle remain unfinished. No physical USB, timing, RF, installation or
service behavior is qualified by these tests. Independent device UTC provision
remains required; GET_CLOCK cannot set it.

## Documentation Impact

This guide and parent/component progress links describe the developer boundary.
The separate Wsprry_Pi_Docs backend configuration, command-line backend, timing
and Setup/Transmitter guidance was reviewed read-only. No operator setting or UI
behavior changed, so those pages remain unchanged. When production selection is
implemented, separately authorized updates must explain endpoint identity and
permissions, host/device clock prerequisites, finite jobs, early preparation,
armed versus running, observation age, output unknown, retained failed reports,
explicit recovery and process-restart limits. The temporary toggle belongs in
developer guidance, not permanent operator instructions.
