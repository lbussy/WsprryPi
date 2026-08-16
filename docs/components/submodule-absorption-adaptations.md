# Issue 415 Cross-Cutting Adaptation Inventory

Status: Pre-migration Phase B inventory

Contract: [`../plans/submodule-absorption-contract.md`](../plans/submodule-absorption-contract.md)

Baseline: [`submodule-absorption-baseline.md`](submodule-absorption-baseline.md)

Snapshot method: [`submodule-absorption-snapshot-method.md`](submodule-absorption-snapshot-method.md)

Exclusions: [`submodule-absorption-exclusions.md`](submodule-absorption-exclusions.md)

## Purpose

This inventory identifies repository-wide changes required to make the absorbed
monorepo truthful and functional. It does not implement them. Every listed edit
belongs to Phase B, after the affected component has passed and recorded Phase A
raw-tree equivalence.

All adaptations must preserve application behavior, public interfaces, source
paths, output names, installation layout, runtime configuration, UI behavior,
hardware behavior, and operator workflow. Terminology-only changes must not
silently redesign scripts or policy.

## Parent Git policy and hooks

| File | Required Phase B action | Invariant and validation |
| --- | --- | --- |
| `.gitmodules` | Remove all ten obsolete entries and then the empty file after all components pass. | `git submodule status --recursive` produces no registered component; all ten paths are ordinary tracked trees. |
| `AGENTS.md` | Replace the submodule policy with a single-repository component policy. Preserve dirty-state protection, component boundaries, UI/Impeccable requirements, hardware safety, and the separate `Wsprry_Pi_Docs` boundary. | Instructions no longer require submodule initialization, detached-HEAD interpretation, pointer commits, or component-remote publication. |
| `.githooks/README.md` | Describe the retained hooks without claiming submodule synchronization. | Installation command remains unchanged. |
| `.githooks/lib/common.sh` | Remove `sync_submodules`, alignment checks, and recursive component-cleanliness checks. Retain only helpers still used by hooks. | No hook initializes or mutates component content. |
| `.githooks/post-checkout` | Remove the obsolete submodule-sync behavior; delete the hook if no independent behavior remains. | Branch checkout performs no component-specific network or index operation. |
| `.githooks/post-merge` | Remove the obsolete submodule-sync behavior; delete the hook if no independent behavior remains. | Merge completion performs no component-specific network or index operation. |
| `.githooks/pre-commit` | Remove submodule validation while preserving the merge-conflict-marker guard. | Existing conflict protection still blocks the same staged conflict markers. |
| `.githooks/pre-push` | Remove submodule validation while preserving direct-push protection for `main` and `master`. | Branch protection behavior is unchanged. |

## Parent build, installation, and operational scripts

| File | Required Phase B action | Invariant and validation |
| --- | --- | --- |
| `src/Makefile` | Rename `SUBMODULE_SRCDIRS` and `SUBMODULE_CPP_SOURCES` plus related comments/macros to component-oriented names. Do not change directory membership, source filtering, include paths, object paths, targets, or linkage. | Before/after source manifests and dry-run build commands are equivalent apart from variable names. Parent output remains `wsprrypi` and `wsprrypi_debug`. |
| `scripts/install.sh` | Remove `--recurse-submodules -j8` from the repository clone command. Leave parent `remote.origin.url` project/organization discovery unchanged. | Clone destination, branch, owner, install flow, and all runtime behavior remain unchanged; a normal clone contains all source. Do not run the installer during migration validation. |
| `scripts/sync_all_branches.sh` | Remove `--recurse-submodules -j8`, the per-branch submodule reinitialization, and its message. Preserve branch fetch, tracking, prompts, fast-forward attempts, stash handling, and return-to-original-branch behavior. | Script behavior changes only by eliminating obsolete component initialization. Review with shell syntax checks; do not use it to mutate branches as validation. |
| `scripts/copy_ui.py` | Rename submodule-oriented docstrings, variables, and `submodule_exists` to component/directory terminology. Continue requiring `WsprryPi-UI/data` at the same path and copying it to the same destination with the same permissions. | Source/destination comparison and existing tests prove identical UI deployment content. Do not run privileged deployment during migration validation. |
| `scripts/research/websocket_thread_memory_rig.py` | Replace `submodule_revisions.txt` and recursive submodule-status commands with parent/component provenance and ordinary-tree evidence. Preserve all runtime, safety, redaction, and artifact behavior. | Dry-run/self-test evidence remains hardware-safe; no service or live rig execution is authorized by Issue 415. |

