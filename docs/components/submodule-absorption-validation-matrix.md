# Issue 415 Validation Matrix

Execution results for the completed migration are recorded in
[`submodule-absorption-validation-results.md`](submodule-absorption-validation-results.md).

Status: Executed for the committed migration; remaining gates are recorded in
the linked results

Contract: [`../plans/submodule-absorption-contract.md`](../plans/submodule-absorption-contract.md)

Baseline: [`submodule-absorption-baseline.md`](submodule-absorption-baseline.md)

Snapshot method: [`submodule-absorption-snapshot-method.md`](submodule-absorption-snapshot-method.md)

Adaptations: [`submodule-absorption-adaptations.md`](submodule-absorption-adaptations.md)

## Purpose and execution rule

This matrix fixes the validation scope before any gitlink is replaced. Commands
were derived from the tracked Makefiles, CMake project, tests, and CI workflows
at the Issue 415 baseline. Reinspect those files immediately before execution.

Run a component's raw-tree checks after Phase A and its build/test checks only
after that component's Phase B adaptations are staged. Stop at the first failed
acceptance criterion. Keep command output, tree OIDs, archive digests, exit
statuses, and host identity in the migration evidence.

`wspr4` is a read-only Raspberry Pi validation target for this issue. Before
each target slice, verify that `/home/pi/WsprryPi` is clean, on the Issue 415
branch, and at the same commit as the authoritative checkout. Do not edit,
stage, commit, or push from `wspr4`.

## Host classes

| Class | Permitted role |
| --- | --- |
| Local macOS | Repository comparisons, script syntax checks, UI static tests whose dependencies are already present, and portable builds that do not require Linux-only headers or commands. A macOS result does not replace Debian or Raspberry Pi validation. |
| Debian CI | Canonical unprivileged parent and transmitter non-hardware suite with `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1`; standalone Linux builds and tests. No physical device access. |
| `wspr4` | Raspberry Pi compile and explicitly reviewed non-hardware tests only. No install targets, services, GPIO, mailbox device, I2C, Si5351, or RF execution. |
| Post-commit disposable clone | True checkout completeness without `--recurse-submodules`. This cannot be accepted while the migration exists only in the index. |
| Forbidden | `sudo` mutation, installation/deployment, service lifecycle, reboot, `/dev/gpiomem`, `/dev/mem`, `/dev/vcio`, GPIO, I2C, Si5351 output, transmitter device nodes, test tones, or RF. |

## Universal repository checks

Run these for every component before advancing to the next one:

1. Repeat the clean, initialized, SHA-alignment, ignored/untracked, nested-Git,
   license, and inventory gate from the baseline.
2. Follow the snapshot method and record the source tree, expected tree, staged
   tree, archive SHA-256, and archive path list.
3. Require staged-tree equality. WSPR-Transmitter may differ only by the
   approved tracked `src/.codex` exclusion; no other raw-tree difference is
   allowed.
4. Require no `.git` file/directory and no ignored, generated, untracked, or
   unregistered tool-state artifact in the staged component.
5. Require `git diff --cached --check -- <component>` and a raw diff showing
   the `160000` deletion plus ordinary-file additions.
6. Record the component validation result before beginning another component.

## Component matrix

All Make commands below run from the named component's `src/` directory unless
the row says otherwise. Phase B may remove, rename, or rewrite a component test
target when the current recipe is a demo, service-dependent, unbounded, or
hardware-facing. Prefer deterministic unprivileged hardware-free replacements.
Pass means exit status zero, the expected fixed output name is produced, and no
forbidden device or operational side effect occurs. Update this matrix with the
final safe command whenever a target is adapted.

