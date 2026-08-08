# Issue 379 conducted transmitter qualification procedure

Status: maintainer research procedure. This is not an operator tutorial, an
authorization to transmit, or a guarantee for other hardware.

This page records how Issue 379 tested the Si5351 and GPIO transmitter paths
with a highly attenuated conducted output and a local SDR. It is intended to
make the bench procedure repeatable after the immediate test setup and command
history are gone. The retained measurements and conclusions are in
[`issue-379-si5351-stability.md`](issue-379-si5351-stability.md).

## What the procedure establishes

The procedure separates four questions that must not be collapsed into one:

1. **RF silence:** Is the output quiet before startup and after shutdown?
2. **Carrier formation:** Does a steady tone coalesce into a usable carrier,
   rather than a broad comb or unrelated spurs?
3. **Hardware stability:** How does one uninterrupted carrier move with time?
4. **Software operation:** Are the four WSPR tones, transitions, frame timing,
   bounded execution, independent decode, and cleanup correct?

Absolute frequency accuracy is a fifth, separate measurement. It requires a
checked receiver reference and a hardware-specific calibration value. A
constant carrier offset must be removed before calculating stability. Neither
the transmitter nor receiver PPM measured on this bench becomes a project
default.

## Bench arrangement

Issue 379 used this conducted path, with no antenna:

```text
WsprryPi output -> fixed attenuator(s) -> shielded 50-ohm load / sample point
                -> local SDRplay RSP1B on wspr5 -> complex-IQ file
```

The Si5351 tests used 30 dB of series attenuation, minimum Si5351 drive, and a
shielded 50-ohm load. The same attenuated/load arrangement was transferred to
the GPIO output for the GPIO tests. Before repeating a test, calculate the
maximum possible SDR input from the transmitter's worst-case output and the
installed attenuation. Add attenuation if the result could exceed the SDR's
safe input or compression level.

The receiver was **local to `wspr5`**. The capture process opened the RSP1B
directly through SoapySDR; it did not stream samples through SoapyRemote. The
`soapyremote-server.service` service was stopped while the local capture owned
the SDR and restored during cleanup.

| Role | Issue 379 equipment and setting |
|---|---|
| Si5351 transmitter | `wspr5`, Raspberry Pi 5, Si5351 at I2C bus 1/address `0x60`, CLK0, 2 mA |
| Si5351 reference | ATX-11-F-27.000MHZ-F05-T, 27 MHz TCXO |
| GPIO transmitter | `wspr4`, Raspberry Pi 4 Model B Rev 1.1, BCM GPIO 4/GPCLK0, power 7 |
| Receiver | SDRplay RSP1B serial `2404058C60`, connected locally to `wspr5` |
| Sample format | Interleaved complex float32 (`CF32`) |
| Sample rate | 250,000 samples/second |
| Receiver bandwidth | 200 kHz |
| Receiver AGC | Off |
| Fixed gain | 25 dB for the Si5351 captures; 40 dB for the GPIO captures |

Those gains describe this bench, not required values. Choose a non-clipping
gain that gives a clear carrier, record the requested and actual gain, and do
not change it between RF-off, RF-on, or comparison captures.

When absolute accuracy matters, check the RSP1B reference against a known
signal such as WWV immediately before the transmitter measurement and record
the observed receiver error. Keep receiver correction and gain fixed through
the comparison. Chrony describes a Raspberry Pi system clock; it is not a
frequency reference for the SDR.

## Required preparation and record

Do not begin live hardware work until the exact host, backend, output pin,
frequency, duration, attenuation, load, receiver, and stopping procedure have
been confirmed. Use a bounded process and install cleanup handlers before
enabling RF.

Record these items in `session.log` before each run:

- UTC start and expected WSPR slot start;
- transmitter hostname, Raspberry Pi model, operating system, branch, parent
  commit, and WSPR-Transmitter submodule commit;
- backend, output, I2C address or GPIO, drive/power setting, requested RF
  frequency, and `PWM_CLOCKS_PER_ITER_NOMINAL` when testing GPIO;
- installed `Calibration.PPM`, whether GPIO NTP correction is enabled, and the
  complete `chronyc tracking` output when relevant;
- oscillator/reference description, warm-up condition, uptime, and CPU
  temperature;
- attenuator value, 50-ohm termination, filter state, and confirmation that no
  antenna is connected;
- SDR driver, model, serial, center frequency, sample rate, bandwidth, AGC
  state, requested gain, and actual gain; and
