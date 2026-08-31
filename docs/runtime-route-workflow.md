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

The manager temporarily inhibits systemd starts using its own service drop-in,
stops WsprryPi, and switches the owned runtime overlay with output disabled.
It does not overwrite the installed service unit or an administrator's mask.
After the route is verified idle, the installed WsprryPi companion updates the
saved pin and disables transmission. A previously running application restarts
in idle mode; a previously stopped or administrator-masked application stays
stopped. Saved `Enable on Boot` preferences and unrelated configuration remain
unchanged. A stopped application's first subsequent startup is also idle.

The browser may disconnect when its application stops. Refresh after the
application restarts. Do not repeat a switch merely because the connection was
lost: `runtime_route_client.py query` reports the durable transaction. Application
readiness is acknowledged only after startup reconciliation, loop setup and
binding the HTTP listener when enabled; a
systemd start request alone is not success. The UI distinguishes restoration in
progress, successful completion, route recovery, and application restoration
failure. Completed records describe the last transaction, not future uptime.

`runtime_route_client.py restore --execute` retries incomplete application
restoration on the installed route without repeating overlay removal/application.
An interrupted route change requires `recover --execute`, followed by a new
explicit switch. Prior-boot state is never used to restart automatically.
Transmission is not resumed by either switching or restoration; the normal
operator controls and existing RP1 operation authorization remain available.

The application installer and `scripts/copy_exe.py` install
`/usr/local/lib/wsprrypi/route_application.py` with the executable. Use a matching,
newly bound DKMS runtime bundle supporting `applicationRestorationVersion=1`.
The companion supports the canonical `/usr/local/bin/wsprrypi -J -i
/usr/local/etc/wsprrypi.ini` service command, optionally with `--no-web`.
It refuses missing services, unsupported command overrides, alternate config
paths or old binaries before stopping the application. Administrator-owned
units using that command are preserved. An old service mask from a previous
manual deployment must be reviewed during installation; it is not automatically
removed or assumed to belong to this workflow.

The startup-only `WSPRRYPI_ROUTE_RESTORE_IDLE` environment value is supplied by
the manager's owned drop-in. It is not a transmission permit or operator setting.
Startup forces `Transmit=false` while preserving the stored boot preference;
the token binds the application's readiness acknowledgement to this transaction.

These changes have offline software validation only. Coherent target deployment,
clock-disabled route switching and actual service restart still require separate
authorization and target evidence. No module installation, GPIO changes, reboot
or RF operation is performed by the offline test suite.

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
