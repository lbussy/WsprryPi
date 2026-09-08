# Phase 10 production integration execution prompt

Continue WsprryPi on `codex/phase10-wtp-slice1`, starting from Slice 7 commit
`7d4a5aa54b4283f0a88b89fa69a836a212f1475f` and retaining RP1 reboot-recovery
merge `a5239049f01bd8741e2f101b71d46e81a15ca28a`. Inspect instructions, clean/dirty
state, remote refs, all affected parent/component contracts and the existing
configuration/runtime/UI lifecycle. Save this prompt before implementation.
Only WsprryPi may be edited; other repositories are read-only references.

Implement the next coherent task: make the existing complete-job WTP stack
reachable through production backend configuration, runtime orchestration,
status/recovery controls and the first-party UI. Do not leave another development
wrapper as the only entry point. Preserve every existing backend's valid settings
and behavior. Keep WTP logic in parent integration code and the portable client
independent. Do not implement remote jobs through symbol callbacks or allow any
fallback to local GPIO, Si5351, RP1 or simulation.

Add explicit persisted WTP selection and endpoint configuration with defaults,
JSON/INI parsing, strict validation, candidate copying, normalization,
serialization, CLI backend selection and runtime consumption. Endpoint identity
must include the selected WTP CDC path, USB serial, VID/PID and expected WTP device
identity. Validate bounded strings/integers without opening a device. Preserve
inactive backend settings and reject malformed active settings before mutation.
Keep device clock prerequisites distinct from host UTC and RF calibration.
Use a read-only host synchronization source; GET_CLOCK remains observational.
Expose a bounded start uncertainty setting. Never provision Console time implicitly.

Build/link the parent WTP stack in the application and provide an owned worker
through the runtime bridge. Serialize Session/transport operations, use fresh
session/request/job identities, and keep the same session through explicit
reconnect/recovery. Prepare complete finite jobs before absolute UTC admission.
Use existing canonical request/encoder semantics, select one intended WSPR frame
per slot, preserve plan iteration and timing policy, and schedule non-WSPR jobs
early instead of sleeping until their start before preparing. Reject unsupported
continuous tone, shaping/calibration/host-output features explicitly. Do not alter
RF timestamps to compensate for failed admission or replay uncertain work.

Route configure/start/stop/shutdown/reload and any direct legacy calls through
the selected runtime boundary. Dispatch completion outside the protocol worker
so application callbacks cannot self-join or mutate the active Session. Local
preparing/armed is not RF start; only authoritative matching remote evidence
may produce running/completed/cancelled meaning. WTP must never assert ancillary
transmitter GPIO through the legacy start callback. Block subsequent scheduling,
backend replacement and generic fault clearing while remote work is unresolved.
Keep cleanup bounded, observe foreign ownership without stealing it, and require
explicit same-session recovery. New process startup must inspect inactive state
before any mutation and must not adopt or abort foreign/standalone work.

Publish the existing typed status/JSON with current observation age, identities,
uncertainty, fault and historical report distinctions through an application status
surface. Provide an explicit recovery action with current-state admission; status
GET remains read-only. Follow existing HTTP/control request validation and trusted
LAN boundaries. Never translate disconnect into cancellation, ARM into running,
or successful cleanup into retroactive execution success.

Use Impeccable for UI implementation. Extend the current bench-instrument design,
Bootstrap controls and light/dark themes. Add a temporary UI-level development
toggle, default off, allowing the maintainer to reveal/hide Pico controls. Toggling
visibility must not change persisted backend selection, enable transmission or
clear a fault. Preserve a selected Pico backend visibly and safely when controls
are hidden; never normalize it to GPIO/Si5351. Show endpoint fields, independent
clock requirements, unsupported-feature feedback and status/recovery feedback
near their controls. Preserve failed drafts and existing autosave semantics.
Render desktop/mobile light/dark states, exercise toggle/selection/save/error/
unknown/recovery flows, address material findings and use the skill finish review.
Do not commit local skill/runtime artifacts.

Add meaningful tests for configuration round trips/default compatibility,
malformed/partial endpoints, CLI/profile selection, worker ownership and shutdown,
early scheduling, exact WSPR frame selection, stop/reload races, unresolved
replacement refusal, startup foreign jobs, explicit recovery, status null/false
and historical identity, UI toggle preservation and backend persistence. Use
injected clocks/byte streams/OS seams with the real client/backend/scheduler and
actual application integration where practical. Run focused tests, all affected
component tests, UI tests, portable semantics, pinned Pico endpoint interop,
sanitizers as appropriate, build checks, whitespace and documentation links.
Inspect every test recipe first; simulator/injected evidence is software only.

No physical USB, RF/GPIO, SSH, firmware changes, installation, services, reboot,
dependency installation or independent-repository edits are authorized. Report
any concrete companion firmware defect rather than silently patching Pico.
Review Wsprry_Pi_Docs guidance read-only and record separately authorized operator
follow-up. Document implemented behavior and exact remaining target/USB/functional/
RF validation without claiming Phase 10 qualification from software tests.

Perform adversarial review after implementation, repair all actionable findings,
rerun affected checks and reassess. Review component portions and the complete
staged diff. Commit and push only the existing feature branch, verify clean origin
parity, then report behavior, UI/Impeccable evidence, exact validation, limitations,
component changes, commit/push and Documentation Impact. Do not merge or release.
