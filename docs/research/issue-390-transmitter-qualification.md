# Issue 390: remaining transmitter qualification

## Disposition

The initial conducted qualification completed all eight cells from Issue 390.
Three cells qualified and five did not meet the carrier gate. Later work
superseded the initial 2200 m GPIO result for BCM2711 systems and the initial
160 m Si5351 result, as described below.

| Band | Backend | Carrier gate | WSPR decode gate | Disposition |
|---|---|---:|---:|---|
| 2200 m | GPIO | Failed twice | Not run | Initially unqualified; superseded for BCM2711 |
| 2200 m | Si5351 | Planner rejected before RF | Not run | Unqualified |
| 630 m | GPIO | Passed | 3 of 3 | Qualified |
| 630 m | Si5351 | Failed twice | Not run | Unqualified |
| 160 m | GPIO | Passed | 3 of 3 | Qualified |
| 160 m | Si5351 | Failed twice | Not run | Initially unqualified; superseded by radiated follow-up |
| 12 m | Si5351 | Passed | 3 of 3 | Qualified |
| 1.25 m | Si5351 | Planner rejected before RF | Not run | Unqualified |

The results qualify only the named backend, band, hardware, and production
settings. GPIO evidence does not qualify Si5351, and Si5351 evidence does not
qualify GPIO. The 70 cm status was not tested or changed by this work.

## GPIO clock profile follow-up

Follow-up testing separated the two supported GPIO clock profiles rather than
assuming that one Raspberry Pi result applied to all models.

| Profile and test system | Band | Carrier gate | WSPR decode gate | Disposition |
|---|---|---:|---:|---|
| BCM2711, 750 MHz PLLD profile, Raspberry Pi 4 (`wspr4`) | 2200 m | Passed | 3 of 3 | Qualified |
| Legacy 500 MHz PLLD profile, Raspberry Pi Zero 2 W (`wspr2`) | 2200 m | Passed | 0 of 3 | Unqualified |
| Legacy 19.2 MHz oscillator experiment, Raspberry Pi Zero 2 W (`wspr2`) | 2200 m | Passed | 0 of 3 | Experimental path rejected |
| Legacy 500 MHz PLLD profile, Raspberry Pi Zero 2 W (`wspr2`) | 12 m | Not repeated | 0 of 3 | Unqualified |

The maintained BCM2711 implementation uses the 54 MHz oscillator when the
750 MHz PLLD divider cannot synthesize the requested low frequency. Its 2200 m
carrier was measured at 137,499.43 Hz, 0.57 Hz below the request, with 41.11 dB
on/off contrast and 78.06% of transmitter-added power in the best 20 Hz. Three
consecutive frames decoded `AA0NT EM18 20` at +8 dB.

The Raspberry Pi Zero 2 W produced a carrier near the requested frequency with
both its maintained 500 MHz PLLD path and an experimental 19.2 MHz oscillator
path, but neither path produced a WSPR decode in three consecutive frames. The
experimental oscillator change was not retained. The 12 m repeat also decoded
0 of 3 frames, bringing the observed Zero 2 W result to 1 of 8 across both test
runs; it remains insufficient for qualification. These results support
profile-specific qualification but do not prove that processor type alone is
the cause.

## Si5351 160 m follow-up

A radiated SDR follow-up on `wspr5` showed that the earlier conducted result
did not represent the Si5351 output. With the existing bare radiator in
parallel with the RSP1B antenna, eleven of twelve combinations of receiver gain
and center frequency passed the carrier gate. All captures at 10, 20, and
30 dB gain passed. At 40 dB, two of three passed; the deliberately unfavorable
capture with the carrier 75 kHz from center failed the resolved-power-share
threshold. Moving the receiver center did not move the carrier's absolute RF
frequency, and all four CLK0 drive strengths passed.

The final carrier and decode gates used the exact feature-branch build:

