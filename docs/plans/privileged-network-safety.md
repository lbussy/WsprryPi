# Privileged Network Safety Contract

Status: Proposed

Implementation state: Not implemented
Repositories affected: `WsprryPi`, `WsprryPi-UI`, and `Wsprry_Pi_Docs` (the separate sibling operator-documentation repository)

## Purpose

Protect privileged HTTP operations and the browser-facing WebSocket endpoint from clients outside the Raspberry Pi's directly connected LAN while preserving the existing port-80 Apache proxy workflow.

This is network-location access control, not user authentication. It reduces exposure of privileged controls but does not identify users, distinguish trusted users on the same LAN, encrypt traffic, or replace appropriate network and host security.

## Governing Architecture

Apache is the browser-facing security boundary. It sees the browser's actual peer address and SHALL enforce the directly-connected-LAN restriction before forwarding protected HTTP requests or upgrading the WebSocket connection.

The backend remains a defense-in-depth boundary. It SHALL:

- validate the actual socket peer for direct backend-port access;
- validate a local `Host` identity;
- validate an `Origin` when one is supplied;
- ignore `Forwarded`, `X-Forwarded-For`, `X-Real-IP`, and every equivalent forwarded-client header; and
- fail closed when required network information cannot be established.

The port-80 Apache proxy and same-origin browser workflow SHALL remain the normal browser path. Browser code SHALL NOT fall back to backend port `31416` when the Apache WebSocket path is unavailable.

Direct backend ports remain available only for compatible same-LAN non-browser clients. They are not the supported browser path.

## Protected Operations

### HTTP

The following operations are privileged:

| Method | Route | Operation |
|---|---|---|
| `PUT` | `/config` | Replace configuration |
| `PATCH` | `/config` | Modify configuration |
| `POST` | `/config/repair` | Repair or restore configuration |
| `POST` | `/control/stop` | Stop transmission and disable configured transmission |
| `POST` | `/api/support-bundles` | Create a support bundle |
| `GET` | `/api/support-bundles/{id}` | Read support-bundle job status |
| `GET` | `/api/support-bundles/{id}/download` | Download a support bundle |
| `DELETE` | `/api/support-bundles/{id}` | Delete a support bundle |

Preflight and other method handling for a protected route SHALL NOT provide a bypass. A request that would enable or target a protected operation SHALL pass the same Apache boundary and retain the protected route's Host, Origin, CORS, malformed-request, and method-validation policies.

`GET /config`, version information, ordinary status, and telemetry are not privileged under this contract and SHOULD remain available where practical. No read-only route may mutate configuration, runtime state, support-bundle state, or transmission state as a side effect.

### WebSocket

The following commands are privileged:

- `shutdown`
- `reboot`
- `stop`
- `tone_start`
- `tone_end`

The following commands or protocol operations are read-only:

- `get_tx_state`
- `echo`
- ping and pong
- server broadcasts

Apache cannot authorize individual commands after a WebSocket upgrade. In enforced mode, Apache SHALL therefore restrict the entire browser-facing `/wsprrypi/socket` endpoint to the directly connected LAN. This intentionally makes read-only WebSocket commands, ping/pong, and server broadcasts unavailable through that endpoint to off-LAN clients.

A separately designed read-only WebSocket path could restore off-LAN read access later. It is not part of this contract.

## Directly Connected LAN

An allowed browser or direct-backend peer is an address in a subnet assigned directly to an eligible active interface on the Pi.

Eligible interfaces are active non-loopback Ethernet or Wi-Fi interfaces with a usable IPv4 or IPv6 address and netmask/prefix. The initial implementation SHALL exclude:

- loopback interfaces from subnet discovery, while still allowing an actual loopback backend peer;
- point-to-point interfaces;
- tunnel and VPN interfaces;
- container interfaces;
- software bridge interfaces;
- multicast and unspecified addresses; and
- interfaces without a usable address and netmask/prefix.

IPv4-mapped IPv6 addresses SHALL be normalized before comparison. IPv4 and IPv6 subnet matching SHALL use the interface's actual netmask or prefix. A host is not considered local merely because its address is private, unique-local, link-local, or resolves to a local-sounding name.

Multiple eligible physical interfaces and their directly assigned subnets MAY be allowed simultaneously. Extending eligibility to a VPN, tunnel, bridge, container network, or manually declared subnet requires a separate reviewed feature; it SHALL NOT happen implicitly.

