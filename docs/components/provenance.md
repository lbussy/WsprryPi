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

## LCBLog

| Field | Recorded value |
| --- | --- |
| Component | LCBLog |
| Retained path | `src/LCBLog` |
| Original repository | `https://github.com/WsprryPi/LCBLog` |
| Parent-recorded import SHA | `a5d74b837e663d8740bc9aba4e3e712cd2d84308` |
| Checked-out SHA | `a5d74b837e663d8740bc9aba4e3e712cd2d84308` |
| Observed state | detached HEAD; `git submodule status` described the commit as `heads/main` |
| Most recent commit | `2026-07-06T09:55:02-05:00` — `Normalize .gitignore` |
| Version/tag provenance | No repository tags contain or point at the imported revision; short revision `a5d74b8` |
| Raw source tree | `94e4e24439e03dbf3b22e3781037b6c9e92ffdff` |
| Expected and staged Phase A tree | `94e4e24439e03dbf3b22e3781037b6c9e92ffdff` |
| Staged tree after Phase B | `766e636ceb4c8912480c168a27fec6df2252b547` |
| Phase A archive | SHA-256 `a5869ff83ad8e61a481227ff2c8a162f23872bfddf8f8130bebae724e05cea0e`; 9 tracked files and 2 directory entries |
| Former component license | `src/LCBLog/LICENSE.md` at raw import; MIT, copyright 2025–2026 Lee Bussy; blob `b0b8a0f9d2bdb41a17a6368d1fc155e0470f21c9` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license was removed in Phase B after the ownership audit found only Lee Bussy contributor identities and no third-party obligation in this component. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`1ef60d1206b8823d552f6af1158fc508374972cf83a3be6e97263b048613b87a`.
The object-derived archive contained the exact recorded tree, including the
component license, before any adaptation. The ordinary staged Phase A subtree
OID equaled the source tree OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. There were no ignored files, untracked files, nested submodules,
nested Git administration, or other exclusions. LCBLog's tracked `.gitignore`
contains `lcblog*`, which initially hid its three tracked library files from an
ordinary parent `git add`. Force-adding only the exact archive-restored tree
restored Phase A equality; the snapshot method now records this required case.

### Phase B adaptations

- Fixed standalone output naming as `lcblog` and `lcblog_test` instead of
  deriving it from `remote.origin.url`.
- Removed unused GPIO pkg-config/linker and preprocessor remnants from the
  standalone Makefile. LCBLog itself has no GPIO dependency; optional journald
  support remains intact.
- Removed the redundant component-level MIT license after adopting the parent
  MIT license. Source comments refer generically to the repository-root license,
  and the README records the monorepo and extraction dispositions.
- Updated README paths, standalone build instructions, license wording, and
  practical extraction guidance without changing the logging API or behavior.

The production library remains the cohesive set `src/lcblog.cpp`,
`src/lcblog.hpp`, and `src/lcblog.tpp`. No WsprryPi header, global,
configuration, runtime-service, hardware, or directory-layout dependency was
introduced.

### Standalone build, test, and extraction

From `src/LCBLog/src`, the standalone entry points are `make release`,
`make debug`, and the bounded unprivileged hardware-free `make test SUDO=`. The
expected outputs are `build/bin/lcblog` and `build/bin/lcblog_test`. Journald is
optional and detected through `libsystemd`; stdout/stderr operation remains the
portable default when it is unavailable.

To extract LCBLog, copy `src/LCBLog` alone into a new repository, add repository
metadata and a license file, then run the standalone commands from its `src`
directory. WsprryPi-specific integration must remain outside the extracted
component.

### Slice validation

The default standalone command reached compilation on macOS but Apple Clang
rejected GCC's `-fmax-errors=10` under `-Werror`; its next diagnostic also
required suppressing Apple Clang's `-Wpessimizing-move` for the existing
template implementation. Supplemental macOS debug, test, and release builds
passed with only platform/toolchain flags and unavailable Linux libraries
overridden. The test was bounded, unprivileged, and used stdout only.

A Git-free temporary extraction containing only `src/LCBLog` independently
built and passed the same hardware-free test. Parent dry-run source discovery
includes `LCBLog/src/lcblog.cpp`, excludes the standalone `main.cpp`, and the
production library files contain no reference to WsprryPi or another component.
Generated build output was cleaned before staging.

Authoritative default-command validation on Debian and synchronized `wspr4`,
optional journald coverage, parent-wide regression/CI, final staged export, and
post-migration fresh-clone acceptance remain deferred to their contract gates.

## Mailbox

