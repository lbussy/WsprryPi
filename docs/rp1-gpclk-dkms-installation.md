# RP1-GPCLK-DKMS installation

WsprryPi can orchestrate installation of the independently maintained
[`WsprryPi/RP1-GPCLK-DKMS`](https://github.com/WsprryPi/RP1-GPCLK-DKMS)
provider. WsprryPi does not vendor its module source, reproduce its DKMS
lifecycle, select a GPIO route during installation, edit boot configuration, or
enable output. A source or release that implements the reviewed runtime contract
is activated only to route-neutral controller and manager administration.

## Selection

`INSTALL_RP1_GPCLK_DKMS` accepts exactly:

- `auto` (default): install only on a positively identified Raspberry Pi 5 or
  Compute Module 5 with BCM2712 compatibility;
- `true`: request installation on any platform, without bypassing provider
  hardware, resource, ownership, or build checks; or
- `false`: deliberately omit provider installation.

`RP1_GPCLK_DKMS_SOURCE` accepts exactly:

- `auto` (default): select a published release for WsprryPi `main`/release
  installation, or the DKMS `origin/devel` tip for a named non-production
  WsprryPi branch;
- `release`: select the latest eligible published stable release;
- `devel`: resolve upstream `origin/devel` once to a full commit;
- `commit:FULL_40_CHARACTER_SHA`: select an exact upstream commit; or
- `checkout:/absolute/path`: select the clean HEAD of an authoritative local
  checkout.

Abbreviated commits, relative or symlinked checkout paths, dirty or unrelated
repositories, unavailable branches or commits, and ambiguous WsprryPi source
channels are refused. An explicit source selector can resolve an otherwise
ambiguous WsprryPi source channel, but never weakens source verification.

When provider installation is selected, package handling adds `dkms`, `kmod`,
`device-tree-compiler`, and the exact running-kernel package
`linux-headers-$(uname -r)`. Before provider apply, the installer requires that
package to be installed, requires `/lib/modules/$(uname -r)/build` to resolve to
the matching `/usr/src/linux-headers-$(uname -r)` tree, and verifies the DKMS,
compiler, module, checksum, and Device Tree tools used by the reviewed runtime
profile. Missing or mismatched prerequisites fail before provider mutation.
These kernel-development packages are not added for automatic installations on
non-Pi-5 systems or when `INSTALL_RP1_GPCLK_DKMS=false`. `build-essential` and
`python3` remain ordinary WsprryPi installer prerequisites on every supported
platform.

Examples:

```sh
sudo INSTALL_RP1_GPCLK_DKMS=false ./scripts/install.sh

sudo INSTALL_RP1_GPCLK_DKMS=true \
  RP1_GPCLK_DKMS_SOURCE=commit:0123456789abcdef0123456789abcdef01234567 \
  ./scripts/install.sh
```

## Published releases

No eligible RP1-GPCLK-DKMS release is currently published. Production/main
selection therefore fails closed until the canonical release is prepared with
the contract below; it does not fall back to development source.

Release selection enumerates published, immutable, non-draft, non-prerelease
semantic versions rather than following a `latest` redirect. A candidate must provide
exactly one Debian package plus `SHA256SUMS` and
`rp1-gpclk-dkms-installation-manifest-v1.json`. The selected manifest binds:

- repository, release tag, immutable tag commit, product and Debian versions,
  and release-channel classification;
- package filename, package/DKMS/kernel-module identities, and package SHA-256;
- installed UAPI path and SHA-256;
- `rp1-gpclk-route-manager`;
- route-neutral, output-disabled package behavior with no module load, overlay
  application, boot-configuration edit, or service operation; and
- a canonical checksummed inventory of every package data member.

The installer validates authoritative asset URLs, GitHub asset digests when
present, tag resolution, the checksum file, manifest consistency, Debian control identity, safe archive paths and
symlinks, complete package inventory, and UAPI identity before package-manager
mutation. If the highest selected release is corrupt, validation stops; an
older release is not silently substituted. Historical releases without this
manifest are not eligible.

An exact installed release is verified idempotently. Foreign, mixed,
development, active, enrolled, manager-bound, or different-version state is
refused and must be handled by its owning migration or recovery procedure.
An exact release that was already present is accepted but never claimed as
WsprryPi-owned.

The pre-mutation inventory also rejects opt-in runtime-controller residue,
including its binding, manager fragment, tools, and journals,
endpoints, runtime overlays, and the owned WsprryPi application-inhibition
drop-in. WsprryPi reports the paths but does not remove independently owned
runtime state. Use the RP1-GPCLK-DKMS runtime cleanup workflow; foreign or
ambiguous service overrides remain untouched.

## Development sources

Development selection clones or validates the authoritative repository,
records a full commit, checks out that commit detached when applicable, rejects
tracked or untracked changes, and revalidates checkout identity immediately
before mutation. The upstream preflight derives the development module version
from the exact checkout's canonical source header and binds that header by hash.
WsprryPi does not infer a development version from release metadata
or supply a version to the upstream installer.

The maintained exact-source installer must provide the explicit
`runtime-controller` DKMS profile through a route-neutral installation without
loading either module. A single DKMS instance owns
the consumer and controller; WsprryPi verifies the returned exact commit,
canonical version, kernel, UAPI, both installed-file and decompressed-ELF hashes,
null route, and quiescent unowned state before recording success. Development
identity remains `v0.9.0-pi5`; exact commit, kernel, route, hashes, enrollment,
and qualification remain separate facts.

Runtime-capable source selection additionally requires commit
`8a8bf5d4184714949faffbf2e2538a1cac0526b2`, which provides the single-owner
DKMS runtime-controller profile and binding version 3, or a selected commit/tag
whose Git history contains it. A development checkout or immutable release that
cannot prove that ancestry is rejected before provider mutation or runtime bundle
creation.

A repeated development installation is a no-op only when WsprryPi's existing
v2 ownership record and the complete inactive provider inventory still match
the newly resolved commit, source tree, version source, UAPI, compatibility
identity, running kernel, DKMS registration, source destination, module bytes,
and retained upstream rollback tools. The installer revalidates the ownership
record after those checks before reporting success. It never adopts an exact
foreign installation. Missing, changed, mixed, active, routed, enrolled, or
runtime-profile state still requires its owning migration or recovery workflow.

Runtime-profile residue accounting includes `runtime_activation.py`, both
device endpoints and loaded-module paths, the manager socket, both WsprryPi
application drop-ins, the complete runtime-admin directory (including
activation archives and `last-deployment.json`), the binding, private UAPIs and
overlays, readiness schema, scripts, and units. Installed consumer and controller
artifacts are provider inventory owned exclusively by DKMS, not runtime-deployment
residue.
Foreign or unowned residue is preserved and blocks installation. Exact state
already recorded by WsprryPi is revalidated rather than silently adopted. If
neutral runtime deployment is interrupted after the exact provider has a valid
WsprryPi v2 ownership record, a repeated installation may enter the same bound
deployment and recovery workflow; it does not treat that partial state as a
foreign installation or publish v3 readiness before final neutral proof.
If a later exact development source is selected, the installer first reviews
the ownership record and retained rollback paths, permissions, and hashes, but
does not require ordinary DKMS inventory while the bound runtime deployment is
still present. The installed, ownership-bound runtime provider first interprets
its own binding and every bound artifact against WsprryPi ownership. Ordinarily
it also performs recovery and removal. When a clean reboot has left a coherent
terminal prior-boot journal set that the older installed provider cannot retire,
the newly selected immutable provider may perform only its reviewed,
digest-bound retirement and deployment inverse against that already validated
binding. A retained
neutral-activation journal, including `activation-failed`, is copied
byte-for-byte into the WsprryPi-owned upstream evidence directory before it is
handled through a digest-reviewed recovery transaction. A terminal
`complete-neutral` journal from a prior boot may instead be archived and
retired through the reviewed `activation-retire-plan`/`activation-retire` pair
when the route, modules, endpoints and manager socket are absent. The retirement
plan binds any coherent terminal route, manager and application journals from
that prior boot plus its exact application-owned idle override. It removes the
override while ownership is still proven, then removes journals in retry-safe
dependency order before removing the activation journal. Only then does the selected provider review and execute
the predecessor binding's fixed deployment inverse.
With runtime residue gone, the installer performs the complete
ordinary provider/DKMS inventory check, revalidates the rollback authority,
runs the recorded provider rollback, and installs the new exact source. The
new candidate executable has no route, activation, adoption, or general
predecessor-migration authority beyond that exact inactive retirement/removal
case.
The same migration applies to v2 partial-runtime and v3 ownership. A loaded
consumer is admitted only when exact WsprryPi ownership and runtime residue
lead to the installed provider's digest-reviewed recovery and removal path for
an exact-provider migration. It never migrates an unowned provider, a
boot-configured route, a pending unrelated transaction, or drifted binding or
artifact bytes.

## Plan, record, and safety boundary

Before package mutation a real installation displays the WsprryPi channel,
detected platform, selection reason and override state, requested and resolved
source, immutable tag or commit, version and expected hashes, lifecycle command
class, and the output-disabled boundary. With `DRY_RUN=true`, the standard
installer command wrapper reports RP1 plan resolution as informational context
and the application command as a skipped execution item. It does not invoke
Python, download or clone provider inputs, run upstream preflight, inspect a
package, or mutate provider state. Debug mode shows the exact helper command.
During a real run, the command wrapper owns the status display and suppresses
ordinary helper output. If provider apply fails, its last 20 output lines,
bounded to 16 KiB and stripped of terminal-control bytes, are copied into the
console report and retained installer log before temporary planning state is
removed. The same bounded diagnostic capture applies to pre-update runtime
recovery and final neutral activation failures.

Only when WsprryPi actually mutates an empty provider state and then verifies
the result does it atomically create the root-owned, mode-0600 v2 ownership
record at
`/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json`. A release record embeds
the validated release manifest and hashes every DKMS module artifact present at
ownership creation. Later DKMS artifacts for additional kernels are accepted
only at canonical module paths with the same recorded module version; every
originally recorded artifact must remain byte-identical.
A development record binds the exact source, kernel, installed module,
upstream evidence, rollback record, and captured upstream rollback entrypoint.
Failed provider installs and dry runs create no ownership record. A stale record
blocks a new provider installation rather than being overwritten.

Before a repeat installation replaces application files or restarts the
service, an exact WsprryPi-owned v3 neutral runtime is placed into its provider's
inactive update state. The installer accepts a current `neutral_ready` result,
an already `recovered-inhibited` retry, a valid post-reboot activation-required
state, an exactly journaled same-boot route recovery that left a neutral
nonzero-generation controller, or the narrow same-boot case where the only
conflict is that the application's active PID changed after completed neutral
activation. That last case must retain the exact binding, artifact set,
activation journal, neutral controller, null routes, absent consumer, disabled
output, matching controller observation, and otherwise unchanged
application-service identity.

For a loaded neutral controller, the installer reviews
`activation-recover-plan`, binds its canonical digest to the installed binding,
current boot, controller observation, activation journal, and any terminal
route-recovery journals, archives the activation journal, revalidates
the ownership record, and calls `activation-recover` with only that digest. It
then requires both modules and endpoints absent, every route null, transmission
ineligible, the application inhibited and inactive, and the journal retained as
`recovered-inhibited`. Any other conflict, route, consumer, binding or artifact
drift, changed ownership, malformed recovery plan, or incomplete final state
fails before application mutation. A retry resumes from the retained inhibited
state. First installations with provider-only v2 ownership and dry runs do not
perform this runtime transition.

After the application binary, configuration, service, and exact route companion
then exist, the installer builds the opt-in bundle from the same immutable source
commit and the exact DKMS-installed module pair. Binding version 3 records each
module's canonical path, installed-file digest, decompressed-ELF digest,
compression, version, kernel, and build-note digest. The bundle contains no
module payload and never writes `/lib/modules`. The installer independently
validates its binding and complete artifact inventory,
and calls the upstream runtime provider in this order: `inspect`, `plan`,
`ensure`, `inspect`, `activation-plan`, `activation-ensure`, and final `inspect`.
Both mutation calls receive only the digest returned by the immediately reviewed
plan. The final result must be `neutral_ready`, with
`administrationEligible=true`, `executionReady=false`, no
requested/configured/persisted/active route, no owner or lease, and output
disabled.

The bundle must be self-contained for these pre-deployment calls: its bootstrap
set includes the route client imported by the provider and activation tools.
The route-manager socket and service units are digest-bound deployment payloads,
not assumed host prerequisites; the installed WsprryPi route companion and the
DKMS-owned module pair are external bound prerequisites. A stock `Transmit Backend = gpio` configuration is
valid for neutral inspection and is not rewritten during installation.

Only after that proof is the provider record upgraded to v3 with the readiness
contract, binding and artifact-set digests, source commit,
product/kernel/compatibility identities, reviewed deployment and activation
plan digests, activation request ID, controller session and zero generation,
neutral state, null route, and disabled-output state. This records WsprryPi
orchestration; it does not transfer ownership of upstream files, journals,
modules, units, or systemd state.

A repeat installation accepts an exact WsprryPi-owned v3 development runtime
idempotently. In `neutral_ready`, the administrative route controller is
expected to remain loaded while the transmission consumer, configured route,
output, owner, and lease remain absent. Reuse still requires the exact source,
kernel, DKMS registration, source tree, unique module pair, version, ownership
record, and subsequent bound-runtime validation. An active transmission
consumer, selected route, foreign state, or identity drift remains a refusal.

After neutral administration succeeds, the installer finishes website and
Apache publication and requires `wsprrypi.service` to become active. With web
mode enabled it also
requires the installed loopback `/wsprrypi/version` proxy endpoint to respond.
A systemd start request that returns success but is skipped by a false unit
condition is an installation failure, so no success banner or configuration
URLs are printed. `--no-web` installations require only active service state.

The record proves only that this WsprryPi workflow installed the recorded
provider identity. It is not a signature, route selection, hardware or kernel
qualification, GPIO permission, transmission authority, or RF evidence. The
older v1 installation record did not distinguish a newly installed provider
from an idempotently accepted pre-existing provider, so it is deliberately
insufficient for automatic removal.

Installation stops at route zero. A later explicit operator selection of GPIO4
or GPIO20 invokes upstream `route-plan`; WsprryPi retains the reviewed digest
and calls `route-ensure` only for the same route and current preflight
generation. Only this route transaction changes the application backend to
`rp1-gpclk`, persists the selected pin, and keeps WsprryPi idle. The production
module uses the root-owned mode-0600 endpoint with `output_inhibit=0`; application
policy and operator confirmation still gate every transmission. A saved or
default GPIO value is not installation-time route consent. Because the provider
endpoint is route-owned, `/dev/rp1-gpclk` is normally absent at this neutral
stage. **Setup > Transmitter** keeps GPIO selectable as **GPIO (route required)**
and exposes the route panel so the operator can make the first explicit choice;
the missing endpoint still keeps transmission unavailable until that transaction
completes and runtime readiness is reconciled.

## Ownership-aware uninstall

Before WsprryPi application and service teardown, uninstall recovers an exact
v3 neutral runtime while the recorded application instance still exists. If
that pre-teardown recovery fails, uninstall stops before changing the
application. The exact route, activation, and deployment recovery sequence then
removes runtime residue and the owned provider while the captured application
service and companion still exist, allowing the provider to restore the state it
owns. Ordinary WsprryPi application and service teardown runs only after that
removal succeeds.
Missing, legacy, malformed, symlinked, insecure, foreign,
mixed, active, configured, enrolled, manager-bound, or identity-drifted state
is preserved with an operator-facing reason. A matching release is revalidated
against its complete recorded manifest and module-artifact hashes, then removed
with the normal Debian package lifecycle. A matching development installation
is removed only through the exact upstream `development-rollback` entrypoint
and rollback record captured during installation. WsprryPi never invents a
direct DKMS, module unload, overlay, route, boot, service, GPIO, or RF removal
sequence.

If runtime preparation or provider removal fails, uninstall stops before
application teardown. The provider and ownership record are preserved, the
overall uninstall fails, and no uninstall-success message is reported. Failures
in later independent application teardown remain cumulative: safe teardown
continues where possible, but the overall uninstall still fails.

The ownership record is deleted only after the canonical removal succeeds and
the helper verifies that package, DKMS, source, module, overlay, route,
enrollment, and manager state are absent. A lifecycle failure retains the
record, fails the overall uninstall, and leaves recovery to the recorded owning
procedure. Successfully rolled-back development evidence remains under
`/var/lib/wsprrypi` as uniquely named audit/recovery evidence and does not block
a later installation.

`REMOVE_RP1_GPCLK_DKMS` accepts exactly:

- `auto` (default): remove only a matching, inactive provider proven by the v2
  ownership record;
- `true`: explicitly request the same ownership-aware removal; it does not
  bypass any provenance, identity, inactivity, or upstream lifecycle check; or
- `false`: preserve the provider even when WsprryPi ownership is proven.

For example, to remove WsprryPi while deliberately retaining an owned provider:

```sh
sudo ACTION=uninstall REMOVE_RP1_GPCLK_DKMS=false ./scripts/install.sh
```

`DRY_RUN=true ACTION=uninstall` passes the planned helper argv through the
standard command wrapper, does not start Python, and does not inspect or mutate
provider or ownership state. Debug mode displays the exact `remove --debug`
argv. No failure selects a fallback backend.