- names of all raw IQ, transmitter, capture, decoder, and analysis files.

Build the exact source under test on the transmitter host. On a Raspberry Pi,
use one fewer job than the processor count. Run the applicable non-hardware
tests before live RF. Stop scheduled transmission and verify the output is idle
before taking ownership of the hardware.

## Local fixed-gain IQ capture

Issue 379 retained a small SoapySDR helper at
`/home/pi/issue379-long-stability/fixed_capture` on `wspr5`. Its interface was:

```text
fixed_capture CENTER_HZ SAMPLE_COUNT GAIN_DB OUTPUT.cf32
```

It configured the local RSP1B for 250 kS/s, 200 kHz bandwidth, CF32 samples,
AGC off, fixed gain, and bias tee off. It discarded the first read, captured an
exact sample count, and logged the receiver identity, actual gain, start/end
monotonic times, overflow count, and output path. Its source was retained as
`/home/pi/issue379-long-stability/streaming_rsp1b_capture.cpp`. An earlier
in-memory prototype is retained beside it as `local_rsp1b_fixed_capture.cpp`;
the streaming source is the implementation matching the recorded overflow
field and exact-sample-count behavior.

The receiver was normally centered 25 kHz above the expected carrier. This
kept the signal away from receiver DC while retaining a wide view of nearby
spurs. Example captures were:

```text
# Five minutes: 75,000,000 samples
fixed_capture 144515500 75000000 25 2m-tone0-300s.cf32
fixed_capture 10165200  75000000 25 30m-tone0-300s.cf32

# One 2 m frame plus margins: 130 seconds
fixed_capture 144515500 32500000 25 si5351-2m-frame.cf32

# Two consecutive 2 m frames plus margins: 250 seconds
fixed_capture 144515500 62500000 25 si5351-2m-two-frames.cf32

# Three GPIO frames plus margins: 370 seconds
fixed_capture 28149500 92500000 40 gpio-10m-three-frames.cf32
```

Use `sample_count = sample_rate * duration`. Reject or repeat a capture with
overflows, clipping, an unexpected gain, the wrong receiver, or an incomplete
sample count.

## Phase 1: RF-silence gate

1. Connect the attenuated, terminated conducted path and local SDR.
2. Leave the transmitter disabled. Capture or observe the intended frequency
   and enough surrounding bandwidth to establish ambient and receiver noise.
3. Confirm that no output attributable to WsprryPi is visible.
4. Start and stop a bounded RF-inhibited or minimum-drive hardware exercise.
5. Repeat the observation after cleanup.
6. For Si5351, confirm output-enable register 3 is `0xFF`. For GPIO, confirm
   DMA and PWM are inactive, GPCLK0 is disabled/not busy, and the transmit GPIO
   is restored to input.

Do not proceed if the supposedly idle transmitter produces a measurable
carrier or if cleanup cannot restore the idle state.

## Phase 2: continuous-carrier gate

Start with a bounded steady tone, not a WSPR frame. Capture a wide enough span
to discover where transmitter-added energy actually appears; do not search
only the requested FFT bin.

For a short spectral capture, use a timeline that contains both RF-on and
RF-off samples at the same fixed SDR gain. The 30-second GPIO captures used
steady RF during the middle of the file, averaged seconds 7 through 20 as
RF-on, and averaged seconds 26 through 28 as RF-off.

Calculate spectra in linear power, then subtract the RF-off spectrum from the
RF-on spectrum. Issue 379 used a Hann window and a 262,144-point FFT. It
excluded the receiver center +/-1 kHz and inspected the full captured
+/-100 kHz region. Record:

- requested frequency and strongest transmitter-added frequency;
- offset between them;
- requested-bin power;
- strongest-bin power and RF-on/RF-off contrast;
- the strongest peaks across the full usable capture; and
- the fraction of resolved transmitter-added power contained in the best
  20 Hz channel.

The resolved-power calculation counted only positive RF-on minus RF-off bins
at least 6 dB above the RF-off baseline. This is a relative spectral
concentration measurement, not calibrated transmitter power, and it excludes
harmonics outside the capture.

If the tone does not form a stable, usable carrier, stop testing that
backend/band combination. Do not transmit WSPR frames merely because a narrow
search failed to find the carrier, and do not call a broad comb a carrier. A
failed carrier gate creates a band-specific unqualified result plus its RF-on,
RF-off, full-band spectrum, and cleanup evidence.

## Phase 3: long steady-carrier characterization

