# Submodule Absorption Contract

Status: Proposed

Implementation state: Not implemented

Repository affected: `WsprryPi`

History strategy: Snapshot import

## Purpose

Absorb every Git submodule currently recorded by `WsprryPi` into the parent
repository as ordinary tracked content. Preserve the identity, provenance,
licensing, documentation, buildability, and reusable boundaries of each
component while eliminating gitlinks and submodule-management mechanics.

This is a repository-organization migration only. It must not change application
behavior, public interfaces, build behavior, generated output, installation
layout, runtime configuration, UI behavior, RF behavior, GPIO behavior,
scheduling, or operator workflow.

This document records the reviewed migration contract. Its presence does not by
itself authorize implementation, staging, committing, pushing, remote-repository
changes, deployment, or hardware operation. Those actions require explicit task
authorization.

## Execution Roles

Use one active writer throughout the migration:

- the local macOS checkout at `/Users/lbussy/GitHub/WsprryPi` is the sole
  workspace for migration edits, staging, review, and any separately authorized
  commits or pushes;
- `wspr4`, using `/home/pi/WsprryPi`, is the Raspberry Pi build and test
  validation target for Issue 415, not a second migration writer;
- `origin/codex/issue-415-submodule-absorption` is the synchronization point
  between the two checkouts.

Before each validation slice delegated to `wspr4`, confirm that both checkouts
are clean, use the Issue 415 branch, track the synchronization branch, have zero
ahead/behind divergence, and resolve to the same parent commit. Stop if any of
those conditions fails.

Do not edit, stage, commit, or push from `wspr4` during this migration unless a
later task explicitly changes its role. Validation on `wspr4` remains subject to
the repository hardware boundary: do not install software, operate services,
use `sudo` for mutation, access GPIO, MMIO, DMA, mailbox, I2C, or Si5351 devices,
or generate RF. Copying uncommitted migration content to the target is not part
of the default workflow; target validation begins only from a synchronized,
reviewable Git revision unless a later task authorizes and defines a different
transfer method.

## Authoritative Baseline

The implementation task must treat the selected parent checkout and its recorded
gitlinks as authoritative. Reinspect `.gitmodules`, the parent index, recursive
submodule status, and every component revision immediately before acting.

The reviewed pre-migration identities and reproducible inventory digests are in
[`docs/components/submodule-absorption-baseline.md`](../components/submodule-absorption-baseline.md).
They are comparison evidence, not a substitute for the mandatory fresh gate.

The expected inventory is:

- `WsprryPi-UI`
- `src/INI-Handler`
- `src/LCBLog`
- `src/Mailbox`
- `src/MonitorFile`
- `src/PPM-Manager`
- `src/Signal-Handler`
- `src/Singleton`
- `src/WSPR-Transmitter`
- `src/WSPR-Reference`

This inventory and any previously recorded commit IDs are historical context
only. Stop if the actual repository structure materially differs.

## Initial Gate

Read the current root `AGENTS.md` and all applicable nested instructions before
acting. Inspect and record:

- current directory and repository identity;
- parent branch, HEAD, upstream, and ahead/behind state;
- parent working-tree and index state;
- `.gitmodules`;
- `git submodule status --recursive`;
- every submodule's working-tree status, HEAD, remote URL, and parent-recorded
  gitlink;
- ignored and untracked files inside each component;
- nested submodules, if any;
- applicable licenses;
- existing user changes anywhere in the parent or submodules.

Stop without changing anything if:

- the parent or any submodule is dirty;
- a submodule is uninitialized or unavailable;
- a checked-out component differs from its parent-recorded gitlink;
- a nested submodule exists but is not covered by the implementation task;
- existing changes overlap the migration;
- the actual structure materially differs from the expected inventory.

Detached submodule HEADs are normal and are not themselves blockers. Never
reset, clean, stash, rebase, overwrite, or discard repository state to satisfy
the gate.

## Target Organization

Preserve every component at its current path:

```text
WsprryPi/
├── WsprryPi-UI/
│   ├── data/
│   ├── tests/
│   ├── README.md
│   ├── LICENSE.md
│   ├── DESIGN.md
│   └── PRODUCT.md
└── src/
    ├── INI-Handler/
    ├── LCBLog/
    ├── Mailbox/
    ├── MonitorFile/
    ├── PPM-Manager/
    ├── Signal-Handler/
    ├── Singleton/
    ├── WSPR-Transmitter/
    └── WSPR-Reference/
```

Do not flatten directories, rename components, move headers, combine source
trees, normalize names, or reorganize nested `src/` directories. Each imported
component must remain an obvious, coherent unit that can be diagnosed
independently and extracted for later reuse.

Retain, where present:

- component source hierarchy;
- README, design, API, and component documentation;
- license, copyright, and attribution files needed to establish the raw import
  and support the Phase B licensing decision;
- standalone Makefiles or CMake entry points;
- standalone `main.cpp` or demonstration programs;
- tests, examples, test vectors, golden vectors, and test data;
- packaging files and relevant component-specific documentation.

Exclude:

- nested `.git` files or directories;
- generated build products;
- ignored local artifacts;
- editor state;
- `.codex`, `.agents`, `.claude`, `.impeccable`, and similar local tool state,
  unless already intentional tracked product content required by current
  instructions.

Document every exclusion.

The complete reviewed allow/deny decisions and observed local-artifact evidence
are in
[`docs/components/submodule-absorption-exclusions.md`](../components/submodule-absorption-exclusions.md).
Recompute that register's gates immediately before converting each component.

Two currently tracked paths require deliberate, documented treatment:

- retain `WsprryPi-UI/.impeccable/design.json` because it is the UI's intentional
  tracked design-system definition and is required by the repository's UI review
  workflow;
- exclude the empty `src/WSPR-Transmitter/src/.codex` file because it is local
  tool-state residue rather than product content.

Do not generalize either decision to similarly named paths without inspecting
their tracked contents and history.

## Component Reuse Requirements

### `LCBLog`

`LCBLog` is used by other projects and must retain a strong reusable boundary:

- keep `src/lcblog.cpp`, `src/lcblog.hpp`, and `src/lcblog.tpp` together;
- preserve its README, Makefile, and test or demonstration entry point; preserve
  its component license through raw verification and then apply the approved
  parent-license consolidation decision;
- do not introduce dependencies on WsprryPi headers, globals, configuration,
  runtime services, or directory layout;
- keep WsprryPi-specific integration outside the component directory where
  practical;
- document its original URL, imported revision, license, and future extraction
  procedure;
- do not modify the former remote or attempt bidirectional synchronization.

### `WSPR-Reference`

Preserve `WSPR-Reference` as an independently understandable and buildable
component:

- preserve its standalone `CMakeLists.txt`;
- preserve the `wspr_ref_lib` API and package structure;
- preserve CLI programs, examples, tests, golden vectors, and test data;
- keep it buildable and testable from its component root;
- do not introduce dependencies on WsprryPi application internals;
- record its original repository, imported revision, and relevant version or tag
  provenance.

### Other source components

Preserve each remaining component's named root, README, source
hierarchy, and available standalone build, test, or demonstration assets. Do not
refactor, format, modernize, or otherwise change component contents during
raw absorption. Preserve component licenses through raw verification, then apply
the approved Phase B licensing decision.

#### `INI-Handler`

- preserve `test/test.ini`, the standalone `main.cpp`, atomic file-update
  behavior, and the dual stock/live file contract;
- exclude ignored `src/build/` artifacts;
- run standalone tests against temporary copies of INI fixtures so validation
  cannot modify tracked files.

#### `Mailbox`

- preserve `bcm_model.hpp`, `mailbox.cpp`, `mailbox.hpp`, the standalone demo,
  and build entry point;
- review and correct active README instructions that still describe submodule
  installation or files no longer present in the imported tree;
- state accurately that Broadcom mailbox work is a historical predecessor or
  design lineage only, not code-level provenance for the current component;
- do not require or retain a separate Broadcom license or notice unless the
  fresh file-header and contribution audit finds actual third-party material
  that contradicts the established current-tree ownership;
- compile during ordinary non-hardware validation, but do not execute privileged
  `/dev/mem` or `/dev/vcio` paths without separate hardware authorization.

#### `MonitorFile`

