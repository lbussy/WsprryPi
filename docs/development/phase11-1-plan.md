# Phase 11.1 execution plan

Status: implemented and locally software-validated; see [review and acceptance](phase11-1-review.md). No physical acceptance claim.
Baseline: clean `devel`, `dff597467f8ec8aebaf30e4361c985c073953eab`, equal to
fetched `origin/devel` on 2026-09-08. Read-only Pico reference:
`0fd8191c5218d3b5f2da9122a2ae55bf728ae3f2`. The WTP-Client provenance pin and
protocol fixtures remain at their existing revision.

## Initial code review

- The parent owns worker/scheduler/backend/Session and freezes complete jobs.
  USB is the only native stream. Extend the native transport factory, retaining
  the typed injected stream seam and hardware-disabled guard.
- `ByteStream::WouldBlock` means no bytes accepted from that call. OpenSSL WANT
  results cannot directly implement that promise. Own a bounded plaintext queue;
  report accepted bytes as Progress and retain the exact SSL retry buffer until
  completion. Any later loss remains an ambiguous accepted write in Session.
- Native resolution must not block a worker or shutdown. Run system resolution
  outside the owner, bound outstanding resolver resources and the caller's wait,
  and inject deterministic address sets in tests. Each new connection resolves
  afresh. Configured identity, never resolver/reverse-DNS output, is TLS authority.
- Existing equality checks cover path changes but not credential replacement at
  unchanged paths. Capture validated credential contents into the runtime and
  compare content fingerprints on explicit reload/reconnect. Refuse rotation
  while the runtime has ownership, pending work or unresolved output.
- Pico has one TLS slot. Idle remote management must use the same application
  operation lock, check authoritative inactive/unowned state, close the idle WTP
  connection without RELEASE, perform one HTTP exchange, and require WTP
  reconciliation before later preparation. Polling copies observations only.
- Current HTTP guards classify explicit routes; new API paths need backend and
  Apache proxy policy coverage together. Existing trusted-LAN control is not
  user authentication; outbound Pico control uses the administrator's mTLS identity.

## Resource and ownership mapping

Shared API v1 response/error/ETag/decimal-string conventions are retained.
Host extensions have explicit names; remote standalone values never overwrite
WsprryPi application configuration.

| Resource | Authority and mapping |
| --- | --- |
| capabilities | Host adapter reports actual supported controls and observed WTP CAPS; remote management capabilities are separately identified. |
| status / jobs | Copied host-owned WTP observation, exact quantities, age and output uncertainty; polling never connects or mutates. |
| jobs cancellation | Explicit cancellation of the currently tracked host job through the existing application; browser request identity binds replay to that exact job. No raw browser CLAIM/LOAD/ARM path or second scheduler. |
| config / schedules | Pico standalone shared resource semantics through serialized idle management; validated by Pico with its exact If-Match. WsprryPi config remains `/config` and receives a distinct versioned host resource with revision checking. |
| network | Configured/resolved/authenticated host transport observation plus separately named remote Pico network management; no implicit connection during polling. |

Browser sessions identify request/replay scope, not independent WTP ownership.
The host runtime remains the sole WTP owner. No browser can adopt foreign work,
clear uncertainty, or resubmit a job after an ambiguous result. Legacy routes
remain compatible. Mutations require the existing peer/Host/Origin policy plus
same-origin JSON request intent; no permissive CORS on the new API.

## Execution sequence and acceptance

1. Implement parent OpenSSL TLS 1.3 transport, mTLS, exact hostname/IP verification,
   mandatory ALPN, bounded resolution/connect/handshake/I/O, cancellation and
   credential snapshots. Test partial I/O, stalls, EOF, identities, invalid
   credentials and resolver changes before connecting it to production selection.
2. Add default-USB transport discriminator and network fields through typed
   settings, JSON, INI defaults/parsing/persistence and runtime. Validate active
   credentials before mutation, preserve inactive values, block unsafe rotation.
3. Implement versioned shared API adapters and serialized idle Pico management,
   host ownership/request replay rules, ETags and exact-number serialization.
   Cover proxy and direct-backend authorization, revisions and bounded parsing.
4. Extend the existing Transmitter panel using Impeccable Operate guidance:
   explicit USB/network selection, local credential references, configured versus
   authenticated status, preserved drafts, nearby failure/recovery feedback,
   explicit idle management. Preserve console-controlled development visibility.
5. Build a WsprryPi-owned host harness against the unmodified Pico server/core
   and SDK-pinned Mbed TLS, with a valid 32-hex device identity and inhibited
   software engine. Keep all generated credentials/build output private and
   ignored. Test actual application jobs and shared management over loopback.
6. Run focused existing WTP/component tests, new TLS/API/interop tests, portable
   production and semantics tests, relevant policy/UI checks and separate
   sanitizer builds. Render desktop/mobile and failure states; perform the
   Impeccable finish review and repair its actionable findings.
7. Adversarially review identity confusion, ambiguous mutations, replay/ownership,
   cancellation bounds, secrets, rotation/config races and guard/proxy coverage.
   Record each finding, repair and affected rerun; repeat until actionable
   findings close. Update integration/component docs and acceptance record.
8. Inspect final complete diff and tree; stage only this task, commit on `devel`,
   push `origin/devel`, then verify remote parity and report exact evidence.

## Documentation and scope

Update in-repository network/setup/API/test guides and stale Phase 10 integration
statements. The separate operator docs repository is read-only: follow up in
`docs/Command_Line_Operations/transmitter_backends.md`,
`docs/Advanced_Operations/ini_configuration/transmitter_backends.md`,
`docs/User_Interface/Setup/Transmitter/index.md`, Operations recovery and REST API
guidance. No generated credential or local skill artifacts enter Git.

DHCP plus mDNS is the intended deployment; no address reservations, `/etc/hosts`,
resolver installation or trust-store mutation. Document Linux NSS/mDNS resolver
prerequisites. Present Pico interoperability uses its supported loopback IP SAN;
separate hostname fixtures test future name-based operation.

11.2 concurrent physical-job servicing, 11.3 Pico mDNS/hostname certificates and
joint resolution acceptance, 11.4 inhibited hardware, 11.5 target contention,
11.6 conducted RF and 11.7 joint closure remain outside this implementation.