After the carrier gate passes, capture one uninterrupted tone for at least five
minutes. Record temperature before and after and do not change gain,
attenuation, PPM, or synthesis settings during the capture.

Issue 379 used 300-second captures at 2 m and 30 m for Si5351 and at 80 m for
GPIO. The Si5351 analysis:

1. translated the expected carrier to baseband;
2. coherently averaged into 10 ms blocks;
3. trimmed 0.5 seconds from each edge;
4. estimated frequency from linear phase slope in two-second windows stepped
   every 0.25 seconds; and
5. removed one constant frequency offset before calculating stability.

The GPIO fractional-divider waveform violated the Si5351 phase-fit assumption.
GPIO was therefore coherently integrated into 1 ms blocks and measured with a
two-second Hann-window spectral peak, stepped every 0.25 seconds, after a broad
discovery search identified the persistent carrier lobe. Document the
estimator used; do not compare incompatible estimators as if they were the
same instrument.

Create these results for each capture:

- raw and relative frequency series;
- whole-run linear drift in Hz/min and ppb/min;
- peak-to-peak and central-90-percent movement;
- detrended RMS and central-90-percent movement;
- per-minute slope and movement;
- rolling 60-second slope, stepped every five seconds;
- phase-residual/discontinuity result when the estimator supports it;
- carrier contrast or amplitude statistics; and
- relative-frequency and rolling-drift plots.

The reported relative stability includes both transmitter and SDR references.
Sequential captures cannot assign their difference exclusively to the
transmitter. Hardware drift is characterization, not a software failure.

## Phase 4: bounded WSPR frame qualification

Only a backend/band combination that passes the continuous-carrier gate
advances to frame testing.

1. Disable random frequency offset and use a known valid WSPR identity. Issue
   379 used `AA0NT EM18 20` in the conducted path.
2. Determine the next even UTC two-minute boundary.
3. Start the local fixed-gain IQ capture five seconds before that boundary.
4. Start WsprryPi with a bounded `--terminate` count. One frame used 130
   seconds of IQ; two consecutive frames used one coherent 250-second capture;
   three frames used one coherent 370-second capture.
5. Let the transmitter and receiver exit normally. Preserve both exit codes.
6. Verify post-run RF silence and hardware quiescence before restoring normal
   services.

The retained Si5351 invocation was equivalent to:

```text
wsprrypi_debug \
  --backend si5351 \
  --si5351-i2c-bus 1 \
  --si5351-i2c-address 0x60 \
  --si5351-reference-frequency 27000000 \
  --si5351-tx-output CLK0 \
  --si5351-power-level 1 \
  --ppm DEVICE_SPECIFIC_PPM \
  --no-offset --no-web --no-led --no-amp-pin \
  --terminate FRAME_COUNT AA0NT EM18 20 2m
```

Do not copy the Issue 379 PPM value to another device. If accuracy is not the
question being tested, hold the value fixed and separate constant offset from
spacing, transitions, timing, and stability.

## IQ conversion and independent decode

For each frame, translate the measured RF channel to 1500 Hz audio before
decoding. With complex IQ sampled at `fs`, SDR center `center_hz`, selected RF
carrier `carrier_hz`, and target audio `1500`, Issue 379 mixed by:

```text
mix_hz = (carrier_hz - center_hz) - 1500
audio[n] = real(iq[n] * exp(-j * 2 * pi * mix_hz * n / fs))
```

Write real float32 audio, convert/resample it to a decoder-compatible WAV, cut
each two-minute frame into its own correctly timestamped file, and run the
WSJT-X 2.7.0 `wsprd` decoder independently on each file. Retain the complete
decoder log, not only the expected line.

Record for every attempted frame:

- UTC slot, application-reported duration, transmitter/capture exit codes;
- decoded or not decoded;
- decoded callsign, grid, power, SNR, frequency, and drift;
- whether the decode is the intended positive-frequency signal or a conjugate
  image created by conversion to real audio; and
- receiver overflows and post-frame cleanup state.

Issue 379 required three independently decoded bounded frames for a qualified
usable path. A strong steady carrier alone is insufficient: the GPIO pacing
sweep produced strong tones at larger pacing values while every associated
WSPR frame failed to decode.

## Tone spacing and transition analysis

Use the complex IQ, not only the decoder report, to analyze one clearly
received frame:

- synchronize to the 162-symbol grid;
- estimate each symbol's frequency using the interior of the symbol;
- fit and remove slow carrier drift separately from the four-tone component;
- report recovered symbol order, mismatch count, fitted adjacent-tone spacing,
  three-interval span, model residual, and analysis uncertainty; and