- WsprryPi: `fc1a60440c3431a35c005ffc57f4ac238f369041`;
- WSPR-Transmitter: `abfedbefc780516085b087499b67da0e19017f2c`;
- requested frequency: 1,838,100 Hz;
- Si5351 output: CLK0 at minimum 2 mA drive, R/1;
- SDR center: 1,863,100 Hz, 250 ksps, 200 kHz bandwidth, AGC off, 10 dB gain;
- measured carrier: 1,838,100.381 Hz, +0.381 Hz from the request;
- RF-on/RF-off contrast: 13.30 dB;
- best-20-Hz resolved-power share: 99.17%; and
- carrier gate: passed.

One coherent 370-second capture contained 92,500,000 samples with zero
overflows. Three independent UTC slots at 21:00, 21:02, and 21:04 on
2026-08-09 decoded `AA0NT EM18 20` at +2 dB, -0.9 seconds, and zero drift.
Transmitter and capture return codes were zero. Cleanup restored Si5351
register 3=`0xff` and both managed services to active.

This evidence qualifies the Si5351 CLK0 production path on 160 m for the exact
recorded configuration. It indicates that the earlier displaced features were
introduced by the conducted receiver arrangement or receiver overload rather
than by the WsprryPi synthesis plan. Raw IQ, logs, analyses, decoder output,
and manifests remain on `wspr5` under
`/home/pi/si5351-160m-diagnostic/`.

## Synchronized source boundary

The Mac, `wspr4`, and `wspr5` were clean and synchronized before the initial RF
testing:

- WsprryPi `devel`: `ea3e875a0237ed4550e3ac27931b021f7c484c25`
- WSPR-Transmitter: `47162cf6dc2d91ff664cc1268288808e9ae7eeb7`
- Wsprry_Pi_Docs `devel`: `8553d356454368183056a9b9e1775a19259c862c`

All recursive submodules matched their parent gitlinks and configured upstream
tips. Focused GPIO and Si5351 non-hardware tests passed before transmission.

The later GPIO clock-profile work was published at these source boundaries:

- WsprryPi `devel`: `dab4eaa5bb97bbdab974ff68105e076972ab944c`
- WSPR-Transmitter `main`: `a75b0a69c2ce296132f100f8020198cf0e3218db`
- Wsprry_Pi_Docs `devel`: `a46bf5c04d27fde93d3aaff07e804883124895b6`

## Test hardware

### GPIO

- transmitter: `wspr4`, Raspberry Pi 4 Model B Rev 1.1, Debian 13 arm64
- output: GPIO4/GPCLK0, power level 7
- pacing: production `PWM_CLOCKS_PER_ITER_NOMINAL=1000`
- calibration: current Chrony correction through `--use-ntp`

### Si5351

- transmitter: `wspr5`, Raspberry Pi 5 Model B Rev 1.0, Debian 13 arm64
- device: I2C bus 1, address `0x60`, CLK0, minimum 2 mA drive
- reference: Abracon ATX-11-F-27.000MHZ-F05-T 27 MHz TCXO
- calibration: `+2.353615654` PPM

### Receiver and conducted path

- receiver: SDRplay RSP1B serial `2404058C60`, local to `wspr5`
- capture: complex float32, 250 ksps, 200 kHz bandwidth, AGC off
- fixed gain: 40 dB for GPIO, 25 dB for the initial Si5351 captures
- path: user-confirmed attenuated conducted connection into a shielded 50-ohm
  load/sample point; no antenna

Si5351 630 m and 160 m were repeated at 40 dB receiver gain. GPIO 2200 m was
repeated at 45 dB. The higher-gain repeats did not change their dispositions.

## Carrier results

The carrier analysis compared fixed-gain RF-on and RF-off intervals in each
30-second capture. It used averaged Hann-window spectra and required the
strongest transmitter-added feature to be within 100 Hz of the request and at
least half of resolved transmitter-added power to occupy the best 20 Hz.

### Qualified carrier gates

