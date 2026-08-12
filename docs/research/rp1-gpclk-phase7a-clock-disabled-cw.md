# Phase 7A: clock-disabled keyed-mode integration

## Outcome

Phase 7A passed on `wspr5`. The production WsprryPi application compiled and
executed real QRSS, FSKCW, and DFCW requests through the Raspberry Pi 5 RP1
GPCLK finite-event path. Every run used the installed version-2 provider with
`live_output=N`; GPIO4 remained an input and GPCLK0 remained unprepared and
disabled. No GPIO waveform or RF was generated.

No production source change was required. The existing execution-plan,
finite-event compiler, scheduler-backend, Linux-provider, and kernel-provider
contracts were sufficient for this phase.

## Source and runtime identity

- Parent source commit: `dbfb9b3d0b41a864dcae923ef3dce0c9b508562d`.
- WSPR-Transmitter commit: `fe8a03b17a817175553968f91508fccd48c78bdf`.
- Isolated Pi worktree: `/home/pi/phase7a-gitwork`, detached at the exact parent
  commit and clean after the build and tests.
- Tested binary: `/home/pi/phase7a-gitwork/src/build/bin/wsprrypi_debug`.
- Binary SHA-256:
  `70f42dbefe6fdb40468499096696727e42af898f50b92f44eeba695bac0fbc11`.
- Running kernel: `6.18.44-v8-16k+ #1 SMP PREEMPT Tue Aug 11 14:50:34 CDT 2026`.
- Provider source version: `D33AD651DB5EA8776DE0AAF`.

The operational checkout at `/home/pi/WsprryPi` was not used as a source or
build tree because it contains preserved Phase 6X handoff changes. GitHub and
the clean Mac checkout remained the source of truth.

## Contract traced

For all three modes, `ExecutionPlanCompiler` creates finite `RfEvent` plans.
`compileRp1GpclkEventProgram` converts the complete plan into the version-2
provider tone table and ordered finite events. `WsprRp1GpclkBackend` submits the
program through `Rp1GpclkLinuxProvider`, polls event state, reports progress,
requests finite STOP when canceled, and releases provider ownership.

The tested contracts preserve the required mode distinctions:

- QRSS uses keyed RF-on symbols and RF-off Morse gaps.
- FSKCW keeps RF active and represents Morse symbols and gaps with mark and
  space frequencies.
- DFCW distinguishes dot and dash by frequency while retaining its configured
  RF-off element, character, and word gaps. Dot and dash elements use the DFCW
  dot duration.
- WSPR remains on its existing version-1 162-symbol full-frame path.
- Canonical CW remains unimplemented and rejected; it was not qualified here.

## Portable and Pi validation

The following production tests passed on the clean Pi worktree using
`make -j$(nproc)`:

- `rp1-gpclk-planner-test`
- `rp1-gpclk-lifecycle-test`
- `rp1-gpclk-transition-test`
- `rp1-gpclk-backend-test`
- `rp1-gpclk-linux-provider-test`
- `rp1-gpclk-transmit-backend-test`
- `qrss-execution-regression-test`
- `non-wspr-repeat-policy-test`

The full debug application build also passed. On macOS, the first six focused
RP1 targets passed, but the larger regression sequence stopped while compiling
`qrss_execution_regression_test` because AppleClang treats the pre-existing GCC
option `-fmax-errors=10` as an unused argument under `-Werror`. The same test
passed on the target Pi; the macOS incompatibility is recorded rather than
treated as a Phase 7A product failure.

## Real application executions

Each application run selected `--backend gpio`, GPIO4, and the minimum 2 mA
drive, with system-clock estimation disabled and manual PPM set to zero. The
provider remained clock-disabled.

| Mode | Request | Observed result |
|---|---|---|
| QRSS | `EE`, 14.097050 MHz, 0.7 s dot | Completed, return 0, 3.525336 s |
| FSKCW | `EE`, 14.097055 MHz mark, 14.097050 MHz space, 0.7 s dot | Completed, return 0, 3.511640 s |
| DFCW | `EE`, 14.097050 MHz dot, 14.097055 MHz dash, 0.7 s dot | Completed, return 0, 2.119715 s |

The elapsed results agree with the real mode plans: QRSS and FSKCW each used
two 0.7 s `E` elements plus the configured 2.1 s inter-character interval;
DFCW used two 0.7 s `E` elements plus its 0.7 s inter-character gap. Application
progress and finished callbacks were observed for all three modes.

## Cancellation, owner loss, and reuse

A 2 s-dot QRSS `EE` request provided a known keyed interval from 0 to 2 s and a
known RF-off inter-character interval from 2 to 8 s.

- SIGINT at 0.5 s canceled during the first keyed interval. The application
  reported cancellation after 0.509758 s, returned zero, and a new 0.2 s-dot
  process acquired, completed, and returned zero.
- SIGINT at 3 s canceled during the RF-off character gap. The application
  reported cancellation after 2.981655 s, returned zero, and a new process
  acquired, completed, and returned zero.
- SIGKILL of the actual transmitter owner at 0.5 s exercised provider
  file-release cleanup. A new process acquired and completed successfully after
  the close.

The existing direct version-2 lifecycle harness was then rerun against the same
installed provider. It passed exclusive ownership, second-owner rejection,
normal completion, STOP and terminal reason, active-owner close, reacquisition,
new-owner generation reset, repeated completion, and final release with zero
failures.

An initial owner-close attempt killed only the `sudo` wrapper, leaving its child
application alive. The attempted immediate reacquisition correctly failed on
the application's singleton guard, and the child completed normally. That
invalid attempt is retained under a clearly labeled evidence filename and was
replaced by the correct child-owner termination described above.

## Final state and evidence

The final verified state was:

- `live_output=N`;
- GPIO4 input;
- GPCLK0 prepare count 0 and enable count 0;
- `wsprrypi.service` active; and
- `soapyremote-server.service` active.

Raw application logs, session timings, focused build/test transcripts, provider
lifecycle output, safe-state captures, and the labeled invalid harness attempt
are retained under `/home/pi/phase7a-evidence`. A verified 47-entry SHA-256
manifest covers those files.

## Qualification boundary

Phase 7A establishes clock-disabled application integration for QRSS, FSKCW,
and DFCW on the exact Raspberry Pi 5 / RP1 provider build above. It proves plan
construction, finite-event submission, application progress and completion,
STOP behavior in keyed and gap intervals, owner-close cleanup, reacquisition,
and repeated provider use without energizing GPIO4.

It does not qualify RF waveform accuracy, over-the-air readability, spectral
behavior, calibrated or relative output power, GPIO20, other bands, higher
drive settings, canonical CW, other Raspberry Pi models, or other kernel
configurations. Those require later, explicitly authorized phases.

## Documentation impact

This core-repository engineering report and its compact evidence summary were
added. Operator documentation was considered but intentionally unchanged:
clock-disabled engineering integration is not yet an operator-supported or
RF-qualified feature. Operator power selection, operator documentation, CW,
and live QRSS/FSKCW/DFCW qualification remain required follow-up work.
