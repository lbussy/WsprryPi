# Issue 379: Si5351 and GPIO steady-carrier stability characterization

Status: branch-local research evidence, not an operator specification or a
project default.

## Purpose

This document records steady-carrier behavior measured from the physical
Si5351/ATX-11 TCXO assembly on `wspr5` at 2 m and 30 m, and the GPIO/GPCLK0
output on `wspr4` at 80 m. It characterizes the two hardware paths and the
RSP1B measurement chain under the stated loads. It does not establish
release-wide hardware performance.

Absolute tuning accuracy is intentionally excluded. The installed
hardware-specific calibration remained unchanged, but each capture's constant
frequency offset was removed before stability metrics were calculated.

## Software qualification boundary

Issue 379 qualifies WsprryPi's ability to operate the Si5351 and GPIO/GPCLK
backends correctly and safely. Natural reference-oscillator accuracy, drift,
phase noise, spurs, harmonics, and thermal behavior belong to the selected
hardware and RF chain. They must be measured and retained so operators can
understand the platform, but a hardware characteristic is not by itself a
software failure.

Software qualification therefore requires evidence that each backend:

- calculates and applies the intended four WSPR tones and preserves their
  spacing;
- implements symbol timing and the complete 162-symbol sequence correctly;
- performs controlled transitions without software-created gaps, unintended
  frequencies, or avoidable phase discontinuities;
- fails closed on invalid input, partial hardware operations, interruption, and
  shutdown;
- restores the output hardware and services to the verified idle state; and
- produces bounded conducted frames that an independent WSPR decoder accepts.

Absolute carrier offset and continuous thermal drift are characterized at a
recorded operating condition. They become software defects only when the
software calculates, programs, sequences, or cleans up the hardware
incorrectly. A decoded-frame failure must likewise be diagnosed before it is
assigned to software or hardware.

## Si5351 test configuration

- Transmitter host: `wspr5`
- Backend: Si5351, `/dev/i2c-1`, address `0x60`, CLK0, 2 mA
- Reference oscillator: ATX-11-F-27.000MHZ-F05-T TCXO
- Tones:
  - 2 m tone 0: `144.490497802734375 MHz`
  - 30 m tone 0: `10.140197802734375 MHz`
- RF path: shielded 50-ohm load with 30 dB series attenuation
- Receiver: SDRplay RSP1B serial `2404058C60`
- Receiver configuration: 250 kS/s, AGC off, fixed 25 dB requested and actual
  gain
- Capture duration: 300 seconds per band, sequentially, 2 m first
- Samples: 75,000,000 complex-float samples per band
- Receiver overflows: zero
- Installed calibration: `+2.353615654 ppm`, unchanged and not used to correct
  relative stability
- Observed CPU temperatures:
  - session start: 41.7 C
  - 2 m start/end: 41.7 C / 42.8 C
  - 30 m start/end: 43.9 C / 44.4 C

Before and after each tone, Si5351 register 3 was `0xFF`. After the session,
`wsprrypi.service` and `soapyremote-server.service` were active and no test
process remained.

## GPIO test configuration

- Transmitter host: `wspr4`, Raspberry Pi 4 Model B Rev 1.1
- Backend: GPIO/GPCLK0 on BCM GPIO 4, power level 7
- Tone: 80 m tone 0, `3.570097802734375 MHz`
- Band constraint: the installed hard 80 m LPF limited this host to 80 m
- RF path: GPIO 4, hard 80 m LPF, transferred attenuation/load, RSP1B
- Receiver host: `wspr5`
- Receiver: SDRplay RSP1B serial `2404058C60`
- Receiver configuration: 250 kS/s, AGC off, fixed 40 dB requested and actual
  gain
- Capture: 75,000,000 complex-float samples, 300 seconds, zero overflows
- Valid stability interval: 294.5 seconds after excluding 0.5 seconds at the
  start and five seconds at the end where analysis windows overlapped
  transmitter shutdown
- GPIO timing correction: `GPIO Use NTP = true`; Chrony reported 13.100 ppm
  fast at the start with -0.042 ppm residual frequency
- Configured `Calibration.PPM`: `0.0`; absolute accuracy remains excluded
- Transmitter temperature: 33.1 C at both start and end

The direct tone ran for 319.980 seconds and stopped on the wrapper's bounded
SIGINT. The post-run live audit found DMA inactive and detached, PWM disabled,
GPCLK0 disabled/not busy, and GPIO 4 restored to input. The audit passed,
`wsprrypi.service` and `soapyremote-server.service` were active, and no capture
or tone process remained.

## Analysis method

Each Si5351 IQ stream was translated by its nominal
receiver-to-carrier separation, coherently averaged into 10 ms blocks, and
trimmed by 0.5 seconds at each edge. Carrier frequency was estimated from
linear phase slope in two-second windows stepped every 0.25 seconds.