| Component | Safe validation after adaptation | Required host and acceptance boundary |
| --- | --- | --- |
| `WsprryPi-UI` | From its root: `node tests/cw_timing_state_test.js`; `node tests/responsive_shell_logs_test.js`; `node tests/support_bundle_ui_test.js`; `node tests/wspr_band_frequency_correlation_test.js`; `php tests/gpio_dropdown_test.php`; `php tests/spot_menu_test.php`. Run `node tests/cw_duration_latch_integration_test.js` and `node tests/conditional_transmit_gpio_integration_test.js` only where the tracked Node `ws` dependency and PHP CLI are available; they use loopback servers and mocks. | Local macOS or Debian. All listed tests pass. `tests/log_stream_disconnect_integration_test.sh` is forbidden in this migration because it uses the installed Apache endpoint, `sudo`, and `systemctl`. Parent UI/source integration must also pass. No visual review is required because absorption must not alter UI presentation. |
| `src/INI-Handler` | `make debug SUDO=` followed by `make test SUDO=` using a temporary copy of any mutable INI fixture. Confirm `build/bin/ini-handler_test`. | Debian or synchronized `wspr4`; local macOS only if the Makefile toolchain is supported. The repository fixture and checkout remain byte-identical after the run. |
| `src/LCBLog` | `make debug SUDO=` and `make test SUDO=`. Confirm `build/bin/lcblog_test`. Also compile its release/demo target with `make release SUDO=` and confirm `build/bin/lcblog`. | Debian or synchronized `wspr4`. Tests pass without WsprryPi headers, globals, configuration, services, or directory assumptions. This is the explicit reusable-boundary gate. |
| `src/Mailbox` | Current baseline: compile with `make debug SUDO=` and confirm `build/bin/mailbox_test`; optionally `make release SUDO=` and confirm `build/bin/mailbox`. Phase B should replace or rename the current device-opening `test` recipe and add a hardware-free contract test if the existing interface permits one without production refactoring. | Debian or synchronized `wspr4`. Do **not** run the baseline `make test`: its demo opens the Raspberry Pi mailbox interface. The final ordinary `test`, if retained, must not open `/dev/vcio`; otherwise compilation is the bounded acceptance and device behavior remains unqualified. |
| `src/MonitorFile` | `make debug` then `make test`; confirm `build/bin/monitorfile_test`. Run from a disposable temporary working directory if the demo creates monitored files. | Debian or synchronized `wspr4`. Test exits cleanly and leaves no checkout artifact. Its deliberate waits are bounded; no service or device access. |
| `src/PPM-Manager` | Current baseline: compile with `make debug SUDO=` and confirm `build/bin/ppm-manager_test`; optionally `make release SUDO=` and confirm `build/bin/ppm-manager`. Phase B should replace or rename the current live-provider demo target; prefer a bounded provider-unavailable or fake-provider test. | Debian or synchronized `wspr4`. Do **not** run the baseline `make test`: it queries Chrony and waits for a signal. The final ordinary `test`, if retained, must be bounded and provider-neutral. Chrony integration and time discipline remain unqualified. |
| `src/Signal-Handler` | `make debug SUDO=` and `make test SUDO=`; confirm `build/bin/signal-handler_test`. | Debian or synchronized `wspr4`. Test exits cleanly with its internal signal/thread exercise and performs no service or hardware action. |
| `src/Singleton` | From `src/Singleton/src`: `make test`; confirm `singleton_test`. | Debian or synchronized `wspr4`. Test passes and leaves only declared build artifacts, which must not be staged. The Makefile uses Linux `nproc`, so macOS is not the acceptance host. |
| `src/WSPR-Transmitter` | `make simulated-backend-test`; `make build/bin/simulated_transmit_backend_realtime_test`; `timeout 5s ./build/bin/simulated_transmit_backend_realtime_test`; `make transmission-controller-contract-test`; `make startup-quiesce-test`; `make si5351-transition-test`. Phase B should remove, rename, or safely retarget generic `make test` so an ordinary test cannot select a physical backend. | Debian CI with hardware access disabled. These are the tracked parent/nested-workflow commands. Do not run the baseline generic `make test`, `watchdog`, live qualification executables, or any physical backend. Simulator evidence is software-only. |
| `src/WSPR-Reference` | From the component root: `cmake -S . -B build`; `cmake --build build -- -j$(nproc)`; `./tests/run_major_regressions.sh`; `cmake --install build --prefix install`; `cmake -S examples/consumer -B examples/consumer/build -DCMAKE_PREFIX_PATH="$PWD/install"`; `cmake --build examples/consumer/build`; `./examples/consumer/build/consumer`. Use disposable build/install directories or remove only newly created declared artifacts after recording results. | Debian CI. All regressions and the exported-package consumer pass; project remains independently configurable from its root and retains `wspr_ref_lib`, CLI tools, examples, vectors, tests, and package exports. A macOS run is supplemental because the tracked regression script uses `nproc`. |

Generated outputs from any validation remain excluded from the migration index.
Use exact, explicit cleanup targets only after confirming that the output was
created by the current validation run; never run a broad recursive cleanup.

## Parent and cross-cutting matrix