- preserve the header-only library, standalone demo, and build entry point;
- run its file-change test in a temporary directory because the demonstration
  creates and modifies a test file;
- replace active standalone-clone instructions with truthful monorepo and
  extraction guidance.

#### `PPM-Manager`

- preserve `PPM_Weighting_Discussion.md`, the provider-neutral snapshot API,
  standalone demo, and build entry point;
- exclude ignored `src/build/` artifacts;
- validate provider-unavailable and safe provider-neutral behavior without
  installing, starting, stopping, or reconfiguring chrony;
- replace active instructions that describe the component as a submodule.

#### `Signal-Handler`

- preserve the signal-thread API, standalone demo, and build entry point;
- use bounded callback and shutdown tests that require neither `sudo` nor
  real-time scheduler changes and do not send uncontrolled signals to unrelated
  processes.

#### `Singleton`

- preserve the header-only library, standalone demo, and fixed-name Makefile;
- validate first-instance success and second-instance rejection with a
  temporary available UDP port.

#### `WSPR-Transmitter`

- preserve `external/`, all public interfaces, standalone build files, backend
  contract tests, fake-device tests, and guarded qualification sources;
- exclude ignored build output and the empty tracked `src/.codex` file;
- preserve the distinction among simulated, fake-I2C, fake-GPIO, guarded live
  qualification, and production physical-backend paths;
- port the component's Debian backend-contract workflow to the parent workflow
  directory or provide demonstrably equivalent parent CI coverage;
- ordinary migration validation may run simulator, planner, fake-I2C,
  fake-GPIO, startup-quiesce, and controller-contract tests only after their
  targets have been inspected;
- do not run generic `make test`, `watchdog`, `gdb`, live qualification
  executables, privileged backends, or physical-device paths as migration
  validation.

### `WsprryPi-UI`

Keep `WsprryPi-UI` at its existing path as a coherent first-party UI component.
Preserve:

- the `data/` deployment tree;
- UI tests;
- README, design, product, and API documentation; preserve the component license
  through raw verification and then apply the approved parent-license
  consolidation decision;
- installed web-root layout;
- parent/UI integration behavior.

Do not change UI appearance, wording, navigation, validation, persistence, or
behavior. The migration is not a UI design task.

Retain the tracked `.impeccable/design.json` design-system definition. Run the
existing UI suite, parent source-integration coverage, and deployment-copy
comparison. Because this migration affects the tracked UI component, follow the
root `AGENTS.md` Impeccable workflow and render and inspect applicable desktop
and mobile views, even though no visual change is intended.

### Standalone component naming

Before absorption, several standalone Makefiles derive their project and output
names from the component repository's `remote.origin.url`. After absorption,
that lookup resolves to the parent `WsprryPi` remote and can silently rename or
collide component binaries.

Inspect every retained standalone build entry point. Where output naming depends
on the former component remote, make the smallest behavior-neutral adaptation
needed to preserve the component's pre-migration output names when built from
its component root. Record each adaptation separately from the raw snapshot
comparison. Do not retain a fake nested Git remote or nested repository solely
to preserve naming.

## Provenance Record

Create `docs/components/provenance.md` as part of the migration. For every
imported component, record:

- component name and retained path;
- original repository URL;
- exact parent-recorded import SHA;
- checked-out SHA;
- branch or detached state observed during migration;
- most recent commit date and subject;
- former component license path;
- former component license and copyright holder;
- ownership and contribution-audit result;
- parent license adopted after absorption;
- component license file retained or removed, with the reason;
- third-party licenses and notices retained, if any;
- relevant tags or version provenance;
- whether the former remote was left untouched;
- files deliberately excluded from import;
- standalone build or test entry points;
- special reuse requirements;
- extraction guidance where applicable.

Use snapshot import. Do not merge unrelated repository histories into the parent
or rewrite history. Former repositories remain untouched historical references
unless a later task separately authorizes another disposition.

## Licensing Consolidation

The parent WsprryPi repository is MIT-licensed. A former component that is
entirely the user's own copyright-controlled work may be absorbed under the
parent repository's MIT license without permanently retaining a redundant
component-level MIT license file.

License consolidation is a Phase B adaptation, not part of raw snapshot import:

1. retain and record the component license during Phase A tree-equivalence
   verification;
