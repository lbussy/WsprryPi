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

## WsprryPi-UI

| Field | Recorded value |
| --- | --- |
| Component | WsprryPi-UI |
| Retained path | `WsprryPi-UI` |
| Original repository | `https://github.com/WsprryPi/WsprryPi-UI.git` |
| Parent-recorded import SHA | `bd9f14c9194754db5d1380ef2874cfa0ccbfd675` |
| Checked-out SHA | `bd9f14c9194754db5d1380ef2874cfa0ccbfd675` |
| Observed state | `main`, tracking `origin/main`, clean and synchronized with the parent gitlink |
| Most recent commit | `2026-08-12T07:29:59-05:00` — `Gate RP1 GPIO operator controls` |
| Version/tag provenance | No repository tag points at or contains the imported revision. Runtime UI version display remains derived from the parent WsprryPi executable. |
| Raw source tree | `a8f824e6545d5d40c5fe4e4acd7571b84a1c2d36` |
| Expected and staged Phase A tree | `a8f824e6545d5d40c5fe4e4acd7571b84a1c2d36` |
| Staged tree after Phase B | `1157a0646ca98abef63d262393736795ab1f1dc0` |
| Phase A archive | SHA-256 `5388ee5e50abd9521b9d9f543051caf5e467207cded1218039e7801385d638f5`; 90 tracked files |
| Former component license | `WsprryPi-UI/LICENSE.md` at raw import; MIT, copyright 2023-2025 Lee C. Bussy; blob `8f76068ed640c62116659925c0ece878f7d8b286` |
| License after absorption | Parent `LICENSE.md` (MIT) for WsprryPi-authored UI code. Independent vendor terms are retained with the deployed assets in `WsprryPi-UI/data/vendor/THIRD_PARTY_NOTICES.md` and `WsprryPi-UI/data/vendor/licenses/OFL-1.1.txt`. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`e94ee1e99cc0d9d8e7fd09404829dc4699e5cd7f79483b5fa7735a9377969e06`.
The object-derived archive and ordinary staged Phase A subtree both exactly
matched the recorded source tree before adaptation.

The live submodule's `.git` administrative directory link was not tracked and
was not imported. There were no ignored files, untracked files, nested
submodules, or nested Git administration. No tracked product file was excluded.
In particular, `.impeccable/design.json` was retained byte-for-byte at blob
`2ad1636ac96ea5a048417ec7cc81c2e8a9211771`, and the tracked `data/cache/`
snapshots were retained as part of the authoritative recorded tree.

### Ownership and third-party licensing

The contribution history contains only Lee Bussy identity variants, so the
component's redundant first-party MIT license was consolidated under the parent
license after Phase A verification. Vendored assets retain their embedded
attribution and exact imported bytes:

- Bootswatch Zephyr/Bootstrap 5.3.8 CSS:
  `0eb1079dbae68677d81044a09023da7a28e6748f3462d5c9276fe4ce745d8820`;
- Bootstrap 5.3.8 bundle:
  `c6f670216aedd2c61ec83102f7e40c1c44114eecd7f37b2edc332757390c386a`;
- jQuery 3.7.1:
  `fc9a93dd241f6b045cbff0481cf4e1901becd0e12fb45166a8f17f95823f0b1a`;
- Bootstrap Icons 1.11.3 CSS/font:
  `fc499030409f5641ca47078ac415ff7ed39308a2aebdff8abf783b167a18ae24` /
  `476adf42b40325098fcfa8b36ab3e769186bb4f6ce6a249753e2e1a9c22bf99e`;
- Font Awesome Free 6.5.0 CSS:
  `3219148441227c82ff4cbcbc9deab1bdbcb9c2b1535377a3ee6ccb3edf1abcd4`,
  with its four imported webfont hashes recorded by the slice audit; and
- Barlow Semi Condensed and Source Sans 3 font files, whose seven exact hashes
  are recorded by the slice audit and whose embedded families/foundries match
  the declared packages.

