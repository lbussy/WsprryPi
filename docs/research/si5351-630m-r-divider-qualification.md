# Si5351 630 m R-divider qualification

## Result

WsprryPi's Si5351 backend passed the bounded SDR carrier and WSPR decode gates
on 630 m using CLK0 and a final R divider of 4. This result is limited to the
exact hardware, software, and radiated test arrangement recorded below.

The tested synthesis plan was:

- requested RF: 475,700 Hz;
- parked PLL: 850 MHz;
- final R divider: 4;
- internal MultiSynth output: 1,902,800 Hz; and
- MultiSynth ratio: approximately 446.71.

Live register readback during the carrier test reported Si5351 register
44=`0x20`. Bits 6:4 therefore contained R-divider code 2, which selects R/4.
Output-enable register 3 was `0xfe` during RF and returned to `0xff` during
cleanup.

## Software under test

- parent branch: `codex/si5351-lf-r-divider`;
- qualified parent commit: `1799a22355e0250e4384418414fae550e4d9fa91`;
- qualified WSPR-Transmitter commit:
  `fd35ddf26e42b6d0c68f8e5e530a5b5589f00e1e`; and
- final branch commits retain the same R/4 630 m plan while restoring the
  separately unqualified 2200 m plan to the documented R/8 policy.

The Si5351 planner, transition, fake-device startup/cleanup, parent semantics,
and debug-build gates passed on `wspr5` before RF testing.

## Hardware and capture configuration

- transmitter: `wspr5`, Raspberry Pi 5 Model B Rev 1.0;
- synthesizer: Si5351 at I2C bus 1/address `0x60`;
- output: CLK0, minimum 2 mA drive;
- reference: ATX-11-F-27.000MHZ-F05-T 27 MHz TCXO;
- configured calibration: `+2.353615654` PPM;
- radiated path: the existing bare radiator in parallel with the local SDR
  antenna;
- receiver: SDRplay RSP1B serial `2404058C60`, local to `wspr5`;
- capture: CF32 at 250 ksps, 200 kHz bandwidth, AGC off, fixed 25 dB gain; and
- receiver center: 500,700 Hz.

This was a radiated SDR validation. It was not a conducted, oscilloscope,
matched-load, filtered-output, harmonic, or regulatory qualification.

## Carrier gate

The conservative R/4 plan passed the retained Issue 390 carrier rule:

- strongest transmitter-added frequency: 475,700.381 Hz;
- strongest-frequency offset: +0.381 Hz;
- RF-on/RF-off contrast: 33.04 dB;
- best-20-Hz resolved-power share: 83.07%; and
- required gate: strongest feature within 100 Hz of the request and at least
  50% of resolved transmitter-added power in the best 20 Hz channel.

The earlier R/2 plan placed the internal MultiSynth at 951.4 kHz. It produced
the strongest feature at the requested frequency but achieved only 9.91% in
the best 20 Hz and did not pass the carrier gate.

## WSPR decode gate

One coherent 370-second capture contained exactly 92,500,000 samples with zero
overflows. Three bounded production-path transmissions used identity
`AA0NT EM18 20`, disabled random offset, and completed normally.

Each UTC slot was translated independently to 1,500 Hz audio and decoded with
`wsprd`:

| UTC slot | Decode | SNR | Time offset | Drift |
|---|---|---:|---:|---:|
| 2026-08-09 19:52 | `AA0NT EM18 20` | -17 dB | -1.1 s | 0 Hz/min |
| 2026-08-09 19:54 | `AA0NT EM18 20` | -17 dB | -1.2 s | 0 Hz/min |
| 2026-08-09 19:56 | `AA0NT EM18 20` | -17 dB | -1.1 s | 0 Hz/min |

Transmitter and capture return codes were zero. Final cleanup verified
register 3=`0xff`, `wsprrypi.service` active, and
`soapyremote-server.service` active.

## Scope and retained evidence

This result qualifies WsprryPi's Si5351 CLK0 production path on 630 m for the
exact configuration above. It does not qualify other Si5351 boards, reference
oscillators, outputs, Raspberry Pi models, output networks, filters, antennas,
or power levels by inference.

It does not qualify 2200 m. R/8 and R/16 2200 m carrier tests both failed the
same SDR carrier gate, so no 2200 m WSPR frames were transmitted.

Raw IQ, transmitter and capture logs, analyses, WAV files, decoder results,
and SHA-256 manifests remain on `wspr5` under:

`/home/pi/si5351-lf-r-divider-validation/`
