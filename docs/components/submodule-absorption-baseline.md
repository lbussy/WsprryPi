# Issue 415 Submodule Absorption Baseline

Status: Pre-migration evidence

Observed: 2026-08-16

Contract: [`../plans/submodule-absorption-contract.md`](../plans/submodule-absorption-contract.md)

## Purpose

This manifest freezes the clean, parent-recorded component state that Issue 415
will absorb. It is evidence for the Phase A raw-tree comparisons; it does not
authorize conversion and is not the final provenance record.

## Parent baseline

- checkout: `/Users/lbussy/GitHub/WsprryPi`
- repository: `https://github.com/WsprryPi/WsprryPi.git`
- branch: `codex/issue-415-submodule-absorption`
- upstream: `origin/codex/issue-415-submodule-absorption`
- ahead/behind: `0/0`
- HEAD: `35a119cf96bb7d68e1f50076c762a6519eb97d05`
- HEAD tree: `cd20faadb4f1ef6f46921b24168146adb3158eae`
- `.gitmodules` blob: `841b0d479cedc335ecbab48801458f425538fd28`
- parent index and working tree: clean
- component worktrees: clean, initialized, and gitlink-aligned
- untracked files in the parent and components: none
- nested submodules: none

The parent contained 68 ignored local artifacts, including `.DS_Store` and
ordinary `src/build/` output. They are not part of any component snapshot and
must not be imported or staged.

## Reproduction method

For each retained path, the component commit is the mode `160000` object in the
parent index. The tree OID is:

```sh
git -C PATH rev-parse 'HEAD^{tree}'
```

The inventory digest is SHA-256 over the exact newline-delimited tracked path
list emitted by Git, including its final newline:

```sh
git -C PATH ls-tree -r --name-only HEAD | shasum -a 256
```

The tree OID is the authoritative raw-content identity because it covers tracked
paths, modes, and blob identities. The inventory digest is a diagnostic aid for
detecting path-set changes.

## Component identities

| Retained path | Parent gitlink and checked-out HEAD | Tree OID | State | Files | Inventory SHA-256 |
| --- | --- | --- | --- | ---: | --- |
| `WsprryPi-UI` | `bd9f14c9194754db5d1380ef2874cfa0ccbfd675` | `a8f824e6545d5d40c5fe4e4acd7571b84a1c2d36` | `main` | 90 | `3df86fbdabc5d0f2cfc0f5a78db6ebc78cef9597b0952ee4e43147e7a5997ba0` |
| `src/INI-Handler` | `7339fd70040bfec5a7d19c3a8fc206b1c032657d` | `c983078fbabbedf0b1271a0b083abbdfe7d2f90e` | detached | 9 | `40f826d83aec76668e1d0dbbd039e595fb2f2005877845aafd4da8ee5359cb70` |
| `src/LCBLog` | `a5d74b837e663d8740bc9aba4e3e712cd2d84308` | `94e4e24439e03dbf3b22e3781037b6c9e92ffdff` | detached | 9 | `1ef60d1206b8823d552f6af1158fc508374972cf83a3be6e97263b048613b87a` |
| `src/Mailbox` | `73943ec32448715a098b784368b74b6e315c2fcc` | `8a7e7ead9a0b47930b994225bff76601834600ab` | detached | 9 | `781981996256b16883d87fa83a699ab79b466cfe41a196133458edccaf00436c` |
| `src/MonitorFile` | `82588ffd565e6aeedc235bd724f1066d40de2382` | `818d2cf9b61f3d4cb5cb7cec6a7bebe0eab0a0e2` | detached | 8 | `c5caf9f1f4fd5a291736099316d23e9fe781ba4f6f03fd1c53ddb301e75f0263` |
| `src/PPM-Manager` | `e60a6f287e4f56790b3a30d9888b1e60ec7b3e49` | `b9ffe89f5bd75c1d54f19203c2c23eb80005e10f` | detached | 9 | `230519b7f75aa09ef59eb61bb4ec39de3ed54a1547a21e29c2b0ba390437d223` |
| `src/Signal-Handler` | `c32118ae4494e49ea110e94e7bbc8c6ac66c2549` | `36f1bfc250ec48de89e45d90c8bce031f3cd71bf` | detached | 8 | `bbb6817f1c6d9d4af7fed4c0c7d07a6c0957df40bddd6893c4817e5d1185c9a5` |
| `src/Singleton` | `f4badd58699c3ba645d5ab537d50888c2881c95d` | `17dfa91cc556c42db7895542e6168c8ca9667a11` | detached | 7 | `a733048cb8b1bb777704fc6a0ae39df3590181b1d76b0de11095e1f85ed31391` |
| `src/WSPR-Transmitter` | `d2e329639e907f735531e48f202263d353320035` | `cc0ca41ad8345369f5c9ff6725e6b03b7be64c40` | detached | 80 | `c300489353f4c0368f5c455fbe59e105b1ee837c4ecf38eea798eed5d14f6762` |
| `src/WSPR-Reference` | `2814fee75d394c404edd0e89ebd568b7e8b5e2d7` | `32e05082087a58140bd33738e654128c58c7f813` | detached | 71 | `baccb76a5910af6750f77e531b33813baee2306804521146b1d3f1ff358aa770` |