The added notice set carries the applicable Bootswatch, Bootstrap, Bootstrap
Icons, jQuery, and Font Awesome MIT notices plus the full SIL Open Font License
1.1 and copyright/reserved-name notices for Font Awesome, Barlow Semi Condensed,
and Source Sans 3. No Font Awesome SVG or JavaScript icon artifact is bundled,
so the CC BY icon-file license does not apply to the retained asset types.

### Phase B adaptations

- Preserved the complete `data/` deployment tree, tests, README, API/design/
  product documentation, design-system metadata, vendor assets, paths, runtime
  configuration, validation, persistence, wording, navigation, light/dark
  themes, responsive rules, and installed web-root layout.
- Removed only the redundant Lee-owned component MIT license and added the
  distributable third-party notice/license set required for the retained vendor
  assets.
- Removed trailing spaces from two JavaScript lines so the staged whitespace
  gate passes without changing executable semantics. Repository attributes
  preserve intentional Markdown hard line breaks in `API_syslog.md` and
  `DESIGN.md` without modifying those files.
- Added the six dependency-free Node/PHP UI regressions to the active parent
  Debian non-hardware workflow and added `php-cli` to that workflow's build
  dependencies. Existing parent `semantics-test` continues to run the compiled
  UI/source regression and its parent-side Node checks.

No PHP, JavaScript, CSS, image, font, page copy, interaction, deployment path,
or application behavior was otherwise changed. The excluded Impeccable page
`data/view_diag_logs.php` was not modified.

### Test, rendering, and extraction entry points

The safe component suite consists of:

- `node tests/cw_timing_state_test.js`;
- `node tests/responsive_shell_logs_test.js`;
- `node tests/support_bundle_ui_test.js`;
- `node tests/wspr_band_frequency_correlation_test.js`;
- `php tests/gpio_dropdown_test.php`; and
- `php tests/spot_menu_test.php`.

The Chromium/WebSocket integration tests additionally require Chromium and the
Node `ws` package, for which this component intentionally has no tracked package
manifest. `tests/log_stream_disconnect_integration_test.sh` is a Raspberry Pi
operator-service test requiring `sudo` and `systemctl`; it is outside ordinary
hardware-free migration validation.

To extract the UI, copy `WsprryPi-UI` alone, preserve `data/`, tests,
`.impeccable/design.json`, the vendor notice/license files, assets, and embedded
notices, then add repository metadata and a license for the
first-party UI code. Parent integration continues to consume
`WsprryPi-UI/data` at the same path.

All six dependency-free Node/PHP component tests passed both before and after
conversion. The parent `ui_source_regression_test` compiled directly with
Apple Clang and passed against the absorbed tree. The broader parent
`make semantics-test SUDO=` reached safe compilation but stopped because Apple
Clang rejects the existing GCC-only `-fmax-errors=10` flag under `-Werror`;
canonical Debian CI remains the authoritative run for that target.

A Git-free export of the staged parent index repeated all six UI tests and the
compiled parent UI/source regression successfully. Source/deployment comparison
found no difference except the documented JavaScript trailing-space removals;
paths, files, and runtime content are otherwise preserved.

Impeccable desktop/mobile and light/dark rendering before and after conversion
showed the same layout, copy, controls, unavailable-controller state, and
pre-existing narrow-viewport clipping. The one required detector pass reported
only pre-existing design-token advisories and warnings in unchanged UI/vendor
files; none arose from the absorption adaptations. No aesthetic correction was
made. `data/view_diag_logs.php` remained untouched.

The Chromium/WebSocket tests were not run because `ws` and Chromium are absent
and the component has no tracked dependency manifest. The privileged Raspberry
Pi log-stream/service test was not run. Comprehensive active repository and
operator-documentation reconciliation remains the agreed late migration gate.

## Signal-Handler