| Field | Recorded value |
| --- | --- |
| Component | Mailbox |
| Retained path | `src/Mailbox` |
| Original repository | `https://github.com/WsprryPi/Mailbox.git` |
| Parent-recorded import SHA | `73943ec32448715a098b784368b74b6e315c2fcc` |
| Checked-out SHA | `73943ec32448715a098b784368b74b6e315c2fcc` |
| Observed state | detached HEAD; `git submodule status` described the commit as `heads/main` |
| Most recent commit | `2026-07-06T09:55:10-05:00` — `Normalize .gitignore` |
| Version/tag provenance | Repository tag `2.0.0` points to `44667d6ef634be7baf492b751cbc825c07a7ce69`; it is not an ancestor of the imported revision, so the import is identified by exact SHA rather than that version |
| Raw source tree | `8a7e7ead9a0b47930b994225bff76601834600ab` |
| Expected and staged Phase A tree | `8a7e7ead9a0b47930b994225bff76601834600ab` |
| Staged tree after Phase B | `c4d317065df349a9f87227427cfb5afbdb9a53b9` |
| Phase A archive | SHA-256 `ec9900382ee13d58a013768f71174e11acd46ccb3867e68bda2902d3672ead72`; 9 tracked files and 2 directory entries |
| Former component license | `src/Mailbox/LICENSE.md` at raw import; MIT, copyright 2025–2026 Lee Bussy; blob `b0b8a0f9d2bdb41a17a6368d1fc155e0470f21c9` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license was removed after the ownership/header audit confirmed no current Broadcom or other third-party code. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`781981996256b16883d87fa83a699ab79b466cfe41a196133458edccaf00436c`.
The object-derived archive contained the exact recorded tree and license before
adaptation. Its ordinary staged Phase A subtree OID equaled the source OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. There were no ignored files, untracked files, nested submodules,
nested Git administration, or other exclusions.

### Ownership, licensing, and historical lineage

The current Mailbox implementation and file headers are Lee Bussy work. Earlier
Broadcom mailbox software is historical design lineage only; no Broadcom source,
legacy `old_mailbox.c`, or Broadcom-derived notice file is present in the
recorded tree. The snapshot therefore creates no separate Broadcom licensing
obligation. The former repository remains an untouched historical reference.

### Phase B adaptations

- Fixed standalone outputs as `mailbox` and `mailbox_test` rather than deriving
  their names from the parent Git remote.
- Changed ordinary `make test` into a hardware-free build-only check. Live demo
  execution is retained only as `make live-test MAILBOX_LIVE_TEST=YES`, with an
  explicit refusal guard; the `gdb` target has the same guard. Neither was run
  during Issue 415.
- Replaced obsolete submodule, Broadcom-repository, legacy-shim, and license
  instructions with the retained monorepo layout, current API, live-device
  boundary, extraction guidance, and accurate historical-lineage statement.
- Removed the redundant component MIT license and redirected source comments to
  the repository-root license.
- Removed five pre-existing trailing spaces in Makefile comments/flags so the
  parent staged whitespace check passes; this has no build or runtime effect.

Production `bcm_model.hpp`, `mailbox.cpp`, and `mailbox.hpp` differ from the raw
tree only in their license comments. Their API, device paths, I/O behavior, and
implementation are unchanged. The standalone live demo remains in `main.cpp`.

### Standalone build, validation, and extraction

From `src/Mailbox/src`, `make debug`, `make release`, and `make test` compile
`mailbox_test` or `mailbox` without executing a device operation. The explicit
`live-test` target opens `/dev/vcio` and `/dev/mem` and is outside ordinary
validation.

To extract Mailbox, copy `src/Mailbox`, add repository metadata and a license
file, then integrate `mailbox.cpp`, `mailbox.hpp`, and `bcm_model.hpp`. Retain
the guarded live demo and do not represent build-only evidence as Raspberry Pi
mailbox qualification.

### Slice validation

Make dry-runs proved that ordinary `make test` compiles and reports a
hardware-free check without invoking `mailbox_test`, while `live-test` contains
both the explicit `MAILBOX_LIVE_TEST=YES` guard and the demo invocation. The
live target was not run.

The macOS compile reached the first platform boundary and failed because Apple
Clang cannot find Linux-only `linux/ioctl.h`. This is the expected host
limitation; no source workaround was introduced. A Git-free temporary
extraction reproduced the safe target graph, and parent dry-run discovery
includes `Mailbox/src/mailbox.cpp` while excluding the standalone `main.cpp`.
Generated partial build output was removed before staging.