## Source and commit metadata

| Retained path | Original repository | Commit date | Subject | Tag or version evidence |
| --- | --- | --- | --- | --- |
| `WsprryPi-UI` | `https://github.com/WsprryPi/WsprryPi-UI.git` | `2026-08-12T07:29:59-05:00` | Gate RP1 GPIO operator controls | no tags; `bd9f14c` |
| `src/INI-Handler` | `https://github.com/WsprryPi/INI-Handler.git` | `2026-08-09T19:11:14-05:00` | Support atomic removal of migrated INI keys | no tags; `7339fd7` |
| `src/LCBLog` | `https://github.com/WsprryPi/LCBLog` | `2026-07-06T09:55:02-05:00` | Normalize .gitignore | no tags; `a5d74b8` |
| `src/Mailbox` | `https://github.com/WsprryPi/Mailbox.git` | `2026-07-06T09:55:10-05:00` | Normalize .gitignore | repository tag `2.0.0`; HEAD `73943ec` |
| `src/MonitorFile` | `https://github.com/WsprryPi/MonitorFile.git` | `2026-07-06T09:55:17-05:00` | Normalize .gitignore | no tags; `82588ff` |
| `src/PPM-Manager` | `https://github.com/WsprryPi/PPM-Manager.git` | `2026-08-09T19:11:28-05:00` | Expose qualified chrony correction snapshots | no tags; `e60a6f2` |
| `src/Signal-Handler` | `https://github.com/WsprryPi/Signal-Handler.git` | `2026-07-06T09:55:32-05:00` | Normalize .gitignore | no tags; `c32118a` |
| `src/Singleton` | `https://github.com/WsprryPi/Singleton.git` | `2026-07-06T09:55:40-05:00` | Normalize .gitignore | no tags; `f4badd5` |
| `src/WSPR-Transmitter` | `https://github.com/WsprryPi/WSPR-Transmitter.git` | `2026-08-16T10:29:15-05:00` | Qualify legacy 2200 m keyed modes | no tags; `d2e3296` |
| `src/WSPR-Reference` | `https://github.com/WsprryPi/WSPR-Reference.git` | `2026-07-06T09:55:49-05:00` | Normalize .gitignore | `v0.18.3-17-g2814fee` |

## License and ownership baseline

Every component has a tracked `LICENSE.md` containing the MIT license and naming
Lee Bussy as copyright holder. The history and header audit found only Lee Bussy
contributor identities. Phase A must retain each component license until raw
tree equivalence is established. Phase B may consolidate those redundant
component license files under the parent MIT license, subject to these retained
third-party obligations:

- `WsprryPi-UI/data/vendor/` contains Bootswatch Zephyr 5.3.8, Bootstrap 5.3.8,
  Bootstrap Icons 1.11.3, Font Awesome Free 6.5.0, jQuery 3.7.1, Barlow Semi
  Condensed, and Source Sans 3. Preserve embedded attribution and provide the
  applicable MIT, SIL Open Font License 1.1, and Font Awesome notices.
- `src/WSPR-Reference/include/nlohmann/json.hpp` exactly matches upstream
  `nlohmann/json` commit `f8eee1bb7953c6a4bff384d45052d5acc3d69698`, with
  SHA-256 `acaa0c0e8cb75bbb2001ef3312549140f0ede093cf6f772683b612db75ecd004`.
  Preserve its embedded notices and provide the applicable MIT, CC0-1.0, and
  Apache-2.0 license texts.
- Mailbox contains no Broadcom-authored code at the recorded revision. Broadcom
  is historical lineage only.
- `src/WSPR-Transmitter/external/` contains Lee-authored integration stubs, not
  a third-party library.

## Deliberate exclusions and special tracked paths

No untracked component files exist. These ignored local build products must not
be included in a snapshot:

- `src/INI-Handler`: `src/build/bin/ini-handler_test` and debug objects under
  `src/build/obj/debug/`.
- `src/PPM-Manager`: `src/build/bin/ppm-manager_test` and debug objects under
  `src/build/obj/debug/`.
- `src/WSPR-Transmitter`: binaries under `src/build/bin/`, dependency files
  under `src/build/dep/`, and objects under `src/build/obj/debug/`.
- all other components: no ignored or untracked files observed.

Two tracked tool-state paths have explicit dispositions:

- retain `WsprryPi-UI/.impeccable/design.json` as intentional product design
  metadata;
- exclude the empty `src/WSPR-Transmitter/src/.codex` residue and record that
  Phase A exception in final provenance.

## Gate use

Immediately before converting any component, repeat the clean/SHA gate and
recompute that component's gitlink, tree OID, tracked-file count, and inventory
digest. A mismatch invalidates this baseline for that component and stops the
conversion pending review. Final `docs/components/provenance.md` must carry
forward the actual import evidence and all documented exclusions.