| Field | Recorded value |
| --- | --- |
| Component | Signal-Handler |
| Retained path | `src/Signal-Handler` |
| Original repository | `https://github.com/WsprryPi/Signal-Handler.git` |
| Parent-recorded import SHA | `c32118ae4494e49ea110e94e7bbc8c6ac66c2549` |
| Checked-out SHA | `c32118ae4494e49ea110e94e7bbc8c6ac66c2549` |
| Observed state | detached HEAD; `git submodule status` described the commit as `heads/main` |
| Most recent commit | `2026-07-06T09:55:32-05:00` — `Normalize .gitignore` |
| Version/tag provenance | No repository tags contain or point at the imported revision; short revision `c32118a` |
| Raw source tree | `36f1bfc250ec48de89e45d90c8bce031f3cd71bf` |
| Expected and staged Phase A tree | `36f1bfc250ec48de89e45d90c8bce031f3cd71bf` |
| Staged tree after Phase B | `a67135c89bea63317ad91016b205c9c2e5d518fb` |
| Phase A archive | SHA-256 `6261045c0aedc57d22633293250382ba68649b21cddb023f846c8f0ff88a17ee`; 8 tracked files and 2 directory entries |
| Former component license | `src/Signal-Handler/LICENSE.md` at raw import; MIT, copyright 2025–2026 Lee Bussy; blob `b0b8a0f9d2bdb41a17a6368d1fc155e0470f21c9` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license and repeated full-license source headers were consolidated after the ownership audit found only Lee Bussy contributor identities and no third-party obligation. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`4e3271e71f671bc2ce9e476e6f1ee46fe9dbe0f2b83d8d5956e2f92ae6c0a1d9`.
The object-derived archive contained the exact recorded tree and component
license before adaptation. Its ordinary staged Phase A subtree OID equaled the
source tree OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. There were no ignored or untracked files, nested submodules, nested
Git administration, or other exclusions.

### Phase B adaptations

- Fixed standalone outputs as `signal-handler` and `signal-handler_test`
  rather than deriving them from the parent Git remote, and removed an unused
  `libgpiodcxx` link probe from this signal-only component.
- Preserved the interactive demonstration in `main.cpp` while separating a new
  bounded `test.cpp`. The test blocks the handled set, sends one controlled
  `SIGTERM` to its own process, verifies callback metadata and idempotent
  shutdown, and uses no scheduler-priority call.
- Guarded the interactive demonstration behind `make live-test
  SIGNAL_HANDLER_LIVE_TEST=YES`; it was not executed.
- Excluded standalone `test.cpp` from parent source discovery so it cannot add
  a second `main()` to the WsprryPi application.
- Replaced standalone/component-license instructions with monorepo build,
  integration, safety, and extraction guidance. Removed the redundant component
  license and replaced repeated full MIT source headers with root-license
  references and retained copyright attribution.

Production signal sets, callback behavior, thread lifecycle, terminal handling,
scheduler hook, public interface, and application integration are unchanged.

### Standalone build, validation, and extraction

From `src/Signal-Handler/src`, `make release` builds the retained interactive
demo and `make test` builds and runs the bounded self-process signal test. The
live target requires explicit opt-in and retains its scheduler-priority attempt
and operator-terminated workflow.

To extract Signal-Handler, copy `src/Signal-Handler`, add repository metadata
and a license file, and retain `signal_handler.cpp`, `signal_handler.hpp`, the
Makefile, interactive demo, bounded test, and README. The component has no
dependency on WsprryPi application internals.

### Slice validation

The default macOS build first rejects the GNU-oriented `-lstdc++fs` compile
flag under `-Werror`. With GNU-only flags removed, Apple Clang reaches the true
platform boundary: macOS does not provide the production implementation's
POSIX `sigwaitinfo()` API. Consequently neither the standalone executable nor
bounded test can be linked or run on this host, and no portability workaround
was introduced during snapshot absorption.

A Git-free staged-tree completeness check, target-graph review, live-target
guard review, production-source comparison, and parent source-discovery check
are performed in this slice. Compilation and bounded callback/shutdown test
execution on Debian or synchronized `wspr4`, interactive behavior,
parent-wide regression/CI, final staged export, and post-migration fresh-clone
acceptance remain deferred. No signal was sent after the build failure, and no
scheduler-priority, service, installation, device, GPIO, I2C, RF, hardware, or
external-process operation occurred.