If no eligible subnet can be discovered, enforced mode SHALL fail closed for non-loopback protected access and report the discovery failure without silently disabling protection.

## Apache Enforcement

Apache SHALL use the browser's actual connection address. It SHALL NOT derive authorization from a forwarded-client header.

The managed Apache configuration SHALL:

- preserve the incoming browser-visible `Host`;
- add explicit proxy mappings for every protected proxied HTTP route, including `/config/repair` and `/control/stop`;
- distinguish protected methods on mixed-use paths such as `/config`;
- reject off-LAN protected HTTP requests before proxying;
- reject off-LAN `/wsprrypi/socket` upgrades before proxying;
- keep unrelated status and telemetry mappings readable where practical; and
- avoid broad proxy grants that accidentally expose future API routes.

Apache requires concrete allowed CIDRs. WsprryPi's privileged-network-safety application path SHALL discover eligible subnets, generate a managed Apache include, and validate the complete Apache configuration before publishing or reloading it.

Network changes do not silently rewrite Apache policy. The initial implementation SHALL require the same explicit validated apply/reload path used for an administrator setting change. Automatic interface-change regeneration is outside the initial scope.

## Backend Enforcement

For protected HTTP operations on backend port `31415`, the backend SHALL require:

- an actual loopback or same-eligible-subnet peer when enforcement is active;
- a syntactically valid local `Host`; and
- a matching local `Origin` when `Origin` is present.

For direct WebSocket access on backend port `31416`, the backend SHALL apply the same peer, Host, and optional Origin rules before completing the upgrade. Because direct WebSocket access is for non-browser clients, no port-80-to-31416 browser Origin exception is provided.

When `Origin` is supplied, its host identity and explicit/effective port SHALL match `Host`. A non-browser client MAY omit `Origin`. Missing `Host`, malformed Host or Origin syntax, `Origin: null`, a foreign identity, or a port mismatch SHALL fail closed.

Proxied backend traffic arrives from Apache loopback. Backend peer acceptance of loopback does not replace Apache browser-peer enforcement.

## Administrator Override

The authoritative setting is:

```ini
[Security]
Privileged Network Safety = enforced
```

Accepted values are:

- `enforced`
- `insecure-disabled`

Missing, empty, malformed, boolean-like, numeric, or otherwise unknown values SHALL mean `enforced` and SHALL produce a warning suitable for operator diagnosis. Only the exact explicit value `insecure-disabled` disables the peer/subnet restriction.

The override affects only the protected operations and WebSocket endpoint described by this contract. It SHALL NOT disable or weaken:

- Host validation;
- Origin validation;
- CORS policy;
- HTTP method validation;
- malformed-request rejection;
- command validation;
- forwarded-header distrust; or
- ordinary input and configuration validation.

Normal configuration autosave SHALL NOT read, write, normalize, delete, or otherwise alter this setting.

## Apply and Reload Transaction

Changing the administrator override or reapplying policy after an eligible network change SHALL use one validated transaction:

1. Parse and validate the requested setting.
2. Discover eligible local subnets when enforcement is requested.
3. Generate candidate backend and Apache policy artifacts.
4. Validate the candidate WsprryPi configuration.
5. Validate the complete candidate Apache configuration with the platform's Apache configuration test.
6. Publish the candidate configuration atomically.
7. Reload Apache without rebooting the Pi or restarting WsprryPi solely for this setting.
8. Confirm the active Apache and backend policy state.

If any step fails, the operation SHALL report failure, preserve or restore the previously active configuration and policy, and leave protection enforced unless the previously active state was already the explicit insecure override. A failed apply must never convert an enforced system into an insecure-disabled system.

The design SHALL distinguish the requested, persisted, and active values so a partial or failed transaction cannot be reported as successful.

## User Interface Contract

UI implementation requires the repository's Impeccable workflow.

Disabling network safety SHALL require:

- a conspicuous explanation that this exposes privileged controls beyond the directly connected LAN;
- deliberate confirmation separate from normal configuration autosave; and
- the exact typed phrase `DISABLE LOCAL-LAN SAFETY`.

Enabling protection SHOULD require an explicit apply action but does not require the insecure confirmation phrase.