| Area | Command or evidence | Acceptance |
| --- | --- | --- |
| Parent build and non-hardware regression | On Debian, reproduce `.github/workflows/debian-non-hardware.yml`: `make debug SUDO=`, `make semantics-test SUDO=`, `make startup-quiesce-parent-test SUDO=`, `make rp1-gpclk-transmit-backend-test SUDO=`, the transmitter commands above, `../scripts/ci/verify-simulated-backend.sh`, `../scripts/ci/verify-simulated-wspr.sh`, and the workflow's `strace` hardware-rejection check. | Workflow passes with `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1`; traces contain no prohibited transmitter-device access. |
| Parent/UI integration | Parent `make semantics-test SUDO=` includes `ui_source_regression_test`; run the safe UI tests listed above. Inspect `scripts/copy_ui.py` tests or source regression that compare the retained `WsprryPi-UI/data` path and destination mapping. | UI/source tests pass and deployment source/layout is unchanged. Do not copy to an installed web root. |
| Build discovery | Capture the parent source manifest and `make -n debug SUDO=` before and after the Phase B variable rename. | Component membership, source filtering, include paths, object paths, link inputs, and outputs are identical apart from terminology. |
| Script adaptations | `bash -n scripts/install.sh scripts/sync_all_branches.sh`; `python3 -m py_compile scripts/copy_ui.py scripts/research/websocket_thread_memory_rig.py` with bytecode directed outside the checkout when needed. Review dry-run/self-test modes only. | Syntax succeeds; clone, branch, UI-copy, and evidence behavior changes only by removing obsolete submodule mechanics. Do not run installer, branch-sync mutation, deployment, or live rig operation. |
| Git hooks | Exercise hooks in a disposable repository/index: conflict markers remain rejected by pre-commit and direct pushes to `main`/`master` remain rejected by pre-push. | Retained protections behave identically; checkout/merge performs no component initialization or network action. |
| Provenance and licensing | Compare final `docs/components/provenance.md`, third-party notices, and license paths against the baseline, ownership audit, exclusions, and per-component evidence. | Every component URL/SHA/license/tag/exclusion/build entry point is accounted for; Mailbox identifies Broadcom only as historical design lineage. |
| Repository final state | `.gitmodules` absent; `git submodule status --recursive` empty; all ten paths are ordinary staged trees; no nested Git administration; `git diff --cached --check`; inspect complete staged diff. | No unexplained content change or imported artifact. Every Phase A tree and Phase B adaptation is documented. |

## Staged-tree checkout-equivalent check

Before a migration commit exists, validate the index without inventing a commit:

```sh
export_dir=$(mktemp -d /private/tmp/wsprrypi-issue415-index.XXXXXX)
index_tree=$(git write-tree)
git archive --format=tar "$index_tree" | tar -xf - -C "$export_dir"
test ! -e "$export_dir/.gitmodules"
test -z "$(find "$export_dir" -name .git -print)"
```

Verify all ten component roots and required retained assets in the export, then
compare each exported component tree to its recorded expected tree. Run only
builds/tests that operate correctly outside a Git checkout; record all Git-
metadata-dependent checks as deferred. Delete the disposable export only after
its evidence has been reviewed.

This proves staged-tree completeness, not clone behavior. After an authorized
migration commit is pushed, make a disposable clone without
`--recurse-submodules`, confirm all ten ordinary trees are present, rerun the
repository and supported build/test checks, and record that separate result.

## Explicitly deferred or prohibited qualification

- True fresh-clone acceptance passed against the published Issue 415 branch as
  recorded in the linked validation results.
- GitHub workflow results are deferred until the migration commit is published
  on a workflow-triggering ref or reviewed through the authorized mechanism.
- `wspr4` validation is deferred until its checkout is explicitly synchronized
  to the same reviewed migration state; do not copy an uncommitted index to it.
- Installation, Apache/web-root deployment, services, reboot, Chrony operation,
  mailbox device execution, GPIO, MMIO, DMA, I2C, Si5351, test-tone generation,
  transmitter backends, and RF are prohibited by Issue 415.
- No build, unit test, simulator, staged export, or fresh clone qualifies those
  hardware and operational behaviors.

## Gate exit criteria

The universal preparation gate is complete when this matrix is reviewed against
the current tracked entry points, linked from the contract, and committed with a
clean, SHA-aligned parent/submodule state. It authorizes no conversion by itself.
The next step is the first per-component migration slice, beginning with a fresh
complete gate and the agreed component order.