## Singleton

| Field | Recorded value |
| --- | --- |
| Component | Singleton |
| Retained path | `src/Singleton` |
| Original repository | `https://github.com/WsprryPi/Singleton.git` |
| Parent-recorded import SHA | `f4badd58699c3ba645d5ab537d50888c2881c95d` |
| Checked-out SHA | `f4badd58699c3ba645d5ab537d50888c2881c95d` |
| Observed state | detached HEAD; `git submodule status` described the commit as `heads/main` |
| Most recent commit | `2026-07-06T09:55:40-05:00` — `Normalize .gitignore` |
| Version/tag provenance | No repository tags contain or point at the imported revision; short revision `f4badd5` |
| Raw source tree | `17dfa91cc556c42db7895542e6168c8ca9667a11` |
| Expected and staged Phase A tree | `17dfa91cc556c42db7895542e6168c8ca9667a11` |
| Staged tree after Phase B | `a3acc0993852ac80668efb1fddb103f6ef223e36` |
| Phase A archive | SHA-256 `85d4701141d1bc78eb6b707b0e28c4ab31b24f567efb673876f246713ea5201d`; 7 tracked files and 2 directory entries |
| Former component license | `src/Singleton/LICENSE.md` at raw import; MIT, copyright 2024 Lee Bussy; blob `30c1bcce2f7b31d2420d0e94aa0c9c67d1e9a216` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license was removed after the ownership audit found only Lee Bussy contributor identities and no third-party obligation. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`52aa0d1627ef8a2447c241896d7d240a11dd7f4a24e472fba58ac3580ccf080d`.
The object-derived archive contained the exact recorded tree and component
license before adaptation. Its ordinary staged Phase A subtree OID equaled the
source tree OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. There were no ignored or untracked files, nested submodules, nested
Git administration, or other exclusions.

### Phase B adaptations

- Retained the existing fixed `singleton` and `singleton_test` naming; no
  Git-derived project naming required correction.
- Preserved the historical fixed-port, child-process, and restricted-port
  program in `main.cpp`, but moved ordinary validation to a new deterministic
  `test.cpp`. It selects an available loopback UDP port, checks first/second
  acquisition, and confirms release and reacquisition.
- Guarded the historical program behind `make demo SINGLETON_DEMO=YES`; it was
  compiled to `singleton_demo` for guard validation but was not executed.
- Excluded standalone `test.cpp` from parent source discovery so it cannot add
  a second `main()` to the WsprryPi application.
- Removed GNU-only `-fmax-errors` and replaced Linux-only `nproc` discovery with
  `getconf _NPROCESSORS_ONLN`, allowing the documented default test command to
  work with both Apple Clang and GNU toolchains without changing test meaning.
- Extended `make clean` to remove dependency and dSYM output produced when the
  retained demo is compiled, preventing ignored build residue in the checkout.
- Replaced component-license and standalone instructions with monorepo build,
  safe testing, integration, and extraction guidance. Removed the redundant
  component license and redirected source comments to the root license.

The header-only singleton socket acquisition and release implementation is
unchanged apart from its license comment.

### Standalone build, validation, and extraction

From `src/Singleton/src`, `make test` builds and runs `singleton_test` using an
ephemeral loopback UDP port. `make demo SINGLETON_DEMO=YES` opts into the
historical demonstration and its fixed-port and child-process assumptions.

To extract Singleton, copy `src/Singleton`, add repository metadata and a
license file, and retain `singleton.hpp`, the Makefile, deterministic test,
historical demonstration, and README. The header has no dependency on WsprryPi
application internals.

### Slice validation

The default `make test` compiled and passed outside the filesystem sandbox. The
sandbox itself denied loopback socket binding, so the identical bounded command
was rerun with loopback permission. It acquired an operating-system-selected
port, rejected a simultaneous second instance, released the socket, and
reacquired it. The demo compiled, then its explicit opt-in guard refused
execution as intended. Generated objects, dependency files, and binaries were
removed afterward.