2. inspect all tracked file headers, contribution history, vendored directories,
   copied material, and attribution documentation;
3. verify that the user owns or controls all copyright in the component files;
4. identify any third-party copyright, contribution, license, or notice that
   must remain;
5. when exclusive ownership is verified and no separate obligation exists,
   remove the redundant component-level MIT license file and document adoption
   of the parent license;
6. otherwise retain the required component or third-party license and notice
   files without alteration.

Previously distributed component-repository versions remain available under
their historical licenses. Absorption does not revoke those grants. The former
component repositories remain untouched historical references; archival,
visibility, deletion, transfer, or other remote disposition is a separate later
task.

For Mailbox specifically, the current component contains no Broadcom code.
Broadcom is historical context only and creates no separate licensing obligation
for the current tree absent contrary evidence discovered during the required
audit. Update active Mailbox documentation so it does not imply current Broadcom
code provenance.

### Verified ownership and third-party dispositions

The universal ownership audit at the recorded Issue 415 revisions found only
Lee Bussy contributor identities in the ten component histories. The ordinary
component implementation in `INI-Handler`, `LCBLog`, `Mailbox`, `MonitorFile`,
`PPM-Manager`, `Signal-Handler`, `Singleton`, and `WSPR-Transmitter` is therefore
eligible for parent-license consolidation. The files under
`WSPR-Transmitter/external/` are Lee-authored minimal parent-project stubs, not a
vendored third-party library. Mailbox source history and current file headers
confirm the established conclusion above: the current tree contains no
Broadcom-authored code.

Two components contain third-party product content that must not be relicensed
solely under the parent license:

- `WsprryPi-UI/data/vendor/` contains Bootswatch Zephyr 5.3.8, Bootstrap 5.3.8,
  Bootstrap Icons 1.11.3, Font Awesome Free 6.5.0, jQuery 3.7.1, Barlow Semi
  Condensed font files, and Source Sans 3 font files. Preserve all embedded
  attribution comments. During Phase B, add or retain a distributable
  third-party notice and license set covering the applicable MIT, SIL Open Font
  License 1.1, and any applicable Font Awesome icon-license terms. The component
  `LICENSE.md` may still be consolidated into the parent license because it
  covers the WsprryPi-authored UI, but it is not a substitute for these vendor
  notices.
- `src/WSPR-Reference/include/nlohmann/json.hpp` is an exact copy of the
  `nlohmann/json` single header at upstream commit
  `f8eee1bb7953c6a4bff384d45052d5acc3d69698` (SHA-256
  `acaa0c0e8cb75bbb2001ef3312549140f0ede093cf6f772683b612db75ecd004`). It
  declares version 3.12.0 but is not byte-identical to the v3.12.0 release
  artifact. Preserve its embedded SPDX and copyright notices. During Phase B,
  add or retain the upstream license texts needed for the MIT-licensed header
  and its identified CC0-1.0 and Apache-2.0 portions. The component
  `LICENSE.md` may be consolidated into the parent license only for the
  WsprryPi-authored component code; it does not replace the upstream notices.

Record these exact dispositions in `docs/components/provenance.md`. Recheck the
tracked trees immediately before import and treat any newly introduced
contributor, vendor file, or changed third-party revision as a new audit gate.

## Per-Component Conversion Procedure

Process components individually in two explicit phases and verify each one
before continuing.

### Phase A: raw snapshot import

Use the proven object-derived procedure in
[`docs/components/submodule-absorption-snapshot-method.md`](../components/submodule-absorption-snapshot-method.md).
Its tree-OID comparison and preservation rules are required Phase A evidence.

1. Record the parent gitlink, checked-out HEAD, remote, status, license, tags,
   commit metadata, and tracked-file inventory.
2. Create a safe temporary snapshot of the exact tracked tree at the recorded
   revision.
3. Remove the gitlink from the parent index without losing reviewed files.
4. Remove only the component Git administrative link or metadata needed to end
   submodule status.
5. Restore the exact tracked component tree at the same path as ordinary parent
   content.
6. Stage the ordinary files when staging is explicitly authorized.
7. Compare the staged ordinary tree with the former component tree.
8. Confirm that differences are limited to Git administrative metadata and
   documented exclusions.