Authoritative compile/build-only validation on Debian and synchronized `wspr4`,
all mailbox-device behavior, parent-wide regression/CI, final staged export,
and post-migration fresh-clone acceptance remain deferred. No `/dev/vcio`,
`/dev/mem`, mailbox ioctl, mapping, GPIO, I2C, or RF operation occurred.

## MonitorFile

| Field | Recorded value |
| --- | --- |
| Component | MonitorFile |
| Retained path | `src/MonitorFile` |
| Original repository | `https://github.com/WsprryPi/MonitorFile.git` |
| Parent-recorded import SHA | `82588ffd565e6aeedc235bd724f1066d40de2382` |
| Checked-out SHA | `82588ffd565e6aeedc235bd724f1066d40de2382` |
| Observed state | detached HEAD; `git submodule status` described the commit as `heads/main` |
| Most recent commit | `2026-07-06T09:55:17-05:00` — `Normalize .gitignore` |
| Version/tag provenance | No repository tags point at the imported revision; short revision `82588ff` |
| Raw source tree | `818d2cf9b61f3d4cb5cb7cec6a7bebe0eab0a0e2` |
| Expected and staged Phase A tree | `818d2cf9b61f3d4cb5cb7cec6a7bebe0eab0a0e2` |
| Staged tree after Phase B | `d8135fb37b6fcf0fc7ca13c1ac66cee31024f0c9` |
| Phase A archive | SHA-256 `ad87d4a4cc2477a56c18832a39ef89a42cb7c557de831df4bb4c123b3977231f`; 8 tracked files and 2 directory entries |
| Former component license | `src/MonitorFile/LICENSE.md` at raw import; MIT, copyright 2025–2026 Lee Bussy; blob `b0b8a0f9d2bdb41a17a6368d1fc155e0470f21c9` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license was removed after the ownership and header audit found only Lee Bussy attribution and no third-party obligation. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`c5caf9f1f4fd5a291736099316d23e9fe781ba4f6f03fd1c53ddb301e75f0263`.
The object-derived archive contained the exact recorded tree and component
license before adaptation. Its ordinary staged Phase A subtree OID equaled the
source OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. There were no ignored files, untracked files, nested submodules,
nested Git administration, or other exclusions.

### Phase B adaptations

- Fixed standalone outputs as `monitorfile` and `monitorfile_test`, independent
  of the parent Git remote, and reduced include discovery to the component root.
- Replaced the indefinite, signal-terminated demo with a bounded callback test
  that creates and removes a unique system temporary directory. The production
  MonitorFile API and implementation are unchanged apart from license comments.
- Replaced obsolete clone, contribution, and component-license instructions
  with monorepo build/use, safety-boundary, and extraction guidance.
- Removed the redundant component MIT license and redirected source comments to
  the repository-root license.
- Removed four pre-existing trailing spaces in the Makefile license block so
  the parent staged whitespace check passes.

### Standalone build, validation, and extraction

From `src/MonitorFile/src`, `make release` builds `monitorfile` and `make test`
builds and runs `monitorfile_test`. The test uses only filesystem and threading
facilities, waits at most three seconds for its callback, calls `stop()`, and
removes its temporary directory.

To extract MonitorFile, copy `src/MonitorFile`, add repository metadata and a
license file, then compile `monitorfile.cpp` with `monitorfile.hpp`. No parent
application internals or directory layout are required.

### Slice validation

The default macOS build reached the known Makefile portability boundary:
Apple Clang rejects the GNU-oriented `-lstdc++fs` compilation flag under
`-Werror`. A supplemental host-only invocation overriding GNU-only flags with
`COMMON_FLAGS='-Wall -Werror -MMD -MP' COMM_CXX_FLAGS='-std=c++20'
LDFLAGS='-lpthread'` built and ran `monitorfile_test` successfully. It exited
zero within two seconds and left neither build output nor a temporary test
directory in the checkout or `/private/tmp`.

A Git-free temporary extraction and parent dry-run source discovery are checked
in this slice. Authoritative default-Makefile validation on Debian and
synchronized `wspr4`, parent-wide regression/CI, final staged export, and
post-migration fresh-clone acceptance remain deferred. No scheduling-priority,
service, installation, device, GPIO, I2C, RF, or hardware operation occurred.

## PPM-Manager