| Cell | Strongest offset | Best-20-Hz share | On/off contrast |
|---|---:|---:|---:|
| GPIO 630 m | +4.20 Hz | 99.92% | 32.37 dB |
| GPIO 160 m | +21.36 Hz | 96.78% | 43.42 dB |
| Si5351 12 m | +2.29 Hz | 99.92% | 35.36 dB |

These absolute offsets include the transmitter and RSP1B reference errors and
do not become project calibration defaults.

### Unqualified carrier gates

- **GPIO 2200 m:** the initial and higher-gain repeats found their strongest
  transmitter-added features 15.7 kHz and 33.4 kHz below the requested
  137,500 Hz. Requested-bin contrast was +2.18 dB and -1.29 dB. No usable
  requested-frequency carrier was established.
- **Si5351 2200 m:** the production planner rejected 137,500 Hz as unusable
  before RF activation. Output-enable register 3 remained `0xff`.
- **Si5351 630 m:** neither the 25 dB nor 40 dB capture found a usable carrier
  at 475,700 Hz. Requested-bin contrast was -0.71 dB and -1.26 dB.
- **Si5351 160 m:** the initial capture showed a stronger feature about 44 kHz
  above the request and only 45.47% in the best 20 Hz. The 40 dB repeat had
  only +1.07 dB requested-bin contrast and no resolved best-20-Hz share.
- **Si5351 1.25 m:** the production planner rejected 222,101,500 Hz as
  unusable before RF activation. Output-enable register 3 remained `0xff`.

Cells that failed this gate did not transmit WSPR frames.

## WSPR decode results

Each passing cell used one 370-second coherent capture containing three
consecutive bounded frames. Each capture contained exactly 92,500,000 samples
with zero overflows. The frames were converted independently and decoded with
`wsprd`.

| Cell | UTC slots | Decodes | SNR |
|---|---|---:|---|
| GPIO 630 m | 01:28, 01:30, 01:32 | 3 of 3 | -18, -18, -18 dB |
| GPIO 160 m | 01:36, 01:38, 01:40 | 3 of 3 | -9, -9, -8 dB |
| Si5351 12 m | 01:44, 01:46, 01:48 | 3 of 3 | -5, -5, -5 dB |

Every accepted frame decoded `AA0NT EM18 20` correctly.

An earlier GPIO 630 m acquisition used placeholder identity `NXXX ZZ99 20`.
The decoder consistently recovered the encoder-normalized value
`O03X AA00 20`; those three captures were retained but invalidated as a test
configuration error and were not counted. The complete valid-identity run was
then repeated.

## Safety and cleanup

Every accepted bounded frame run returned transmitter and capture exit code
zero. GPIO4 was restored to input. The final Si5351 run forced and verified
output-enable register 3=`0xff`. The managed WsprryPi services on both Pis and
the SoapyRemote service on `wspr5` were active afterward.

During a non-qualifying higher-gain repeat, an interrupted temporary wrapper
left Si5351 register 3=`0xfe` after its bounded tone process had exited. The
condition was detected immediately, register 3 was forced to `0xff`, and the
wrapper was retired. No frame evidence depends on that wrapper; the later
Si5351 frame wrapper used explicit fail-safe register cleanup and verified
`0xff` in its session log.

## Evidence

Small reviewed logs, decoder results, JSON summaries, and SHA-256 manifests are
retained in
[`issue-390-transmitter-qualification/`](issue-390-transmitter-qualification/).
The later clock-profile results are in its
[`gpio-profile-followup/`](issue-390-transmitter-qualification/gpio-profile-followup/)
subdirectory. That directory also preserves the exact uncommitted diffs from
the superseded maintained-validation prototype and the rejected fixed-source
oscillator experiment. Temporary complex-IQ and decoder WAV captures were
removed from the test systems after the reviewed summaries and capture hashes
were retained.

Successful WSPR decoding does not establish antenna-ready spectral compliance.
Filtering, harmonics, spurs, absolute output power, and the assembled station
remain hardware-specific responsibilities.
