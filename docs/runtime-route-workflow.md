# Runtime route-manager application integration

This companion implementation consumes the explicit
`rp1-gpclk-route-manager-runtime` profile from RP1-GPCLK-DKMS. The
packaged/source-development protocol remains separate. Unknown contracts fail
closed; discovery of the runtime profile asserts the transmission inhibit. Explicit
`idle` and `reconcile-output` requests establish runtime route readiness for
startup and the existing operation-scoped development authorization path. Neither
query authorizes output.

The installer stops in `neutral_ready` with no route. The existing route panel
is the later explicit operator choice: preflight calls upstream `route-plan`,
reviews and retains its SHA-256 digest, and **Switch route (stay idle)**
queues `route-ensure` in a bounded transient system service only for that same
route and preflight generation. This keeps the HTTP request out of the service
stop/restart transaction and prevents the application from waiting on its own
shutdown. A queued response confirms submission, not route completion. The
application requires its full idle predicate and keeps requested state separate
from controller-reported active state.
Recovery explicitly asks the controller to reach no route; it is not a rollback
to a previous GPIO. Removal errors retain their kernel errno and overlay ID.

The manager temporarily inhibits systemd starts using its own service drop-in,
stops WsprryPi, and switches the owned runtime overlay while the application is
idle and transmission is not authorized.
It does not overwrite the installed service unit or an administrator's mask.
After the route is verified idle, the installed WsprryPi companion updates the
backend to `rp1-gpclk`, updates the saved pin, and disables transmission. Before
this explicit selection, a stock `gpio` backend is accepted for neutral
inspection but is left byte-for-byte unchanged. A previously running application restarts
in idle mode; a previously stopped or administrator-masked application stays
stopped. Saved `Enable on Boot` preferences and unrelated configuration remain
unchanged. A stopped application's first subsequent startup is also idle.

The browser may disconnect when its application stops. The Setup route-status
dialog remains open, treats this interruption as expected, and retries read-only
status queries with bounded backoff after the application restarts. It never
repeats the route mutation. If automatic checks pause, the operator may retry a
status query or close the dialog; `runtime_route_client.py query` reports the
durable transaction when the application does not return. Application
readiness is acknowledged only after startup reconciliation, loop setup and
binding the HTTP listener when enabled; a
systemd start request alone is not success. The UI distinguishes restoration in
progress, successful completion, route recovery, and application restoration
failure. Successful completion is labeled **Route selected** and names the
active GPIO while explicitly retaining idle mode. Runtime administration does
not display development-provider fields that its response cannot substantiate;
those absent fields are not evidence that the selected route failed. Completed
records describe the last transaction, not future uptime or transmission
authorization.

`runtime_route_client.py restore --execute` retries incomplete application
restoration on the installed route without repeating overlay removal/application.
An interrupted route change requires `recover --execute`, followed by a new
explicit switch. Prior-boot state is never used to restart automatically.
Transmission is not resumed by either switching or restoration. The normal
operator controls and application-owned RP1 operation authorization remain
available; the kernel endpoint does not receive an authorization credential.
On a runtime-profile system, choosing **None** for **Transmit Pin** exposes the
existing explicit recovery-to-neutral operation as **Remove route**. The UI
passes the last confirmed GPIO route to recovery, does not persist an invalid
pin, and leaves transmission disabled. The detailed route fact panel remains
in the page source for debugging but is hidden from the normal operator layout;
the action and concise state pill sit beside the selector.
An application update after same-boot route recovery may retire the neutral
controller only through the provider's digest-bound activation-recovery plan;
the completed neutral activation and recovered route journal chain must both
remain exact.

The application installer and `scripts/copy_exe.py` install
`/usr/local/lib/wsprrypi/route_application.py` with the executable. Use a matching,
newly bound DKMS runtime bundle reporting `applicationRestoration: true`.
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