A Git-free staged export, parent source-discovery check, production-header
comparison, and staged whitespace/scope review are performed in this slice.
Authoritative Debian and synchronized `wspr4` validation, parent-wide
regression/CI, final staged export, and post-migration fresh-clone acceptance
remain deferred. No fixed or privileged port, unrelated process, service,
installation, device, GPIO, I2C, RF, or hardware operation occurred.

## WSPR-Reference

| Field | Recorded value |
| --- | --- |
| Component | WSPR-Reference |
| Retained path | `src/WSPR-Reference` |
| Original repository | `https://github.com/WsprryPi/WSPR-Reference.git` |
| Parent-recorded import SHA | `2814fee75d394c404edd0e89ebd568b7e8b5e2d7` |
| Checked-out SHA | `2814fee75d394c404edd0e89ebd568b7e8b5e2d7` |
| Observed state | detached HEAD; `git submodule status` described the revision as `v0.18.3-17-g2814fee` |
| Most recent commit | `2026-07-06T09:55:49-05:00` — `Normalize .gitignore` |
| Version/tag provenance | `git describe` reported `v0.18.3-17-g2814fee`; no tag points at or contains the imported revision. The retained CMake package-version declaration is `0.14.1`. |
| Raw source tree | `32e05082087a58140bd33738e654128c58c7f813` |
| Expected and staged Phase A tree | `32e05082087a58140bd33738e654128c58c7f813` |
| Staged tree after Phase B | `f52c783c4a1c09d89d480442a8ca45273101033f` |
| Phase A archive | SHA-256 `78723eeedd18d53b086dba54e8152311165fd044241338606cf39733509a4297`; 71 tracked files and 17 directory entries |
| Former component license | `src/WSPR-Reference/LICENSE.md` at raw import; MIT, copyright 2026 Lee Bussy; blob `6584c5f4f63a8cfbeb0a1a2c642992e5e24af859` |
| License after absorption | Parent `LICENSE.md` (MIT) for WsprryPi-authored code. The redundant component license was removed after the contribution audit found only Lee Bussy identities. Bundled nlohmann/json obligations remain separately preserved. |
| Third-party content | `include/nlohmann/json.hpp`, upstream commit `f8eee1bb7953c6a4bff384d45052d5acc3d69698`, declared version 3.12.0, SHA-256 `acaa0c0e8cb75bbb2001ef3312549140f0ede093cf6f772683b612db75ecd004`; embedded SPDX/copyright notices retained byte-for-byte. Full MIT, CC0-1.0, and Apache-2.0 texts are under `docs/licenses/nlohmann-json/`. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`17d46d9c3c0d90ac3a21e7b7f552b5d7442a71a9b3747177287896dc03c45fa9`.
The object-derived archive contained the exact recorded tree, including CMake
package files, CLI tools, examples, tests, vectors, bundled JSON header,
historical nested workflow, component license, and the intentionally tracked
`install/` package snapshot. Its ordinary staged Phase A subtree OID equaled
the source tree OID.

The live submodule's `.git` administrative link was not tracked and was not
imported. There were no ignored or untracked files, nested submodules, nested
Git administration, or other raw-tree exclusions. The tracked `install/`
snapshot was retained as authoritative package/export evidence; all validation
outputs for this slice were instead written under `/private/tmp`.

### Phase B adaptations

- Preserved project `wspr_ref`, library `wspr_ref_lib`, alias
  `wspr::wspr_ref_lib`, CLI targets, examples, tests, golden vectors, package
  configuration, installed-header layout, and source hierarchy.
- Removed the obsolete former-repository CI badge and described active parent
  Debian coverage, monorepo location, licensing, and extraction.
- Ported the former component build/regression/install/consumer job into the
  parent `debian-non-hardware.yml` workflow and added CMake to that workflow's
  build dependencies. The historical nested workflow remains component
  documentation but is no longer represented as active parent CI.
- Made `run_major_regressions.sh` accept an explicit out-of-tree build path and
  portable parallel count while preserving its default component-root build.