9. Confirm that no user, ignored, generated, or local-tool artifacts were
   imported.
10. Continue only after the comparison passes.

Preserve the raw comparison evidence before making any integration adaptation.
At this boundary, the ordinary tracked tree must match the former component tree
except for Git administrative metadata and pre-approved, documented exclusions.

### Phase B: required monorepo adaptations

After the raw comparison passes, apply only the reviewed adaptations required by
absorption:

1. preserve standalone component output naming without depending on a component
   Git remote;
2. relocate component-local CI coverage or add equivalent parent workflow jobs;
3. update active component documentation that still instructs users to clone,
   initialize, or update a submodule;
4. consolidate wholly owned component licensing under the parent license and
   retain all required third-party notices according to the licensing audit;
5. apply any other behavior-neutral path or terminology change demonstrated to
   be necessary by the integration inspection.

List every Phase B content change separately in the migration report. Re-run
component and parent validation after these adaptations. Do not describe an
adapted tree as byte-for-byte identical to the raw source snapshot.

After all components pass:

- remove all obsolete entries from `.gitmodules`;
- remove `.gitmodules` if no submodules remain;
- stage that removal when staging is authorized;
- do not perform optional destructive cleanup of former remotes or unrelated
  local Git metadata.

The review diff for each component must clearly show deletion of a mode `160000`
gitlink and addition of its ordinary files.

## Required Integration Updates

Inspect the entire repository for active assumptions about submodules. At
minimum inspect:

- root `AGENTS.md` and `README.md`;
- `src/Makefile`;
- `scripts/sync_all_branches.sh`;
- `scripts/install.sh`;
- `scripts/copy_ui.py`;
- support and research scripts that record submodule revisions;
- release and update scripts;
- CI workflows;
- component-local `.github/workflows` files that will not execute after they are
  nested in the parent repository;
- `release_tools/developer_notes.md`;
- `release_tools/Wsprry Pi Codebase Map.md`;
- active plans containing submodule instructions.

Update only what is required to make the repository truthful and functional
after absorption. This may include:

- removing `--recurse-submodules` requirements;
- removing initialization and pointer-management workflows;
- replacing submodule-state diagnostics with component and provenance checks;
- describing one parent repository in active instructions;
- renaming misleading build variables such as `SUBMODULE_SRCDIRS` where needed.

Script and build-file edits must be behavior-neutral apart from removing
obsolete submodule mechanics. Historical documents may retain the word
"submodule" when clearly describing past state. Do not rewrite unrelated
historical material merely for terminology cleanup.

GitHub Actions only discovers workflows in the parent repository's root
`.github/workflows` directory. Preserving a former component workflow under its
component root is provenance, not active CI. For every imported component-local
workflow, either port its jobs to the parent workflow directory or document and
demonstrate equivalent parent coverage. In particular, preserve:

- WSPR-Transmitter simulator and transmission-controller backend contracts;
- WSPR-Reference configure, build, regression, temporary-prefix install,
  exported-package consumer build, and consumer execution coverage.

`Wsprry_Pi_Docs` remains a separate sibling repository. Do not modify it without
explicit cross-repository authorization. Report any required follow-up there.

## Non-Goals

The migration does not authorize:

- application or component source refactoring or formatting;
- modernization or warning cleanup;
- header, API, or component renaming;
- flattened component directories;
- parent-build redesign or replacement of Make with CMake;
- a package manager or changed linkage architecture;
- changed installation, deployment, or runtime paths;
- changed configuration, UI, RF, GPIO, MMIO, DMA, mailbox, I2C, Si5351,
  scheduling, or transmission behavior;
- remote deletion, archival, transfer, visibility changes, or synchronization;
- an automated `LCBLog` export workflow;
- changes to `Wsprry_Pi_Docs`;
- hardware, service, installation, deployment, GPIO, or RF operations.

## Validation

### Repository validation

Confirm:

- `.gitmodules` is absent unless an unexpected, separately approved submodule
  remains;
