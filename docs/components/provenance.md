# Imported Component Provenance

Status: Incremental Issue 415 migration record

Contract: [`../plans/submodule-absorption-contract.md`](../plans/submodule-absorption-contract.md)

This document records snapshot imports from the former component repositories.
Each entry is completed and reviewed with its component migration slice. The
history strategy is snapshot import: former repository histories and remotes
remain untouched historical references and are not merged into WsprryPi.

## INI-Handler

| Field | Recorded value |
| --- | --- |
| Component | INI-Handler |
| Retained path | `src/INI-Handler` |
| Original repository | `https://github.com/WsprryPi/INI-Handler.git` |
| Parent-recorded import SHA | `7339fd70040bfec5a7d19c3a8fc206b1c032657d` |
| Checked-out SHA | `7339fd70040bfec5a7d19c3a8fc206b1c032657d` |
| Observed state | detached HEAD; `git submodule status` described the commit as `heads/main` |
| Most recent commit | `2026-08-09T19:11:14-05:00` — `Support atomic removal of migrated INI keys` |
| Version/tag provenance | No repository tags contain or point at the imported revision; short revision `7339fd7` |
| Raw source tree | `c983078fbabbedf0b1271a0b083abbdfe7d2f90e` |
| Expected and staged Phase A tree | `c983078fbabbedf0b1271a0b083abbdfe7d2f90e` |
| Staged tree after Phase B | `5889c25a2384c86c2efbfcaf74041278907a2ed8` |
| Phase A archive | SHA-256 `2b02fadd16f7ad6c3a5d48b906fa4bd3beb5d83f5d466fa857a7c3dfa8f4a2e7`; 9 tracked files and 2 directory entries |
| Former component license | `src/INI-Handler/LICENSE.md` at raw import; MIT, copyright 2025–2026 Lee Bussy; blob `b0b8a0f9d2bdb41a17a6368d1fc155e0470f21c9` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license was removed in Phase B after the ownership audit found only Lee Bussy contributor identities and no third-party obligation in this component. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`40f826d83aec76668e1d0dbbd039e595fb2f2005877845aafd4da8ee5359cb70`.
The object-derived archive contained the exact recorded tree, including the
component license, before any adaptation. The ordinary staged Phase A subtree
OID equaled the source tree OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. These three ignored build products were deliberately excluded because
they were absent from the recorded tree:

- `src/build/bin/ini-handler_test`
- `src/build/obj/debug/ini_file.o`
- `src/build/obj/debug/main.o`

Their newline-delimited path list had SHA-256
`838b15ec18798aff09ea06d14e7e73fbaab83eb0c209eea4846a285c13b005a8`.
There were no untracked files, nested submodules, nested Git administration, or
other exclusions.

### Phase B adaptations

- Fixed standalone output naming as `ini-handler` and `ini-handler_test`
  instead of deriving it from `remote.origin.url`.
- Changed the standalone test harness to use only the retained
  `test/test.ini` fixture as read-only input and to create, mutate, and remove
  its working copies in a unique system temporary directory. It no longer
  prefers an installed `/usr/local/etc/wsprrypi.ini` file or writes derivatives
  beside the tracked fixture.
- Corrected the stale standalone write assertion and README description to
  match the existing production behavior that persists explicitly added
  sections and keys. No production implementation was changed.
- Removed four trailing spaces from Makefile license-comment blank lines so the
  final parent staged diff passes Git's whitespace check; this has no build or
  runtime effect.
- Removed the redundant component-level MIT license after adopting the parent
  MIT license under the approved ownership disposition, and redirected the
  component source and README license references to the parent license.

Production `ini_file.cpp` and `ini_file.hpp`, the public interface, atomic
update behavior, source hierarchy, fixture, and parent integration were not
changed. The source-file changes above affect license comments only; the README
change is limited to the behavior and licensing corrections recorded above.

### Standalone build, test, and extraction

From `src/INI-Handler/src`, the standalone entry points are `make release`,
`make debug`, and the unprivileged hardware-free `make test SUDO=`. The expected
outputs are `build/bin/ini-handler` and `build/bin/ini-handler_test`.

To extract the component later, copy `src/INI-Handler` into a new repository,
add the desired repository metadata and license file, and run the standalone
build/test commands from its `src` directory. The component has no dependency
on WsprryPi headers, globals, services, hardware, or parent Git metadata.

### Slice validation

The default standalone Makefile command was attempted on macOS and rejected by
Apple Clang because the Linux-oriented `-lstdc++fs` input is unused under
`-Werror`. A supplemental macOS build with only platform-incompatible flags
overridden successfully produced `ini-handler` and `ini-handler_test`; the full
unprivileged test passed. The retained fixture SHA-256 remained
`5194611c74aa3b59624f12a6f9433bd7c5d8ecf5baba69fefff5770c3934662b`
before and after the run, the unique temporary directory was removed, and
generated build output was cleaned before staging.

Authoritative default-command validation on Debian and synchronized `wspr4`,
parent-wide regression/CI, final staged export, and post-commit fresh-clone
acceptance remain deferred to their contract gates.