- Made CMake copy the immutable golden-vector JSON into the build tree so
  `verify_vectors` works from arbitrary out-of-tree build locations. The source
  vector was not modified or regenerated.
- Removed the redundant Lee Bussy component MIT license and added the required
  distributable nlohmann/json MIT, CC0-1.0, and Apache-2.0 license texts without
  altering the bundled header.

Production encoding, decoding, correlation, Fano, unpacking, planning, public
headers, APIs, algorithms, CLI behavior, and golden expected data are unchanged.

### Standalone build, validation, and extraction

From the component root, configure and build with `cmake -S . -B <build>` and
`cmake --build <build>`. Run the complete safe suite with
`WSPR_REFERENCE_BUILD_DIR=<build> ./tests/run_major_regressions.sh`. Install to
a disposable prefix with `cmake --install`, configure `examples/consumer`
against that prefix, build it, and run `consumer`.

To extract WSPR-Reference, copy `src/WSPR-Reference`, add repository metadata
and a license for the WSPR-Reference-authored code, and retain the nlohmann/json
header, embedded notices, and corresponding third-party license set. The CMake
project remains independent of WsprryPi application internals.

### Slice validation

Apple Clang configured and built every retained library, CLI, example, and test
target in a disposable out-of-tree directory. The complete major-regression
script passed, including six golden vectors, core round trips, Fano coverage,
planning, paired encoding, ambiguity observations, CLI text/quiet/JSON checks,
and correlation. Installation to a disposable prefix succeeded; the external
CMake consumer configured, built, ran, encoded a message, and decoded `AA0NT`.

The first disposable run exposed and then verified the correction for the
former `../test_vectors` build-location assumption. Header SHA-256 confirmed
the bundled nlohmann/json file is the audited upstream snapshot; the golden
vector SHA-256 remained
`1d981e0e2aeeaee914a473eb6cce94276bc11df28bcef785bce69d109106900c`.
A Git-free staged export and repeat validation, parent source-discovery check,
workflow syntax review, staged whitespace/scope audit, Debian CI execution, and
post-migration fresh-clone acceptance complete the remaining slice/final gates.
No RF, GPIO, I2C, hardware, service, installation-to-host, deployment, or
former-remote operation occurred.

## WSPR-Transmitter

| Field | Recorded value |
| --- | --- |
| Component | WSPR-Transmitter |
| Retained path | `src/WSPR-Transmitter` |
| Original repository | `https://github.com/WsprryPi/WSPR-Transmitter.git` |
| Parent-recorded import SHA | `d2e329639e907f735531e48f202263d353320035` |
| Checked-out SHA | `d2e329639e907f735531e48f202263d353320035` |
| Observed state | detached HEAD; `git submodule status` described the revision as `d2e3296` |
| Most recent commit | `2026-08-16T10:29:15-05:00` — `Qualify legacy 2200 m keyed modes` |
| Version/tag provenance | No tag or version description was observed for the imported revision; the exact SHA is authoritative. |
| Raw source tree | `cc0ca41ad8345369f5c9ff6725e6b03b7be64c40` |
| Expected and staged Phase A tree | `cc0ca41ad8345369f5c9ff6725e6b03b7be64c40` |
| Staged tree after Phase B | `8da10107aa32ccb013bbd6402fb349c9bb8b4e68` |
| Phase A archive | SHA-256 `d002ed090f0af7d30a097c43270f514881da5e2d2c9b2a484b2268b04956b175`; 80 tracked files |
| Former component license | `src/WSPR-Transmitter/LICENSE.md` at raw import; MIT, copyright 2026 Lee Bussy; blob `6584c5f4f63a8cfbeb0a1a2c642992e5e24af859` |
| License after absorption | Parent `LICENSE.md` (MIT). The redundant component license was removed after the contribution audit found only Lee Bussy identities. |
| Former remote | Left untouched; no commit, push, branch, tag, archive, visibility, or synchronization operation was performed in the former repository. |

### Raw-tree evidence and exclusions

