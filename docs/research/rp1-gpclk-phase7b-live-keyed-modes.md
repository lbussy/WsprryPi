# Phase 7B: live RP1 GPCLK keyed-mode qualification

## Outcome

Phase 7B passed on `wspr5`. Two independent production-application runs each
of QRSS, FSKCW, and DFCW produced the expected live RP1 GPCLK0 behavior on
GPIO4 at the minimum 2 mA drive selection. Retained SDR captures show the
required keyed intervals, gaps, continuous-frequency-keyed behavior, and
approximately 5 Hz tone shifts.

All measurements are relative SDR findings. No absolute RF power calibration
or spectral-compliance claim is made.

## Exact test identity

- Parent source commit: `5f8029d2d66b9ed0fe9164c2ef29db78cf86cd55`.
- WSPR-Transmitter commit: `fe8a03b17a817175553968f91508fccd48c78bdf`.
- Isolated Pi worktree: `/home/pi/phase7b-gitwork`, detached at the exact parent
  commit with every submodule at its recorded revision.
- Tested binary: `/home/pi/phase7b-gitwork/src/build/bin/wsprrypi_debug`.
- Binary SHA-256:
  `5ef51bda0d09201ba712bc1c96aed0b3eb6402595bb6a9326812b22e2c2539bd`.
- Running kernel: `6.18.44-v8-16k+ #1 SMP PREEMPT Tue Aug 11 14:50:34 CDT 2026`.
- Provider source version: `D33AD651DB5EA8776DE0AAF`.
- Provider vermagic:
  `6.18.44-v8-16k+ SMP preempt mod_unload modversions aarch64`.
- SDR: SDRplay RSP1B serial `2404058C60`.

The operational `/home/pi/WsprryPi` checkout and its preserved work were not
used or modified.

## Preflight and build

The Mac Issue 399 branch was clean and synchronized before the test. All Mac
submodules were clean and synchronized. A new Pi worktree was created at the
exact pushed parent revision and its submodules were initialized at the
recorded commits.

The following tests passed on the Pi with `make -j$(nproc)`:

- `rp1-gpclk-planner-test`
- `rp1-gpclk-lifecycle-test`
- `rp1-gpclk-transition-test`
- `rp1-gpclk-backend-test`
- `rp1-gpclk-linux-provider-test`
- `rp1-gpclk-transmit-backend-test`
- `qrss-execution-regression-test`
- `non-wspr-repeat-policy-test`

The full debug build also passed. The pre-existing AppleClang incompatibility
with the GCC option `-fmax-errors=10` remains a macOS build-system limitation;
the affected tests passed on the supported Pi build host.

## Live method

Each accepted run used:

- the production application's normal `gpio` backend and scheduler path;
- GPCLK0 on GPIO4;
- the 2 mA RP1 drive selection;
- system-clock estimation disabled and manual GPIO correction set to zero;
- message `ET` and a 0.7-second dot;
- an SDR center of 14.122100 MHz, 250 ksample/s, and 25 dB requested gain; and
- a 12-second complex-float capture containing baseline before and after the
  transmission.

The base, space, and dot request frequency was 14.097050 MHz. The FSKCW mark
and DFCW dash request frequency was 14.097055 MHz. The established SDR and
readback offset was not calibrated away; only timing, relative contrast, and
within-capture frequency separation were evaluated.

Before every application process, the provider was loaded with
`live_output=Y`. Immediately after the process completed, it was reloaded with
`live_output=N` before capture analysis continued. Each new process therefore
also exercised provider release and reacquisition.

The application reports `Completed transmission` only after the RP1 backend
has observed the provider's terminal completion state. Every accepted process
reported completion and returned zero; application exit alone was not used as
the completion criterion. Every SDR capture returned zero with no overflow.

## Results

### QRSS

The expected `ET` plan is a 0.7-second keyed `E`, a 2.1-second RF-off
inter-character gap, and a 2.1-second keyed `T`.