A constant frequency offset was removed independently from each capture. The
reported metrics therefore describe stability, not accuracy. Whole-run,
per-minute, and rolling 60-second linear fits were calculated. Phase continuity
was checked after removing a quadratic phase trend; no adjacent residual phase
change crossed the larger of pi/2 radians or eight robust standard deviations.

The observed residual includes both the transmitter and receiver references.
Because the captures were sequential rather than simultaneous, band-to-band
differences cannot be assigned exclusively to the Si5351 synthesis path.

The GPIO fractional-divider waveform did not satisfy the Si5351 estimator's
within-window phase-residual assumption. Applying it anyway produced a median
two-second phase-fit residual of 6.33 radians, so those results were rejected.
For GPIO, the IQ was instead translated and coherently integrated into 1 ms
blocks. A two-second Hann-window spectral peak was measured every 0.25 seconds
with a 32,768-point FFT. A broad discovery pass located the persistent carrier
near +43.4 Hz relative to the requested frequency; tracking was then confined
to the 40-to-47 Hz carrier lobe so isolated broadband-noise peaks could not be
mistaken for motion. That constant offset was removed from all stability
results. Median carrier contrast within the tracking interval was 20.48 dB.

The different estimators are a documented consequence of the backend
waveforms. Hertz and ppb results are useful hardware observations, but they are
not a controlled band-for-band performance ranking: backend, transmitter host,
band, LPF, attenuation, gain, and thermal history all differ.

## Whole-capture results

| Stability measure | Si5351 2 m | Si5351 30 m | GPIO 80 m |
|---|---:|---:|---:|
| Capture / analyzed duration | 300.0 / 299.0 s | 300.0 / 299.0 s | 300.0 / 294.5 s |
| Frequency estimates | 1,189 | 1,189 | 1,171 |
| Relative peak-to-peak movement | 4.369 Hz (30.24 ppb) | 0.436 Hz (42.97 ppb) | 0.782 Hz (218.96 ppb) |
| Central 90% movement | 3.354 Hz (23.21 ppb) | 0.357 Hz (35.26 ppb) | 0.213 Hz (59.79 ppb) |
| Whole-interval linear fit | +0.319 Hz/min | -0.074 Hz/min | -0.0122 Hz/min |
| RMS after linear-fit removal | 0.908 Hz (6.28 ppb) | 0.0477 Hz (4.70 ppb) | 0.0783 Hz (21.94 ppb) |
| Central 90% after fit removal | 3.513 Hz | 0.161 Hz | 0.220 Hz |
| Phase-discontinuity result | 0 detected | 0 detected | not comparable; spectral estimator |
| Median carrier contrast | phase-tracked | phase-tracked | 20.48 dB in tracking lobe |

The 30 m signal was approximately 72 dB weaker at the receiver. Its amplitude
span is consequently noise-sensitive and should not be compared directly with
the 2 m amplitude span. Phase coherence remained sufficient for carrier
tracking.

## Drift by minute

| Minute | Si5351 2 m drift / movement (Hz) | Si5351 30 m drift / movement (Hz) | GPIO 80 m drift / movement (Hz) |
|---:|---:|---:|---:|
| 1 | -4.855 / 4.289 | -0.049 / 0.146 | +0.067 / 0.782 |
| 2 | +2.076 / 2.865 | -0.078 / 0.196 | -0.081 / 0.633 |
| 3 | +1.016 / 2.458 | -0.099 / 0.203 | +0.069 / 0.316 |
| 4 | -0.561 / 2.066 | -0.045 / 0.186 | +0.004 / 0.231 |
| 5 | -0.061 / 1.785 | +0.116 / 0.183 | -0.093 / 0.258 |

The drift did not remain constant. At 2 m, the minute-five slope was about
1.2 percent of the first-minute magnitude. The carrier therefore settled
substantially, although short-term excursions remained. At 30 m, the slope was
negative through minute four and reversed positive during minute five; this is
also inconsistent with a constant-drift model.

The GPIO slope also alternated sign. Its movement decreased from 0.782 Hz in
minute one to 0.231 and 0.258 Hz in minutes four and five. That is consistent
with a settling observation, not a constant-drift model. The minute-five GPIO
row covers the valid portion through 295 seconds; shutdown-overlap windows are
excluded.

These are hardware-characterization results, not software acceptance failures.

## Frequency history

The panels use different vertical scales. Each trace is one uninterrupted tone;
the peaks and valleys are measured variation, not WSPR tone changes.

![Five-minute relative carrier frequency](issue-379-si5351-stability/combined-relative-frequency-stability.png)

## Rolling drift rate