The tracked inventory SHA-256 was
`356dfc130a1f9af2ff73111a7157ca1bdce974625c0a4a0e9ed486472f49c00d`.
The object-derived archive and ordinary staged Phase A subtree both exactly
matched the recorded source tree before adaptation.

The live submodule's `.git` administrative link was not tracked and was not
imported. Ignored `src/build/` contained 14 generated build artifacts and was
excluded; the ignored-path inventory SHA-256 was
`996cfbd04f7d8dd23bc173fbddc1491d14153f1dc6d449139ebf602a7d47a76d`.
There were no untracked files, nested submodules, or nested Git administration.
The tracked zero-byte `src/.codex` local-tool marker was deliberately removed
during Phase B. The tracked `external/` compatibility stubs were retained as
intentional product content.

### Phase B adaptations

- Preserved controller and backend APIs, source hierarchy, compatibility stubs,
  standalone Makefile, demo entry point, simulator contracts, fake-I2C and
  fake-GPIO tests, and guarded live qualification sources.
- Fixed standalone project and executable naming so it does not depend on a
  component Git remote, and renamed Makefile source-discovery variables from
  submodule to component terminology without changing paths.
- Replaced the former privileged generic `test` behavior with the hardware-free
  simulator, controller, planner, fake-I2C transition, and startup-quiescence
  suite. Live `watchdog` and `gdb` diagnostics now require the explicit
  `WSPR_TRANSMITTER_LIVE_TEST=YES` guard.
- Recorded the parent Debian workflow as active CI. It already provides the
  former nested workflow's simulator and controller coverage plus transition
  and startup-quiescence coverage; only its transmitter step label changed.
- Removed the redundant component MIT license and documented parent licensing,
  monorepo dependency paths, hardware boundaries, and extraction guidance.

Production controller, scheduler, encoding adapter, transmission behavior,
hardware backends, GPIO/RP1/Si5351 logic, public interfaces, and runtime paths
are otherwise unchanged.

### Standalone build, validation, and extraction

From `src/WSPR-Transmitter/src`, `make test` is the canonical hardware-free
suite. Individual safe entry points include `simulated-backend-test`,
`simulated-backend-realtime-test`, `transmission-controller-contract-test`,
`si5351-planner-test`, `si5351-transition-test`, and
`startup-quiesce-test`. Live qualification targets remain build-only; their
executables were not run.

To extract WSPR-Transmitter, copy `src/WSPR-Transmitter` together with
`src/WSPR-Reference`, `src/Mailbox`, and `src/Signal-Handler` while preserving
their relative layout, add repository metadata and a license file, and run the
hardware-free suite before planning separately authorized physical-backend
qualification. WsprryPi application integration should remain outside the
extracted component.

On macOS, the unmodified `make test` reached compilation but failed because
Apple Clang treats the existing compile-time `-lstdc++fs` as an unused linker
input under `-Werror`. With platform-only compiler/linker overrides, the full
suite advanced until Linux-only `linux/ioctl.h` and `linux/i2c-dev.h` headers
were required; Apple Clang also diagnosed an existing unused lambda capture in
`startup_quiesce_test.cpp`. Individually, the simulator, real-time simulator,
transmission-controller contract, and Si5351 planner targets all built and
passed. The guarded `watchdog` and `gdb` targets both refused execution with
status 2 before building or running the application.

A Git-free export of the staged parent index repeated and passed those four
portable hardware-free targets. Parent debug source discovery included the
production transmitter source and excluded its standalone test sources, though
the dry run emitted expected macOS warnings for Linux `/proc/meminfo` and
`nproc` assumptions. The source-snapshot comparison found only Git
administration, ignored build output, the two documented exclusions, and the
README/Makefile adaptations.

The authoritative default-command run on Debian and synchronized `wspr4`, the
Linux-only fake-device tests, parent-wide regression/CI, final migration export,
and post-commit fresh-clone acceptance remain deferred. No hardware, RF, GPIO,
I2C, mailbox-device, service, installation, deployment, or former-remote
operation occurred.