`scripts/update_local.sh` was inspected and needs no absorption-specific change.
It already builds from `src` and invokes the same UI copy path. Its install and
service effects remain outside ordinary migration validation.

## Parent continuous integration

### `.github/workflows/debian-non-hardware.yml`

Required changes:

- rename checkout wording and remove `submodules: recursive`;
- remove recursive submodule safe-directory configuration while retaining the
  parent safe-directory requirement;
- preserve the existing parent build, semantics, cleanup, RP1 test-double,
  simulator, WSPR simulation, `strace`, and hardware-access rejection coverage;
- keep WSPR-Transmitter commands rooted at
  `src/WSPR-Transmitter/src` with no physical-device or privileged target;
- add or retain safe UI/source integration coverage appropriate to the
  workflow's installed Node/PHP dependencies;
- add a WSPR-Reference job or steps that reproduce its standalone workflow:
  configure, build, `tests/run_major_regressions.sh`, install to a temporary
  prefix, configure/build the exported-package consumer, and run the consumer.

The nested `src/WSPR-Transmitter/.github/workflows/debian-backend-contracts.yml`
job is already covered by the parent workflow's simulator, bounded realtime,
and transmission-controller steps, with additional startup and Si5351 contract
coverage. Verify that equivalence after build-name adaptation. Retain the nested
workflow as part of the coherent extractable component unless review proves it
misleading.

The nested `src/WSPR-Reference/.github/workflows/ci.yml` is not discovered by
GitHub in the parent repository. Port its `build-and-test` behavior to the
parent workflow while retaining the nested file for standalone extraction.

## Standalone component output names

Seven standalone Makefiles currently consult the component repository's
`remote.origin.url`. After absorption they would see the WsprryPi parent remote
and can emit colliding `wsprrypi` names. Replace remote-derived naming with the
smallest fixed, component-local definition that preserves these outputs:

| Makefile | Release/demo output | Debug/test output |
| --- | --- | --- |
| `src/INI-Handler/src/Makefile` | `ini-handler` | `ini-handler_test` |
| `src/LCBLog/src/Makefile` | `lcblog` | `lcblog_test` |
| `src/Mailbox/src/Makefile` | `mailbox` | `mailbox_test` |
| `src/MonitorFile/src/Makefile` | `monitorfile` | `monitorfile_test` |
| `src/PPM-Manager/src/Makefile` | `ppm-manager` | `ppm-manager_test` |
| `src/Signal-Handler/src/Makefile` | `signal-handler` | `signal-handler_test` |
| `src/WSPR-Transmitter/src/Makefile` | `wspr-transmitter` | `wspr-transmitter_test` |

Do not change target names, compiler/linker flags, directories, source
discovery, or sibling-component paths except to replace misleading variable and
comment terminology where needed.

`src/Singleton/src/Makefile` already fixes `NAME = singleton` and produces
`singleton_test`; it requires no naming change. WSPR-Reference already fixes the
CMake project as `wspr_ref`, the library as `wspr_ref_lib`, and its
executable/package targets explicitly; those names must remain unchanged.
WsprryPi-UI has no standalone build entry point whose name depends on Git.

## Active component documentation

| File | Required Phase B action | Boundary |
| --- | --- | --- |
| `src/Mailbox/README.md` | Replace the obsolete Broadcom-Mailbox submodule procedure and references to absent legacy files. Describe the retained monorepo path and optional extraction. State that Broadcom is historical design lineage, not current code provenance. | Do not change source/API or claim a Broadcom license obligation. |
| `src/MonitorFile/README.md` | Replace standalone-clone instructions with monorepo-root build/use instructions and optional extraction guidance. | Preserve standalone build and test commands. |
| `src/PPM-Manager/README.md` | Replace the instruction to include/delete `main.cpp` as a submodule consumer with truthful component and extraction guidance. | Preserve provider-neutral behavior and Chrony safety boundaries. |
| `src/WSPR-Transmitter/README.md` | Replace sibling-submodule and pinned-submodule workflow wording with component paths and parent-workflow coverage. | Preserve backend distinctions and standalone build guidance. |
| `src/WSPR-Reference/README.md` | Replace or remove the former-repository CI badge so it does not imply that historical remote CI qualifies the imported component. Point readers to active parent coverage while preserving standalone CMake/API/package instructions. | Preserve `wspr_ref_lib`, examples, vectors, tests, install/export, and consumer guidance. |

