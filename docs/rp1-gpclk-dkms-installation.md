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

Release selection enumerates published, immutable, non-draft, non-prerelease
semantic versions rather than following a `latest` redirect. A candidate must provide
exactly one Debian package plus `SHA256SUMS` and
`rp1-gpclk-dkms-installation-manifest-v1.json`. The selected manifest binds:

- repository, release tag, immutable tag commit, product and Debian versions,
  and release-channel classification;
- package filename, package/DKMS/kernel-module identities, and package SHA-256;
- supported UAPI ABI range, installed UAPI path and SHA-256;
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
WsprryPi uninstall preserves the provider.

## Development sources

Development selection clones or validates the authoritative repository,
records a full commit, checks out that commit detached when applicable, rejects
tracked or untracked changes, and revalidates checkout identity immediately
before mutation. It runs the upstream preflight and exact-source installer only
when that maintained interface supports route-neutral installation with output
disabled and without loading the module.

The present upstream exact-source lifecycle requires a concrete GPIO route.
That is not a route-neutral installation contract, so development installation
currently fails before mutation and identifies the missing upstream interface.
WsprryPi does not guess a route or bypass that refusal. Development identity
remains `v0.9.0-pi5`; exact commit, kernel, route, hashes, enrollment, and
qualification remain separate facts.

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

After successful verification, current provider identity is recorded at
`/var/lib/wsprrypi/rp1-gpclk-dkms-installation.json`. This record describes
source/package state only. It is not a signature, route selection, hardware or
kernel qualification, GPIO permission, transmission authority, or RF evidence.
No failure selects a fallback backend.
