# Runtime route-manager application integration

This companion implementation consumes the explicit schema-3
`rp1-gpclk-route-manager-runtime-v1` profile from RP1-GPCLK-DKMS PR #7. The legacy
packaged/source-development protocol remains separate. Unknown contracts fail
closed; discovery of the runtime profile asserts the transmission inhibit. Explicit
`idle` and `reconcile-output` requests now establish runtime route readiness for
startup and the existing operation-scoped development authorization path. Neither
query authorizes output.

The existing route panel uses runtime preflight and an explicit **Switch route
(output disabled)** confirmation. The application requires its full idle
predicate, binds the request to the selected route and preflight generation/token,
and persists the requested route separately from controller-reported active state.
Recovery explicitly asks the controller to reach no route; it is not a rollback
to a previous GPIO. Removal errors retain their kernel errno and overlay ID.

The manager stops and persistently masks WsprryPi before route changes. The UI
therefore warns that its HTTP connection may disappear. A disconnect means
completion unknown, disables further switch attempts until fresh state is read,
and directs the operator to `runtime_route_client.py query`. Subsequent operations
while WsprryPi is stopped use that client. Nothing automatically unmasks, restarts,
or authorizes output. A permanently available browser administration service is
not provided by this change.

Install/update/recovery belong to the module repository's separately reviewed
runtime bundle workflow. This application commit does not install it, replace a
running binary, change services, migrate a firmware route, reboot, or test GPIO.
Coherent target deployment and clock-disabled GPIO4/GPIO20 switching still need
separate authorization and proof. No product or RF qualification is claimed.

The manager can explicitly release its application mask with `resume gpio20
--execute` after validating the idle runtime route. Start the application only
after that request returns. The existing ABI-v4 lease supports authorized output
with the module loaded at `live_output=0`; no globally enabled load is needed.
Route changes still stop and mask the application.

Bounded-tone start acknowledgements are asynchronous. A response includes an
RP1 operation record only when it belongs to that request; the terminal record
retains provider failure reasons and cleanup failures. A worker exception is
reported without terminating the application process.

The 2026-08-31 wspr5 integration test exercised GPIO20 at a requested
14.097100 MHz for ten seconds, including repeated execution and cleanup without
a reboot. Exact source/build identities, failed attempts and successful outcomes
are retained in RP1-GPCLK-DKMS under `docs/evidence/runtime-tone-20260831/`.
This is not analyzer frequency or product qualification. The test used a
temporary configuration and transient loopback-only service; the normal INI,
served UI and masked service were not replaced as part of that test.