- `git submodule status --recursive` reports no registered submodules;
- all ten paths contain ordinary tracked files;
- no component contains nested Git administrative metadata;
- every imported tree matches its recorded source revision;
- every provenance record matches observed evidence;
- no generated, ignored, or local-tool artifacts are staged;
- `git diff --cached --check` passes before an authorized commit;
- the complete diff contains no unexplained source-content changes.

### Build and test validation

Inspect test targets before running them. Run safe, supported, non-hardware
validation where available, including:

- ordinary WsprryPi build;
- parent non-hardware regression tests;
- canonical unprivileged Debian validation from
  `.github/workflows/debian-non-hardware.yml` where applicable;
- existing UI tests and parent UI/source integration tests;
- Impeccable desktop and mobile visual inspection of the unchanged imported UI;
- standalone `WSPR-Reference` configure, build, major regressions,
  temporary-prefix install, exported-package consumer build, and consumer run;
- standalone `LCBLog` build or test entry point plus an extraction smoke test
  from a temporary Git-free copy of that component alone;
- other safe standalone component builds or tests.

For the remaining components, apply these explicit boundaries:

- INI-Handler: temporary-fixture standalone test;
- Mailbox: compile only, with no privileged device execution;
- MonitorFile: temporary-directory file-change test;
- PPM-Manager: provider-unavailable or controlled provider-neutral tests without
  chrony lifecycle changes;
- Signal-Handler: bounded unprivileged callback and shutdown tests;
- Singleton: temporary-port first/second-instance test;
- WSPR-Transmitter: only the inspected simulator, planner, fake-device,
  startup-quiesce, and controller-contract targets authorized above.

Read `docs/simulated-backend.md` before application-level simulated transmission
tests. Simulator evidence qualifies software contracts only. It does not qualify
physical backend or hardware behavior.

Use documented build commands and an appropriate job count. Do not invoke
installation targets, `sudo`, services, GPIO, MMIO, DMA, mailbox, I2C, Si5351,
RF, or attached devices. If the validation host cannot execute a
Raspberry-Pi-specific build or test, report the limitation precisely rather than
substituting source inspection for Pi qualification.

### Checkout validation

Before a migration commit exists, perform the safest equivalent export or
staged-tree completeness check available. A literal fresh clone cannot contain
an uncommitted staged migration. Defer and explicitly report true fresh-clone
verification until an authorized migration commit exists.

Do not claim fresh-clone acceptance before that post-commit test passes.

### Hardware boundary

Do not perform hardware validation without separate explicit authorization.
Build, unit-test, and simulator results do not qualify RF output, GPIO timing,
MMIO, DMA, mailbox, I2C, Si5351, installation, service lifecycle, frequency
accuracy, or Raspberry Pi runtime behavior.

## Acceptance Criteria

The migration is accepted only when:

- all ten former submodule trees are ordinary parent content;
- every imported tree is traceable to its exact original URL and revision;
- component boundaries, documentation, and tests remain recognizable;
- every licensing disposition is documented, wholly owned components use the
  approved parent license, and all required third-party licenses and notices are
  retained;
- `LCBLog` remains decoupled and practically extractable;
- `WSPR-Reference` remains independently buildable and testable;
- `WsprryPi-UI` remains coherent with unchanged behavior and layout;
- active workflows require no submodule initialization or pointer management;
- parent, component, UI, and applicable non-hardware checks pass;
- a post-commit fresh clone is complete without `--recurse-submodules`;
- no generated files, local-tool state, or nested Git metadata are committed;
- documentation accurately describes the resulting organization;
- no behavioral or public contract changes are introduced;
- former remote repositories remain untouched unless separately authorized.

## Final Review and Handoff

The implementation report must state:

- parent branch, starting HEAD, upstream, and ahead/behind state;
- initial parent and submodule cleanliness;
- every imported component;
- exact source URL and SHA for every import;
- excluded files and reasons;
- all documentation, build, script, CI, and policy files changed;
- diff summary and tree-equivalence results;
- tests and builds run with exact outcomes;
- validation not run and why;
- `Wsprry_Pi_Docs` follow-up identified but not performed;
- whether hardware, RF, services, deployment, remotes, commits, or pushes were
  touched or performed;
- remaining qualification gates, especially true fresh-clone testing after an
  authorized commit.

Do not describe the migration as fully accepted while any required validation
remains unavailable.