The UI SHALL preserve the operator's draft when validation or reload fails and SHALL show the failure beside the control that initiated the action. It SHALL distinguish requested, persisted, and active state.

When the active value is `insecure-disabled`, the UI SHALL conspicuously display:

```text
NETWORK SAFETY OFF
```

## Status, Configuration, and Logging

When network safety is disabled, logs, status output, configuration reporting, and the UI SHALL conspicuously report the exact text:

```text
NETWORK SAFETY OFF
```

Status and configuration reporting SHALL expose both configured and active state without exposing sensitive network details unnecessarily.

Security logs SHALL distinguish at least:

- off-LAN peer rejection;
- eligible-interface discovery failure;
- invalid or foreign Host;
- invalid, foreign, or mismatched Origin;
- malformed request or WebSocket upgrade;
- invalid administrator setting defaulted to enforced;
- Apache validation failure;
- Apache reload failure; and
- successful enablement or explicit insecure disablement.

Logs SHOULD contain enough peer and policy context for diagnosis while avoiding trust in or promotion of forwarded-client values.

## CORS and Failure Behavior

Protected HTTP responses SHALL NOT use a permissive wildcard CORS policy. Allowed browser requests use the normal same-origin Apache path. The insecure override SHALL NOT change CORS behavior.

An off-LAN protected HTTP request SHALL receive a generic `403 Forbidden` response before proxying. A rejected Apache WebSocket upgrade SHALL receive a generic HTTP `403` and SHALL NOT establish a WebSocket connection.

A direct backend HTTP or WebSocket rejection SHALL likewise fail before executing an operation. Error responses and close behavior SHALL avoid disclosing whether a particular privileged resource, job identifier, or command would otherwise be valid.

Malformed requests remain malformed-request errors when they reach the applicable parser after passing network policy. Network rejection takes precedence at the browser-facing Apache boundary.

## Compatibility and Non-Goals

The implementation SHALL preserve:

- the simple port-80 Apache browser workflow;
- same-origin browser operation on an allowed LAN;
- existing valid ordinary configuration values;
- read-only HTTP status and telemetry where practical; and
- same-LAN direct backend access for compatible non-browser clients.

The initial implementation does not provide:

- authentication or per-user authorization;
- TLS or traffic confidentiality;
- protection from another client already on an allowed LAN;
- remotely available read-only commands through the protected WebSocket endpoint;
- automatic VPN, tunnel, bridge, container, or manually declared subnet trust;
- automatic Apache regeneration on every interface or DHCP change; or
- a replacement for firewall, router, VPN, or operating-system security.

## Required Implementation Phases

Implementation SHALL proceed in separately reviewed slices:

1. Policy and focused tests.
2. Backend HTTP and WebSocket protection.
3. Apache browser-peer enforcement.
4. Administrator override and transactional apply/reload.
5. UI and installer integration.
6. Operator documentation in `Wsprry_Pi_Docs`.
7. Separately authorized `wspr5` runtime qualification.

Each phase SHALL inspect current source and GitHub state first, identify compatibility risks, implement only the approved slice, run focused non-hardware tests, and stop for review before advancing.

The final runtime phase requires separate authorization. It must not contact `wspr4`, transmit, generate a tone, manipulate GPIO, exercise RF/audio/I2C, install, reload Apache, restart services, or invoke shutdown/reboot unless the exact action is explicitly authorized.

## Acceptance Contract

The completed feature is acceptable only when focused tests and authorized runtime evidence demonstrate that:

- off-LAN protected HTTP requests are rejected by Apache before proxying;
- off-LAN browser WebSocket upgrades are rejected by Apache;
- allowed same-LAN browser workflows continue through port 80;
- protected direct backend requests reject off-LAN peers and invalid Host/Origin values;
- forwarded headers never grant access;
- every listed protected operation is covered and read-only HTTP remains available where intended;
- missing or invalid override values enforce protection;
- only explicit `insecure-disabled` bypasses peer/subnet checks;
- the override does not weaken Host, Origin, CORS, malformed-request, forwarded-header, or validation policies;
- validated apply/reload takes effect without reboot;
- failed validation or reload preserves the previously active safe policy;
- normal autosave cannot alter the setting;
- insecure state is reported everywhere as `NETWORK SAFETY OFF`; and
- UI disablement requires deliberate confirmation and the exact typed phrase.

No behavior described here is implemented merely by committing this contract.