The READMEs for WsprryPi-UI, INI-Handler, LCBLog, Signal-Handler, and Singleton
were inspected and require no submodule-specific correction. LCBLog extraction
guidance and all original URLs/revisions still belong in final
`docs/components/provenance.md`.

## Parent active documentation

These files require truthful single-repository updates:

- `release_tools/developer_notes.md`: remove recursive-clone, initialization,
  detached-HEAD, pointer-update, and separate component commit/push workflows;
  replace them with ordinary component-tree inspection, single-parent commits,
  reusable-boundary guidance, and provenance checks.
- `release_tools/Wsprry Pi Codebase Map.md`: describe UI and `src/` components
  as ordinary coherent directories rather than separate repositories.
- `release_tools/WsprryPi Full Regression Test Runbook.md`: replace recursive
  submodule preflight/final checks and reporting with parent cleanliness,
  component presence/provenance, and ordinary-tree checks.
- `docs/simulated-backend.md`: replace “reusable submodule tests,” “recorded
  submodules,” and checkout wording while leaving simulator behavior and safety
  claims unchanged.
- `docs/plans/canonical-band-correlation-and-wspr-presets.md`: update the
  provisional implementation checklist from parent/submodule tests and commit
  ordering to parent/component coverage.
- `docs/plans/cw-timing-presets.md`: update the proposed plan's UI ownership,
  dependency boundaries, status commands, commit model, and sequence to the
  single WsprryPi repository while retaining `Wsprry_Pi_Docs` as an independent
  repository.
- `docs/plans/wspr-community-frequency-validation.md`: replace the proposed
  preflight's generic submodule inspection with component/provenance checks.

Root `README.md` and the other proposed plans were inspected and need no
absorption-specific change. Historical records under `docs/research/`, retained
evidence files, and `historical/` may continue to say “submodule” when describing
the repository state that existed when the evidence was produced. Do not rewrite
those records for terminology alone.

## Licensing and provenance additions

Phase B must create final `docs/components/provenance.md` and carry forward the
baseline, ownership audit, exclusions, raw-tree evidence, adaptations, original
URLs/revisions, licenses, third-party notices, build/test entry points, and
extraction guidance.

Add a distributable third-party notice/license set for the UI vendor assets and
the bundled nlohmann/json header as required by the contract. Removing a wholly
owned component `LICENSE.md` is a separately recorded Phase B adaptation, not a
raw import exclusion.

## Separate operator-documentation follow-up

The sibling `../Wsprry_Pi_Docs` repository was inspected read-only on
`codex/issue-418-about-table-wrapping`; it was clean. Do not modify it during
Issue 415 without separate cross-repository authorization.

`Wsprry_Pi_Docs/docs/Development/index.md` currently describes the application
components and UI as Git submodules, lists the old layout and remote repositories,
and explains submodule reuse/licensing. A later docs task must replace that with
the ordinary component layout, parent-repository workflow, provenance links,
and extraction guidance. The UI source path referenced by
`Wsprry_Pi_Docs/AGENTS.md` remains valid and needs no path change.

## Completion check for Phase B

After implementing the inventory:

1. search all parent and component tracked content again for active submodule,
   gitlink, recursive-clone, pointer, nested-remote, and old-CI assumptions;
2. classify every remaining hit as historical, provenance, or an error;
3. compare parent source discovery and build commands before/after;
4. verify every fixed standalone output name;
5. verify parent workflow coverage for WSPR-Transmitter, WSPR-Reference, UI,
   and parent integration without hardware access;
6. validate installer clone and UI-copy behavior without installing or
   deploying;
7. confirm final provenance accounts for every Phase B edit and license;
8. report the separate `Wsprry_Pi_Docs` follow-up as incomplete until an
   authorized docs task performs it.
