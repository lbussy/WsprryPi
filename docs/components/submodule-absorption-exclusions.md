# Issue 415 Exclusion Register

Status: Pre-migration gate

Contract: [`../plans/submodule-absorption-contract.md`](../plans/submodule-absorption-contract.md)

Baseline: [`submodule-absorption-baseline.md`](submodule-absorption-baseline.md)

Method: [`submodule-absorption-snapshot-method.md`](submodule-absorption-snapshot-method.md)

## Rule

Phase A imports the exact parent-recorded component tree. Nothing is excluded
from tracked content unless this register identifies the exact path and reason.
Ignored, generated, untracked, editor, and Git-administration content is never
an import source. A newly discovered tracked exclusion or local artifact stops
that component for review.

This register distinguishes raw-import exclusions from later Phase B changes.
It does not authorize conversion or deletion.

## Reproduction method

Ignored and untracked list digests are SHA-256 over the exact newline-delimited
paths emitted by Git, including the final newline when the list is nonempty:

```sh
git -C PATH ls-files --others --ignored --exclude-standard | shasum -a 256
git -C PATH ls-files --others --exclude-standard | shasum -a 256
```

The empty-list digest is
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.

## Observed local-state lists

| Checkout | Ignored count | Ignored-list SHA-256 | Untracked count |
| --- | ---: | --- | ---: |
| parent | 68 | `a07908d5749c760f7e784db685266076fdea288d000235e58389944be642ce3c` | 0 |
| `WsprryPi-UI` | 0 | empty-list digest | 0 |
| `src/INI-Handler` | 3 | `838b15ec18798aff09ea06d14e7e73fbaab83eb0c209eea4846a285c13b005a8` | 0 |
| `src/LCBLog` | 0 | empty-list digest | 0 |
| `src/Mailbox` | 0 | empty-list digest | 0 |
| `src/MonitorFile` | 0 | empty-list digest | 0 |
| `src/PPM-Manager` | 3 | `ba249d97629071838f6a321e32765813c77dcc2263f34e4a223fecdde6b132b5` | 0 |
| `src/Signal-Handler` | 0 | empty-list digest | 0 |
| `src/Singleton` | 0 | empty-list digest | 0 |
| `src/WSPR-Transmitter` | 15 | `996cfbd04f7d8dd23bc173fbddc1491d14153f1dc6d449139ebf602a7d47a76d` | 0 |
| `src/WSPR-Reference` | 0 | empty-list digest | 0 |

The parent-local set includes `.DS_Store` and ordinary `src/build/` output. It
is not enumerated as component content because object-derived snapshots never
read from the parent build tree. Its count and digest are recorded to detect a
changed preparation environment.

## Mandatory Git-administration exclusion

Every live component root contains the normal submodule `.git` administrative
link. It is not part of the recorded component tree and `git archive` cannot
include it. The snapshot method preserves the complete live submodule outside
the checkout and restores only its object-derived archive.

No nested `.git` path below a component root and no nested `.gitmodules` file was
observed. Do not import or stage any such path if one appears later. Do not
delete the parent's `.git/modules` object databases as part of Issue 415.

## Ignored and generated component artifacts

These exact observed paths are excluded because they are local build products,
not members of the recorded component trees:

### `src/INI-Handler`

- `src/build/bin/ini-handler_test`
- `src/build/obj/debug/ini_file.o`
- `src/build/obj/debug/main.o`

### `src/PPM-Manager`

- `src/build/bin/ppm-manager_test`
- `src/build/obj/debug/main.o`
- `src/build/obj/debug/ppm_manager.o`

### `src/WSPR-Transmitter`

- `src/build/bin/si5351_planner_test`
- `src/build/bin/transmission_controller_contract_test`
- `src/build/dep/si5351_planner.d`
- `src/build/dep/si5351_planner_test.d`
- `src/build/dep/si5351_transition_test.d`
- `src/build/dep/startup_quiesce_test.d`
- `src/build/dep/transmission_controller.d`
- `src/build/dep/transmission_controller_contract_test.d`
- `src/build/dep/wspr_transmit_backend_si5351.d`
- `src/build/obj/debug/si5351_planner.o`
- `src/build/obj/debug/si5351_planner_test.o`
- `src/build/obj/debug/startup_quiesce_test.o`
- `src/build/obj/debug/transmission_controller.o`
- `src/build/obj/debug/transmission_controller_contract_test.o`
- `src/build/obj/debug/wspr_transmit_backend_si5351.o`

All other components had empty ignored lists. No component had an untracked
file. Future ignored files remain ineligible for import even if not listed here;
future untracked files fail the clean gate and require review rather than silent
exclusion.

## Sole approved tracked Phase A exclusion

The only tracked file omitted from a raw component tree is:

- source path: `src/WSPR-Transmitter/src/.codex`
- component-relative path: `src/.codex`
- mode: `100644`
- blob: `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`
- size: 0 bytes
- reason: empty local-tool-state residue with no product content
- handling: preserve it in the component's temporary evidence directory and
  compute the expected tree with only this path removed

No wildcard exclusion for `.codex`, `.agents`, `.claude`, `.impeccable`, or any
other tool directory is approved. Any additional tracked path requires its own
reviewed entry.

## Intentional tracked metadata to retain

The following path resembles local tool state but is intentional product design
metadata and must remain byte-for-byte in the raw UI snapshot:

- path: `WsprryPi-UI/.impeccable/design.json`
- component-relative path: `.impeccable/design.json`
- mode: `100644`
- blob: `2ad1636ac96ea5a048417ec7cc81c2e8a9211771`
- size: 5,329 bytes
- reason: tracked UI design-system definition required by the repository's
  Impeccable review workflow

The UI's vendored frontend assets and WSPR-Reference's
`include/nlohmann/json.hpp` are also mandatory retained product content. Their
third-party notices are obligations, not exclusion candidates.

## Phase B changes are not exclusions

The following reviewed adaptations occur only after Phase A tree equivalence is
recorded and must never be used to claim a smaller raw source tree:

- consolidation of redundant, wholly owned component `LICENSE.md` files;
- addition or retention of required third-party license and notice files;
- standalone build-name fixes;
- active documentation corrections;
- workflow relocation or equivalent parent CI coverage;
- removal of obsolete `.gitmodules` entries after all component conversions;
- other behavior-neutral integration edits expressly allowed by the contract.

Record each Phase B change separately in final provenance and review evidence.

## Gate use

Immediately before each conversion, recompute ignored and untracked lists, scan
the tracked tree for tool-state paths, and compare the result with this register.
Continue only when:

- the component has no untracked files;
- every ignored path remains outside the object-derived archive;
- no nested Git administration is present in the archive;
- the sole tracked exclusion, if applicable, matches its recorded blob and
  size;
- every intentional retained path matches its recorded blob and size.
