# Phase 6Y: packaged RP1 GPCLK executor qualification

## Outcome

Phase 6Y passed on `wspr5`. The exact four-patch Phase 6X kernel build was
installed under a recoverable boot name, booted successfully, and passed its
matching KUnit suite, clock-disabled version-2 lifecycle tests, the established
version-1 production WSPR regression, and a direct version-2 finite-event WSPR
decode.

The production regression produced one initial decoded frame followed by three
decoded frames from three independent normal WsprryPi scheduler processes. A
supplemental version-2 harness then submitted the same 162-symbol payload through
the newly enabled finite-event ioctl and produced another independent decode:

```text
AA0NT EM18 20
```

Every live run used RP1 GPCLK0 on GPIO4 at the minimum 2 mA drive. No absolute
power calibration or spectral-compliance claim was made.

## Installed identity and rollback

- Parent source commit: `314c576c0883027b796f5f45444863c6d3ab9ba9`.
- Raspberry Pi kernel baseline: `89586905b8603e545cce9089a81f5f35d65bc998`.
- Running kernel: `6.18.44-v8-16k+ #1 SMP PREEMPT Tue Aug 11 14:50:34 CDT 2026`.
- Installed image SHA-256: `fc174f88b5208f27b7ff3ee9ae668f545d5b647efaf6f48114d7c074d111187c`.
- Provider source version: `D33AD651DB5EA8776DE0AAF`.
- Provider and KUnit vermagic: `6.18.44-v8-16k+ SMP preempt mod_unload modversions aarch64`.
- Boot selector: `kernel=kernel_2712_phase6x.img`.

The previous `kernel_2712_phase6h.img` was left untouched. The previous
`config.txt` and both provider modules were copied to
`/home/pi/phase6y-install-evidence` before installation. Restoring that config
and its saved modules provides the rollback path.

## Non-RF validation

The provider was loaded with `live_output=N` throughout these checks.

- The matching KUnit module passed 5 tests, with 0 failures and 0 skips.
- The version-2 device lifecycle harness passed:
  - exclusive acquire and second-owner rejection;
  - normal event completion;
  - STOP with terminal reason `STOPPED`;
  - release and reacquisition;
  - active-owner close cleanup;
  - generation reset for a new owner; and
  - repeat completion and final release.
- No GPIO, DMA tick, or RF output was enabled by these checks.

## Production WSPR regression

The SDRplay RSP1B serial `2404058C60` captured at 250 ksample/s, 25 dB requested
gain, and a 14.122100 MHz center. Retained captures were converted using the
previously established relative 27,120 Hz translation. These are relative SDR
findings only.

| Run | Scheduler duration | Primary decode | Relative contrast | Worst of 161 boundaries |
|---|---:|---|---:|---:|
| Initial gate | 110.717577 s | +29 dB, 14.097044 MHz, `AA0NT EM18 20` | 36.69 dB | -0.084 dB |
| Independent 1 | 110.702321 s | +28 dB, 14.097126 MHz, `AA0NT EM18 20` | 37.01 dB | -0.154 dB |
| Independent 2 | 110.706210 s | +28 dB, 14.097002 MHz, `AA0NT EM18 20` | 35.82 dB | -0.054 dB |
| Independent 3 | 110.699900 s | +28 dB, 14.097048 MHz, `AA0NT EM18 20` | 31.78 dB | -0.052 dB |

The third sequential capture began only 0.901 seconds before detected RF rather
than the five seconds assumed by the inherited converter. Its first conversion
therefore discarded about four seconds of the transmitted frame and did not
decode. Re-aligning that same retained capture to its measured start produced
the decode above. No retransmission or encoded-symbol substitute was used.

The state monitor first observed GPCLK0 after each scheduled start boundary:
approximately +0.090, +0.110, +0.059, and +0.099 seconds for the initial and
three independent runs. Each scheduler returned zero, each capture reported
zero overflows, and every run returned GPIO4 to input with GPCLK0 prepare and
enable counts at zero.

## Direct version-2 executor qualification

Normal WSPR execution intentionally uses the version-1 full-frame ioctl, so the
production regression alone does not exercise the Phase 6X finite-event path.
Phase 6Y therefore encoded `AA0NT EM18 20` with the repository's reference
encoder and submitted 162 standard-duration WSPR events through
`RP1_GPCLK_IOC_SUBMIT_EVENTS`.

- Provider result: state `COMPLETE`, current event `162`, terminal reason
  `COMPLETE`.
- Decoder result: +29 dB, 14.097030 MHz, `AA0NT EM18 20`.
- Relative live-to-baseline contrast: 38.08 dB.
- Four detected tone clusters had adjacent spacing estimates of 1.509, 1.458,
  and 1.435 Hz.
- All 161 boundaries were evaluated; the worst centered 20 ms level was
  -0.062 dB relative to the active-frame median.
- GPCLK0 was first observed 0.833 seconds after the requested frame epoch; no
  output state was observed before that epoch.
- Terminal cleanup returned GPIO4 to input, stopped the clock, released the
  provider, restored `live_output=N`, and restarted both services.

This supplemental harness qualified the packaged version-2 executor itself; it
does not change the application's intentional version-1 WSPR contract.

## Kernel and cleanup health

No kernel BUG, oops, panic, general-protection fault, or call trace was recorded.
The final verified state was:

- `live_output=N`;
- GPIO4 input;
- GPCLK0 prepare count 0 and enable count 0;
- `wsprrypi.service` active; and
- `soapyremote-server.service` active.

Raw captures, scheduler and provider logs, state monitors, decoder output,
analysis, and a 106-entry SHA-256 manifest are retained under
`/home/pi/phase6y-evidence` (approximately 1.2 GB). Rollback configuration and
module backups are retained separately under
`/home/pi/phase6y-install-evidence`.

## Qualification boundary

Phase 6Y qualifies the exact packaged Phase 6X build for Raspberry Pi 5, RP1
GPCLK0, GPIO4, 20 m WSPR, and 2 mA drive. It covers both the production
version-1 WSPR path and the new version-2 finite-event executor with a decoded
WSPR payload.

It does not qualify GPIO20, other bands, higher drive settings, CW, absolute
output power, spectral compliance, other Raspberry Pi models, or other kernel
configurations.

## Documentation impact

This core-repository engineering report records installation and qualification
evidence. Operator documentation was not changed. Operator-facing installation,
backend selection, minimum-power behavior, and the precise qualification limits
remain required before this path is presented as supported to operators. CW and
operator power selection remain separate future work.