The rolling fit uses a 60-second window stepped every five seconds. It exposes
the slope changes hidden by a single five-minute linear fit.

![Rolling one-minute drift rate](issue-379-si5351-stability/combined-rolling-drift-rate.png)

## Interpretation

- Both bands produced a continuous, phase-coherent steady carrier for five
  minutes.
- The Si5351 path did not exhibit a fixed drift rate during either capture.
- The 2 m run provides evidence of substantial settling within five minutes.
- Settling does not mean the carrier becomes motionless; shorter excursions
  remain visible.
- The former proposed 0.5 Hz peak-to-peak drift limit is not used as a software
  pass/fail criterion. Drift is reported with the hardware and operating
  condition instead.
- These results characterize one hardware assembly and receiver chain. They do
  not define guaranteed WsprryPi behavior.
- The steady-carrier captures do not by themselves test tone spacing,
  transitions, symbol timing, or decoding. A separate bounded Si5351 frame
  test below covers those software-qualification questions.
- GPIO produced a persistent steady carrier through the valid 294.5-second
  observation. Its fractional-divider modulation required a spectral rather
  than phase-slope estimator, so phase-discontinuity results are not directly
  comparable with Si5351.
- GPIO was tested only at 80 m because of wspr4's hard LPF. These data must not
  be presented as an 80 m versus 2 m/30 m backend benchmark.
- GPIO's constant +43.4 Hz observation is retained only to identify the tracked
  lobe. It is not an accuracy result and is not a proposed PPM correction.

## Si5351 2 m bounded-frame qualification

One attenuated frame using the valid repository reference identity
`AA0NT EM18 20` was transmitted on `wspr5` at the standard 2 m WSPR frequency.
Random offset was disabled. The RSP1B captured the frame at fixed 25 dB gain
with zero overflows.

The transmitter completed the 162-symbol frame in 110.612 seconds and exited
successfully after its configured one-iteration bound. Independent WSJT-X
2.7.0 `wsprd` decoding recovered:

```text
1816  18  3.7 144.490506  1  AA0NT EM18 20
```

The decoder's 3.7 Hz/min drift estimate is a hardware observation and was not
treated as a symbol-spacing error. After separating slow carrier drift, the
four-tone fit measured 1.4849 Hz spacing against the 1.4648 Hz ideal, an error
of 0.0200 Hz.

The frame contained 116 boundaries where the tone changed. At 100 us envelope
resolution, the worst boundary bin was 1.61 dB below the median symbol
interior. No transition produced a carrier interruption reaching -6 dB. The
successful independent decode additionally verifies the complete symbol order
and usable transition timing under the measured drift.

After the bounded frame, Si5351 output-enable register 3 read `0xFF`, both
normal services were active, and no capture or transmitter process remained.
This satisfies the Si5351 four-tone, transition, complete-frame decode, bounded
shutdown, and post-frame RF-silence gates for Issue 379.

## Retained evidence

The repository retains the reviewed combined plots and backend-specific
machine-readable results beside this document:

- `summary.json`: whole-capture results
- `minute-summary.csv`: per-minute slopes and movement
- `rolling-60s-drift.csv`: rolling-slope series
- `frequency-series.csv`: all two-second carrier estimates
- `gpio-80m-summary.json`: GPIO whole-capture results and estimator contract
- `gpio-80m-minute-summary.csv`: GPIO per-minute slopes and movement
- `gpio-80m-rolling-60s-drift.csv`: GPIO rolling-slope series
- `gpio-80m-frequency-series.csv`: GPIO carrier estimates and contrast
- `gpio-80m-capture.log`: receiver identity, settings, samples, and overflows
- `gpio-80m-session.log`: transmitter conditions and duration
- `gpio-80m-quiesce.log`: post-run live hardware audit
- `si5351-2m-frame-summary.json`: concise frame and acceptance measurements
- `si5351-2m-transition-analysis.json`: transition-envelope measurements
- `si5351-2m-frame-session.log`: bounded session conditions and exit status
- `si5351-2m-frame-transmit.log`: application frame timing and shutdown
- `si5351-2m-frame-capture.log`: receiver identity, settings, and overflows
- `si5351-2m-frame-wsprd.log`: independent WSJT-X decode output

The larger working evidence remains outside the repository:

- local analysis bundle: `long-stability-results/`
- raw IQ on `wspr5`: `/home/pi/issue379-long-stability/`
- GPIO raw IQ on `wspr5`:
  `/home/pi/issue379-gpio-stability/gpio-80m-300s.cf32`
- Si5351 decoded-frame raw IQ on `wspr5`:
  `/home/pi/issue379-si5351-frame-valid/si5351-2m-frame-valid.cf32`

The steady-carrier IQ files are 600 MB each and the frame IQ file is 260 MB.
They are intentionally not committed to the repository.