| Run | Scheduler duration | Detected RF intervals | E contrast | Gap versus baseline | T contrast |
|---|---:|---|---:|---:|---:|
| 1 | 4.958642 s | 0.70 s on, 2.10 s off, 2.10 s on | 28.073 dB | -0.144 dB | 28.033 dB |
| 2 | 4.926733 s | 0.70 s on, 2.10 s off, 2.10 s on | 28.233 dB | 0.089 dB | 28.204 dB |

Both captures preserve the keyed elements and suppress the carrier to the
measured baseline during the intended gap.

### FSKCW

The expected `ET` plan keeps RF active for 4.9 seconds: mark for the
0.7-second `E`, space for the 2.1-second character interval, then mark for the
2.1-second `T`.

| Run | Scheduler duration | Detected RF interval | Measured mark-space separation | Relative contrast range |
|---|---:|---:|---:|---:|
| 1 | 4.929364 s | 4.90 s continuous | 5.000 Hz | 28.049–28.059 dB |
| 2 | 4.924377 s | 4.90 s continuous | 4.946 Hz | 27.958–27.966 dB |

Neither capture contains an RF-off interval between the first and final
elements. The space interval remains at the same relative level as the mark
elements while changing frequency by approximately 5 Hz.

### DFCW

The expected `ET` plan is a 0.7-second dot-frequency `E`, a 0.7-second RF-off
inter-character gap, and a 0.7-second dash-frequency `T`. DFCW dot and dash
elements intentionally have equal durations.

| Run | Scheduler duration | Detected RF intervals | Measured dash-dot separation | E/T contrast | Gap versus baseline |
|---|---:|---|---:|---:|---:|
| 1 | 2.113541 s | 0.70 s on, 0.70 s off, 0.70 s on | 4.973 Hz | 28.218/28.216 dB | 0.250 dB |
| 2 | 2.124725 s | 0.70 s on, 0.70 s off, 0.70 s on | 4.928 Hz | 27.076/27.087 dB | 0.008 dB |

Both captures preserve equal-duration elements, the required RF-off gap, and
the intended frequency distinction.

## Invalid attempts

The first QRSS launch attempt captured only a baseline. Its application failed
before execution because the new worktree's binary was not yet visible at the
expected path. The provider was restored to `live_output=N`, GPIO4 remained an
input, and no RF was emitted. The attempt is retained as
`qrss-run1-invalid-prelaunch` and is excluded from acceptance results.

During isolated-worktree preparation, an initial submodule update refused to
overwrite pre-populated directories. Only the newly created disposable
worktree was involved. Its directories were removed, and Git then checked out
the exact recorded submodule commits. This occurred before live output was
enabled and did not affect any accepted run.

## Cleanup and retained evidence

After every accepted run:

- `live_output=N`;
- GPIO4 was input; and
- GPCLK0 prepare and enable counts were zero.

The final state additionally had `wsprrypi.service` and
`soapyremote-server.service` active. No capture or isolated transmitter process
remained. The test-window kernel log contained no warning-or-higher entry and
no BUG, oops, panic, general-protection fault, or call trace.

Raw complex captures, scheduler and capture logs, state monitors, JSON
analysis, build/test transcripts, identities, final state, and the labeled
invalid attempt are retained under `/home/pi/phase7b-evidence`. A verified
65-entry SHA-256 manifest covers the retained evidence.

## Qualification boundary

Phase 7B qualifies the exact Raspberry Pi 5, kernel, RP1 provider, application,
GPIO4, 20 m frequencies, 2 mA selection, message, timing, and 5 Hz shift tested
above for live QRSS, FSKCW, and DFCW operation. It adds live relative SDR
evidence to the clock-disabled application integration established in Phase
7A.

It does not qualify canonical CW, GPIO20, other bands, other timing or shift
values, higher drive settings, absolute output power, spectral or regulatory
compliance, other Raspberry Pi models, other kernels, or general operator
readiness.

## Documentation Impact

This core-repository engineering report and compact evidence summary were
added. Operator documentation was considered but intentionally left unchanged.
Operator power selection, canonical CW, supported-configuration wording, and
operator-facing installation and use documentation remain separate required
work before this capability is presented as generally supported.
