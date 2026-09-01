# RP1-GPCLK-DKMS installation

WsprryPi can orchestrate installation of the independently maintained
[`WsprryPi/RP1-GPCLK-DKMS`](https://github.com/WsprryPi/RP1-GPCLK-DKMS)
provider. WsprryPi does not vendor its module source, reproduce its DKMS
lifecycle, select a GPIO route, edit boot configuration, load the module, or
enable output.

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
- `rp1-gpclk-route-manager-v1`;
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

## Development sources

Development selection clones or validates the authoritative repository,
records a full commit, checks out that commit detached when applicable, rejects
tracked or untracked changes, and revalidates checkout identity immediately
before mutation. The upstream preflight derives the development module version
from the exact checkout's canonical source header and binds that header by hash.
WsprryPi does not infer a development version from release metadata
or supply a version to the upstream installer.

The maintained exact-source installer must provide route-neutral installation
with output disabled and without loading the module. WsprryPi verifies the
returned exact commit, canonical version, kernel, UAPI, installed module hashes,
null route, and output-disabled state before recording success. Development
identity remains `v0.9.0-pi5`; exact commit, kernel, route, hashes, enrollment,
and qualification remain separate facts.

## Plan, record, and safety boundary

Before package mutation a real installation displays the WsprryPi channel,
detected platform, selection reason and override state, requested and resolved
source, immutable tag or commit, version and expected hashes, lifecycle command
class, and the output-disabled boundary. With `DRY_RUN=true`, the standard
installer command wrapper reports both RP1 planning and application commands
as skipped and does not invoke Python, download or clone provider inputs, run
upstream preflight, inspect a package, or mutate provider state. Debug mode
shows the exact helper command; during a real run it also reports the helper's
external command argv and captured validation output. The resolved plan and
ordinary package/DKMS or upstream lifecycle output remain visible during a real
installation even when debug mode is off.

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
Failed installs and dry runs create no ownership record. A stale record blocks
a new provider installation rather than being overwritten.

The record proves only that this WsprryPi workflow installed the recorded
provider identity. It is not a signature, route selection, hardware or kernel
qualification, GPIO permission, transmission authority, or RF evidence. The
older v1 installation record did not distinguish a newly installed provider
from an idempotently accepted pre-existing provider, so it is deliberately
insufficient for automatic removal.

## Ownership-aware uninstall

After WsprryPi application and service teardown, uninstall checks the v2
ownership record. Missing, legacy, malformed, symlinked, insecure, foreign,
mixed, active, configured, enrolled, manager-bound, or identity-drifted state
is preserved with an operator-facing reason. A matching release is revalidated
against its complete recorded manifest and module-artifact hashes, then removed
with the normal Debian package lifecycle. A matching development installation
is removed only through the exact upstream `development-rollback` entrypoint
and rollback record captured during installation. WsprryPi never invents a
direct DKMS, module unload, overlay, route, boot, service, GPIO, or RF removal
sequence.

If any preceding WsprryPi uninstall step fails, safe teardown continues where
possible, but provider removal is skipped. The provider and ownership record
are preserved, the overall uninstall fails, and no uninstall-success message is
reported.

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
