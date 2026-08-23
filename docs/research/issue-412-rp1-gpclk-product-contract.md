# Issue 412: RP1 GPCLK DKMS product contract

> Historical design record: privileged route mutation is now owned by the
> package-installed `rp1-gpclk-route-manager-v1` socket executor. See
> `issue-412-rp1-gpclk-v1.1.1-consumer.md` for the current consumer contract.

This is the WsprryPi product-integration contract for the separately released
[`WsprryPi/RP1-GPCLK-DKMS`](https://github.com/WsprryPi/RP1-GPCLK-DKMS)
provider. The module project owns its kernel source, DKMS/Kbuild, overlays,
canonical UAPI, signing, compatibility metadata, releases, and kernel lifetime.
WsprryPi owns optional installation, application configuration, privileged
route activation, reboot/reconciliation, operator status, and product
qualification decisions.

This contract does not authorize target installation, boot modification,
reboot, GPIO activity, transmission, or RF qualification.

## Immutable dependency

The initial supported dependency is `RP1-GPCLK-DKMS` tag `v1.0.0`, release
decision `d8c45a33e9a8b16cf5ea9a89736347347bc14817`.

- Package: `rp1-gpclk-dkms_1.0.0-1_all.deb`
- Package SHA-256:
  `951289ee5d0e44cff41b59756f00161aba16f43f1450715ba57c4a3679a2e6b8`
- UAPI ABI: 1
- UAPI SHA-256:
  `1d411644352e61402bd4685a5692070d543ab2ee5b016d394294aa98970bd7fb`
- Release source commit: `9e54d88e9f4c9eac79a77c72e60b39c3acb4e6fa`
- UAPI-reported module/build identity: `rp1-gpclk-dkms` / `1.0.0`
- Compatibility manifest SHA-256:
  `370a9ba42f765932783ae6016ee620c9f691d7b55dbb9ba5c2029279ee4d1acf`
- Compatibility manifest ID:
  `rp1-gpclk-dkms-1.0.0-9e54d88e9f4c9eac79a77c72e60b39c3acb4e6fa`
- Qualification identity SHA-256:
  `54ab1395af6fc15f2c67d19f074da490b3b7644af15d253cb59f9f026e10e620`
- GPIO4 overlay SHA-256:
  `c3e17a685694928468bb18c24f5bb4e25454745d6989e6c9d2c2acf447b908d6`
- GPIO20 overlay SHA-256:
  `8eaa8afae7f88a665fc9bec6da1b013be049b2a32c909c729caeff9181bcf3aa`
- Routes: GPIO4 and GPIO20 only

Different artifacts or identities are unsupported until reviewed. The current
v1.0.0 compatibility manifest defaults to `Unavailable`. Its exact GPIO4 and
GPIO20 entries are both `Unavailable` and `liveEligible: false`; the
qualification identity also records `targetVerificationAuthorized: false`.
Installation or compilation does not override those states.

## Optional installation

Ordinary installation does nothing with the DKMS module. Explicit optional
installation is restricted to a confirmed Raspberry Pi 5 or Compute Module 5
with BCM2712 identity. It verifies the exact package checksum and inventory,
canonical UAPI, architecture, headers, DKMS result, and signing state.

Installation places both route overlays inactive. It does not require a route,
edit boot configuration, select an overlay, load or bind the module, change
hardware state, enable output, or reboot. Installation and DKMS build success
establish installation/build compatibility only.

## Route state model

WsprryPi keeps these values distinct:

- **requested:** current operator draft;
- **persisted:** value saved in application configuration;
- **configured:** WsprryPi-owned boot route;
- **active:** module-reported route;
- **eligible:** active route accepted by exact compatibility/live policy; and
- **previous:** last reconciled route retained for rollback.

Transmission readiness requires:

```text
persisted = configured = active = compatibility-accepted live-eligible route
```

A saved value is never presented as active without module confirmation.

## Idle gate

A route change is accepted only when no WSPR, QRSS, FSKCW, DFCW, or test-tone
execution, immediate scheduling commitment, stop, drain, cancellation, cleanup,
provider lease, backend transaction, shutdown, or restart is active. The
scheduler and provider must demonstrate a bounded quiescent state.

Failure rejects the change without modifying persisted or boot state. The UI
may preserve the draft and must identify the blocker. Editing does not stop or
redirect active or committed work.

## Operator transaction

Selecting the already active route validates and persists it without reboot.

Selecting the other route remains a draft while WsprryPi performs read-only
preflight of idle state, package/UAPI/overlay identities, current route,
compatibility, live eligibility, and boot ownership. The UI identifies current
and requested routes, explains the reboot, and offers exactly:

- **Apply route and reboot**
- **Cancel**

Cancel restores the persisted active selection and leaves application, boot,
module, and hardware state unchanged.

Apply revalidates all gates, locks the transaction, writes a recoverable
journal, atomically stages only WsprryPi-owned boot state, verifies exactly one
route overlay, persists the requested route, verifies both sides, records
`awaiting reboot`, and requests reboot. No mutation occurs before explicit
confirmation.

Live eligibility and permission to stage an inactive route are separate gates.
Ordinary operation may stage only a route already accepted by the provider's
exact compatibility policy. A separately authorized target-validation plan may
instead permit one exact, output-inhibited transition even while the provider
reports the inactive route unavailable. That exception permits boot staging
only: it never makes the route eligible, clears the transmission inhibit, or
permits provider acquisition.

## Privileged route manager

The web process does not execute arbitrary shell, accept arbitrary file paths
or overlay names, edit boot files directly, invoke unrestricted `sudo`, modify
foreign configuration, or reboot without confirmation.

The privileged route manager exposes only fixed operations equivalent to:

```text
query
preflight GPIO4|GPIO20
apply-and-reboot GPIO4|GPIO20
rollback
reconcile
```

It rejects unknown commands, routes, arguments, files, boot layouts, artifacts,
foreign or ambiguous overlay state, concurrent transactions, non-idle state,
and unauthorized callers.

WsprryPi manages only an owned boot fragment or delimited block, configures
exactly one route overlay, never overwrites foreign state, uses checked durable
atomic replacement, retains prior owned state until reconciliation, and
journals file identity, prior digest, requested route, and transaction
generation.

## Failure and recovery

The journal distinguishes preflight, boot staged, configuration persisted,
reboot requested, awaiting reconciliation, success, rollback required or
completed, and manual recovery required.

- Boot-stage failure does not persist or reboot.
- Persistence failure after staging rolls back boot state and does not reboot.
- Verification failure rolls back attributable changes and does not reboot.
- Reboot-request failure after staging disables transmission and reports
  **Route change staged — reboot required**, offering **Reboot now** or
  **Roll back**.
- Application/helper restart inspects the journal before enabling transmission;
  it reconciles, rolls back, or fails closed. Stale generations cannot finish.

## Startup reconciliation and acquisition

Before enabling RP1, compare persisted route, owned boot route, active route,
module/build/UAPI identity, route-specific compatibility/live eligibility, and
provider ownership/cleanup state.

Success marks the route active, clears the completed journal, and permits RP1
subject to all other gates. Any mismatch keeps transmission disabled, blocks
provider acquisition, preserves diagnostics, and offers bounded repair or
rollback without automatic boot rewriting or fallback.

Every acquisition requires exact module/build/UAPI, active=persisted=configured
route, required capabilities, accepted compatibility, live eligibility,
supported drive, nonzero lease, and safe cleanup. `ACQUIRE.expected_route`
verifies the route; it never switches it.

The target-validation exception is a fixed root-owned, non-group/world-writable
plan at `/var/lib/wsprrypi/rp1-gpclk-route-qualification.json`. It is accepted
only for purpose `issue-412-target-validation`, package
`rp1-gpclk-dkms=1.0.0-1`, the exact target model and running kernel, the pinned
UAPI and requested-route overlay digests, `output_inhibited: true`, a maximum
24-hour validity window, and the next transition in a generation-bound route
sequence. A missing, malformed, stale, over-broad, identity-mismatched, or
wrong-sequence plan fails closed. Creating the plan and running the target
procedure require separate authorization; residue cleanup removes it.

There is no automatic fallback to the other route, `/dev/mem`, legacy MMIO,
mailbox, Si5351, simulation, or arbitrary GPIO.

## Configuration and GPIO conflicts

Carry the route through defaults/migration, INI/JSON parsing, validation, typed
configuration, UI population, drafts, autosave, persistence, serialization,
runtime reload, scheduling snapshots, acquisition, status, support bundles,
update, and rollback. Migration must not imply activation or qualification.

Validate the route bidirectionally against LED, amplifier, Band GPIO, and other
first-party assignments. A conflict blocks persistence/activation, preserves
the draft, identifies the exact conflict, and never silently moves, disables,
or normalizes either value.

## UI contract

Show route controls only when RP1 selection is meaningful. Distinguish package
installed, provider available/compatible/live-eligible, requested/persisted/
configured/active routes, reboot requirement, progress, failure, and recovery.

Required states include GPIO4/GPIO20 active, checking, reboot required,
applying/rebooting, canceled, staged/reboot failed, route mismatch with
transmission disabled, provider unavailable, and route unqualified.

Feedback stays beside the selector, supports keyboard and assistive technology,
does not rely on color, works desktop/mobile and light/dark, blocks duplicate
apply, and keeps **Apply route and reboot** distinct from autosave.

## Scheduling, compatibility, and qualification

A committed execution retains an immutable route snapshot. Edits do not
redirect or cancel active/committed work. No new RP1 work is committed after a
transition begins. New route use starts only after successful reconciliation.

GPIO4 and GPIO20 are independent targets. Every compatibility entry binds exact
Pi, architecture, kernel/config/headers, firmware, base device tree, overlay,
provider/DMA, module/UAPI, route, drive, modes, signing, cleanup, evidence, and
live decision. Unknown defaults unavailable; version ordering is insufficient.

Install, build, load, output-disabled tests, and persistence do not qualify
electrical behavior, timing, modes, frequency, power, spectrum, services, route
transitions, or RF. Live work requires separate exact route, hardware, mode,
frequency, duration, drive, output-path, and stopping authorization.

## Diagnostics, upgrade, and removal

Runtime and support evidence report model/kernel/headers, package/DKMS/signing,
module/UAPI/overlay identities, requested/persisted/configured/active routes,
compatibility/live state, owner/lease/terminal/cleanup state, journal/reboot
state, last transition, and boot conflicts without unnecessary private content.

Upgrade verifies the exact replacement, preserves route preference, revalidates
compatibility, and remains unavailable until reconciliation. Removal requires
idle/no lease/no cleanup, resolves transactions, removes only owned boot state,
preserves foreign state, refuses unknown package removal, and reboots only with
confirmation when required.

## Validation and completion

Hardware-free tests cover optional installation, immutable identities, both
routes through configuration, idle acceptance, active rejection, state
distinctions, same-route no-reboot, other-route reboot/cancel, fixed privileged
commands, foreign-state rejection, atomic rollback, persistence/reboot failure,
crash recovery, startup reconciliation, mismatch/provider/UAPI/live failures,
no fallback, GPIO conflicts, UI accessibility/responsiveness/themes, and
diagnostics.

Separately authorized target validation covers exact inactive installation,
both activations, GPIO4-to-GPIO20 and reverse application-managed transitions,
cancel, staging/reboot/process/service failure, reconciliation, rollback,
residue audit, and safe GPIO/clock state.

Issue completion requires optional install, inactive overlays, exact identity
checks, both-route configuration, removal of the Pi 5 GPIO4-only rejection,
idle enforcement, privileged transaction, reboot/cancel UI, no-mutation cancel,
attributable rollback, reconciliation, exact-route acquisition, bidirectional
GPIO conflict handling, no fallback, diagnostics, hardware-free tests,
desktop/mobile/light/dark review, operator documentation, target lifecycle
validation, and exact compatibility records.

## Non-goals

- arbitrary GPIO routing;
- hot switching during transmission;
- a route-changing UAPI operation;
- automatic cancellation caused by selector edits;
- automatic route or backend fallback;
- silent boot mutation or unconfirmed reboot;
- qualification transfer between routes;
- pre-Pi-5 support in this module;
- WsprryPi custom-kernel ownership; or
- RF authorization through configuration alone.

## Current implementation gap

Implemented on the Issue 412 integration branch: exact v1.0.0 package/UAPI
pinning, a route-neutral optional Pi-5-only install gate, pre-install
current-kernel-header checks, post-install package/UAPI/overlay/DKMS/module and
signing-policy verification, inactive overlay delivery, the released UAPI
adapter, route-bound query/acquisition, fail-closed checks, GPIO4/GPIO20
configuration parsing, validation, JSON/INI persistence and reload, immutable
scheduling snapshots, bidirectional first-party GPIO conflict handling, and
read-only requested/persisted/configured/active/eligible route reconciliation.
Acquisition now requires exact route agreement and preserves the committed
route snapshot without route or backend fallback. Hardware-free dependency,
provider, mismatch, lifecycle, scheduling, and configuration tests cover these
implemented boundaries. The privileged route-manager core now exposes only
query, GPIO4/GPIO20 preflight, apply-and-reboot, rollback, and reconcile. It
uses a strict WsprryPi-owned fragment, exclusive transaction locking, checked
atomic-write and readback boundaries, durable generation-bound journaling,
attributable rollback, stale-generation rejection, and an injected reboot
boundary. Exhaustive hardware-free failure injection covers every mutation
stage without touching a real boot filesystem or rebooting a host. An
application orchestration core now enforces the complete controller,
scheduler, backend, and lifecycle idle predicate; latches a durable scheduler
transmission inhibit before route mutation; coordinates manager preflight,
apply-and-reboot, rollback, and startup reconciliation; and clears the inhibit
only after exact reconciliation or verified rollback. Hardware-free integration
tests cover every idle blocker, reboot-request failure, process death after
staging, startup mismatch, and pre-reboot recovery using fake privileged
operations. The operator UI now distinguishes requested and active routes,
keeps Pi 5 route edits as drafts instead of autosaving them, and provides
adjacent accessible checking, reboot-required, applying, staged, mismatch,
unavailable, rollback, and active feedback with explicit **Apply route and
reboot**, **Cancel**, and bounded rollback actions. UI regression coverage and
desktop/mobile light/dark review cover the implemented presentation boundary.
Runtime status now carries the pinned expected package identity, configured
route identities, transaction-journal state, and explicit unknown active,
eligibility, and cleanup values until the production route provider supplies
them. Support bundles independently collect read-only dpkg, current-kernel
DKMS, modinfo/module, device, overlay digest, boot-route, persisted-route,
cleanup, and journal evidence without inferring eligibility. Installation,
transmitter setup, troubleshooting, REST/WebSocket API, support-bundle, and RP1
operator documentation is rendered from the separately maintained docs
repository.

The production application now binds the route-manager and orchestration cores
to fixed boot, journal, and lock identities, validated configuration
persistence, the scheduler/controller idle state, coordinated shutdown and
reboot, startup reconciliation independent of the web UI, and the bounded
same-origin `/api/rp1-gpclk-route` endpoint. The adapter preserves foreign boot
content, rejects route overlays outside its owned block, uses durable checked
replacement, serializes configuration writers, and never executes
operator-supplied shell or paths. The UI preflights immediately before applying
the returned generation; an already active route is persisted without reboot.

The route manager now also separates boot-staging authorization from provider
live eligibility. A fixed, short-lived target-validation plan can authorize the
next exact GPIO4/GPIO20 boot transition when its target, kernel, package, UAPI,
overlay, generation, route sequence, ownership, and output-inhibition claims
all match. Reconciliation and acquisition still require provider-reported live
eligibility, so this qualification-only path remains transmission-inhibited.
Hardware-free policy, inactive-provider staging, mismatch, runtime-wiring, and
diagnostic regressions cover the separation; support bundles retain the plan as
evidence when it exists.

Still required: update operator documentation, perform separately authorized
target validation, and establish live-eligible compatibility before
transmission. The v1.0.0 compatibility identities remain unavailable, so this
integration fails closed and cannot authorize transmission merely because the
package or module is present.