| Field | Recorded value |
| --- | --- |
| Component | PPM-Manager |
| Retained path | `src/PPM-Manager` |
| Original repository | `https://github.com/WsprryPi/PPM-Manager.git` |
| Parent-recorded import SHA | `e60a6f287e4f56790b3a30d9888b1e60ec7b3e49` |
| Checked-out SHA | `e60a6f287e4f56790b3a30d9888b1e60ec7b3e49` |
| Observed state | detached HEAD; `git submodule status` described the commit as `heads/main` |
| Most recent commit | `2026-08-09T19:11:28-05:00` — `Expose qualified chrony correction snapshots` |
| Version/tag provenance | No repository tags contain or point at the imported revision; short revision `e60a6f2` |
| Raw source tree | `b9ffe89f5bd75c1d54f19203c2c23eb80005e10f` |
| Expected and staged Phase A tree | `b9ffe89f5bd75c1d54f19203c2c23eb80005e10f` |
| Staged tree after Phase B | `abdfb5967a83b46938f5419b905969d03c159545` |
| Phase A archive | SHA-256 `7c357e4b2271eb3ce993eff5a626087104c85b8d9d3d54edcedffc3c2bcfb299`; 9 tracked files and 2 directory entries |
| Former component license | `src/PPM-Manager/LICENSE.md` at raw import; MIT, copyright 2025–2026 Lee Bussy; blob `b0b8a0f9d2bdb41a17a6368d1fc155e0470f21c9` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license was removed after the ownership and header audit found only Lee Bussy contributor identities and no third-party obligation. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`6a32c6075d8b258a29d0ec54ef6591018a20193d0b64162829f9c7e6639fb107`.
The object-derived archive contained the exact recorded tree, including the
weighting discussion and component license, before adaptation. Its ordinary
staged Phase A subtree OID equaled the source tree OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. These ignored build products were deliberately excluded because they
were absent from the recorded tree:

- `src/build/bin/ppm-manager_test`
- `src/build/obj/debug/main.o`
- `src/build/obj/debug/ppm_manager.o`

Their newline-delimited path-list SHA-256 was
`ba249d97629071838f6a321e32765813c77dcc2263f34e4a223fecdde6b132b5`.
There were no untracked files, nested submodules, nested Git administration, or
other exclusions.

### Phase B adaptations

- Fixed standalone outputs as `ppm-manager` and `ppm-manager_test` rather than
  deriving them from the parent Git remote.
- Preserved the live Chrony demonstration in `main.cpp`, but separated it from
  a new fixture-driven `test.cpp`. Ordinary `make test` calls only the static
  provider-report parser. The live demo is available solely through
  `make live-test PPM_MANAGER_LIVE_TEST=YES` and was not executed.
- Excluded the standalone `test.cpp` from parent source discovery so it cannot
  introduce a second `main()` into the WsprryPi application link.
- Replaced obsolete submodule, source-deletion, installation, and component
  license instructions with monorepo build/use, provider boundary, guarded live
  diagnostic, and extraction guidance.
- Removed the redundant component MIT license and redirected production source
  comments to the repository-root license.

The production provider API and implementation differ from the raw tree only
in their license comments. Chrony commands, snapshot semantics, update-loop
behavior, callbacks, scheduler behavior, and parent integration are unchanged.
`PPM_Weighting_Discussion.md` is byte-for-byte unchanged.

### Standalone build, validation, and extraction

From `src/PPM-Manager/src`, `make release` builds the retained live demo,
`make test` builds and runs the hardware-free fixture test, and `make live-test
PPM_MANAGER_LIVE_TEST=YES` explicitly opts into live Chrony queries and an
operator-terminated wait. The ordinary test covers a qualified mixed-source
snapshot and provider-unavailable parsing without invoking `chronyc`,
`systemctl`, scheduler changes, or external processes.

To extract PPM-Manager, copy `src/PPM-Manager`, add repository metadata and a
license file, and retain `ppm_manager.cpp`, `ppm_manager.hpp`, the Makefile,
demo, parser test, README, and weighting discussion. The provider interface has
no dependency on WsprryPi application internals.

### Slice validation

The default macOS build reached the known GNU-oriented Makefile boundary:
Apple Clang rejects `-lstdc++fs` during compilation under `-Werror`. A
supplemental host-only invocation with GNU-only flags removed built and ran
`ppm-manager_test` successfully. The retained demo additionally requires
`-Wno-deprecated-volatile` on this Apple Clang because its historical worker
loop increments a volatile integer; the source was preserved.

A Git-free staged extraction, guarded-live-target review, and parent dry-run
source discovery are checked in this slice. Authoritative default-Makefile
validation on Debian and synchronized `wspr4`, live Chrony behavior,
parent-wide regression/CI, final staged export, and post-migration fresh-clone
acceptance remain deferred. No `chronyc`, `systemctl`, clock configuration,
scheduler-priority, service, installation, device, GPIO, I2C, RF, or hardware
operation occurred.
