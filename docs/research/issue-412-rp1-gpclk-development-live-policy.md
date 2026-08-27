<!-- SPDX-License-Identifier: MIT -->
# Issue 412: guarded RP1 GPCLK development-use policy

Status: hardware-free implementation; target validation and qualification not established.

WsprryPi treats the unreleased RP1-GPCLK-DKMS 1.1.2 GPIO4 r3 and GPIO20 r3
identities as independent `Experimental` development candidates. Ordinary
backend selection, startup, a saved route, package installation, or prior
evidence never authorizes output.

The centralized decision is implemented in
`src/WSPR-Transmitter/src/rp1_gpclk_development_policy.*`. It returns a stable
machine-readable reason, an operator explanation, and an `Experimental`
warning. Unknown and incomplete values deny in deterministic gate order.

The last pre-acquisition check requires deliberate development enablement;
one explicit and consistently reported GPIO4 or GPIO20 route; the exact ABI v4
operation-scoped gate and ABI v3 passive snapshot,
module 1.1.2, route-specific r3 identity and `Experimental` state; affirmative
`live_output=1` evidence; a resolved attributable route transaction; idle
scheduling and application ownership; a closed, exclusively acquirable
endpoint; no cleanup fault; and current physical-topology confirmation bound to
the exact operation and route.

The exact compatibility identities are:

- GPIO4: `v1.1.2-pi5-gpio4-6.18.34-development-candidate-r4`
- GPIO20: `v1.1.2-pi5-gpio20-6.18.34-development-candidate-r4`

The exact source revision for both routes is
`0509909bd916ee738b14a8479d3be47863c6ac72`. At Raspberry Pi 5 process startup,
WsprryPi first establishes an output-unauthorized idle route. An exact packaged
identity continues into packaged route-manager reconciliation. A different
package identity is accepted only as a passive idle state when the persisted,
configured, active, and requested routes agree, boot ownership is current, and
no transaction is pending. This startup state never authorizes output.
The route transaction remains transmission-inhibiting until a validated bounded
request completes its fresh development reconciliation.

After the loopback-only bounded request is parsed and every development
confirmation is validated, WsprryPi performs a fresh passive development-route
reconciliation and binds its generation to that exact request. Backend
configuration then arms the one-use authorization. The provider authenticates
the ABI-v3 development identity and consumes that authorization immediately
before endpoint acquisition. Ordinary packaged operation continues to require
the complete released 1.1.1-1 identity.

Changing route, operation identity, module identity, transaction state, or
confirmation route invalidates the decision. The decision is recomputed after
a read-only provider query and immediately before endpoint acquisition. An
allow result remains development-only and never changes product qualification.

Development enablement is intentionally not a persistent configuration flag.
It is accepted only on the loopback-only `bounded_tone` API as the exact
seven-field `rp1_development` object: `enabled`, `route`,
`physical_connection`, `attenuation_and_load`, `bounded_operation`,
`non_radiating_topology`, and `experimental_acknowledged`. Every Boolean must
be `true`; the route must be exactly `GPIO4` or `GPIO20`; and the confirmation
is bound to that bounded request ID and the current route transaction
generation. Ordinary UI test tones, CLI startup, schedules, services, retries,
and direct adapter calls carry no authorization and remain denied. The API
returns the stable failed-gate code and explanation from the backend; callers
must continue to present the `Experimental` and unqualified warning at the
confirmation point.

Route operations, startup reconciliation, tone stop, failed start, backend
cleanup, a failed identity query, a wrong consume attempt, and successful
consumption all invalidate the one-use authorization. This prevents a prior
confirmation from being retained or transferred to a later operation or the
other GPIO route.

This step does not implement the 1.1.2 package or installation workflow and
does not establish target, GPIO, transmitter, SDR, RF, or qualification evidence.