- compare measured values with the current issue acceptance criterion without
  silently converting a marginal result into a pass.

For transition continuity, measure the carrier envelope around boundaries
where the WSPR tone changes. Issue 379 used 100 microsecond amplitude bins,
+/-5 ms around each changed boundary, and the median symbol-interior amplitude
as the baseline. Record the worst boundary bin and the longest contiguous
interval below -6 dB. Keep this result separate from decoder acceptance.

## GPIO band and pacing sweep

For GPIO, repeat the carrier gate and frame sequence per band. Hold all other
conditions fixed within a comparison. Issue 379 used production
`PWM_CLOCKS_PER_ITER_NOMINAL=1000` and temporary builds at 4000 and 16000, then
restored 1000 and rebuilt.

For each band and pacing value, retain:

- exact source commit and compiled constant;
- requested and strongest carrier frequencies and powers;
- full-band spectral view and best-20-Hz resolved-power share;
- frame attempts, independent decode count, SNR, and decoded-center spread;
- PPM and Chrony state, recorded but not substituted for synthesis quality;
- cleanup state; and
- final disposition: qualified, unqualified, or untested.

The decision order is important:

1. no usable carrier: stop, mark the backend/band unqualified, and do not run
   frames;
2. usable carrier but no decodes: synthesis/modulation remains unqualified;
3. usable carrier plus the required independent decodes and clean lifecycle:
   qualify only the tested backend, band, platform, and production pacing.

## Cleanup and restoration

Every wrapper must trap normal exit, interruption, and error. Cleanup must:

- stop any capture and transmitter child processes;
- disable all Si5351 outputs or stop DMA/PWM/GPCLK0 and restore GPIO input;
- verify the hardware idle state;
- restore `wsprrypi.service` and `soapyremote-server.service` to their prior
  intended states;
- confirm that no test process remains; and
- record cleanup evidence and service status.

Do not report a successful test if RF output, DMA, PWM, GPCLK, I2C output
enable, a helper process, or a stopped service is left behind.

## Information retained from a completed run

| Artifact | Information it preserves |
|---|---|
| `session.log` | Host, source, UTC timing, temperature, reference/calibration state, RF path, and exit codes |
| `capture.log` | SDR identity, center, rate, bandwidth, fixed gain, sample count, timing, overflows, and IQ path |
| `transmit.log` | Backend, planned RF frequency, PPM, frame count, start/completion timing, and shutdown reason |
| `*.cf32` | Original coherent complex-IQ evidence |
| RF-off and RF-on spectra | Carrier location, spurs/comb behavior, receiver baseline, and spectral concentration |
| `frequency-series.csv` | Time-indexed carrier estimates used by every stability summary and plot |
| minute and rolling CSV files | Settling behavior and whether drift remains constant or changes sign |
| summary JSON | Machine-readable settings, metrics, estimator contract, and disposition |
| per-frame WAV files | Decoder-ready, correctly timed evidence for each frame |
| `wsprd.log` | Independent decoder output, including failed or companion decodes |
| frame/transition JSON | Symbol recovery, tone spacing, drift-separated fit, and carrier continuity |
| quiescence log | RF-off register/peripheral state and restored services |

Commit the small reviewed logs, summaries, CSV files, and plots. Raw IQ files
are much larger and may remain on the measurement host, but the research page
must record their absolute paths, sizes, sample format, hashes, and the source
commit needed to interpret them. If raw IQ will not be retained, derive and
review every required result before deleting it.

## Issue 379 retained locations

- Reviewed report and plots:
  `docs/research/issue-379-si5351-stability.md` and its adjacent directory
- Si5351 long IQ: `/home/pi/issue379-long-stability/` on `wspr5`
- Si5351 first decoded frame:
  `/home/pi/issue379-si5351-frame-valid/` on `wspr5`
- Si5351 two-frame capture:
  `/home/pi/issue379-si5351-two-more-frames/` on `wspr5`
- GPIO 80 m stability IQ:
  `/home/pi/issue379-gpio-stability/gpio-80m-300s.cf32` on `wspr5`
- GPIO band/pacing captures:
  `/home/pi/issue379-gpio-{band}-{tone,3frame}-*` on `wspr5`

The original helper scripts remain in `/home/pi/issue379_*` and the long-tone
helpers remain in `/home/pi/issue379-long-stability/` on `wspr5`. They are
historical evidence, not supported project tools. Review them against the
current CLI, hardware-safety rules, and service layout before reuse.
