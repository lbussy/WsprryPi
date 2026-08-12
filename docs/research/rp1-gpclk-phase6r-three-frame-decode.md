# Phase 6R: three-frame Pi 5 GPIO WSPR decode qualification

## Outcome

Pi 5 GPIO WSPR met the three-independent-decode qualification gate on
`wspr5.local`. Three separate production scheduler processes transmitted three
complete 162-symbol frames through the RP1 provider on GPIO4 at the minimum
2 mA pad drive. Three separate RSP1B captures independently decoded the
required payload:

```text
AA0NT EM18 20
```

Every scheduler process reached provider `COMPLETE`, returned zero, and restored
GPIO/clock state. The run ended with provider live output disabled and both
services active.

This qualifies the tested Pi 5/RP1 GPIO4, 20 m, WSPR, 2 mA path. It does not
qualify other bands, GPIO20, higher drive settings, CW, or calibrated power.

## Preflight and identity

- Parent tip: `eea6bfb6771db00e2f6833b5c8269d01d03dcab3`.
- Transmitter tip: `d6eb8bb6568d612483b48c6ccf7181449cfaa06e`.
- Kernel: `6.18.44-v8-16k+ #3`.
- Provider source version: `00435149E8EC6D24857F8C1`.
- Source files on wspr5 matched the pushed Phase 6Q provider tip.
- Initial state: `live_output=N`, GPIO4 input, GPCLK0 prepare/enable counts
  zero, WsprryPi and SoapyRemote active.
- Receiver: SDRplay RSP1B serial `2404058C60`, 250 ksample/s, 25 dB requested
  gain, center frequency 14.122100 MHz.

Portable sequential-owner, static kernel contract, and KUnit preflight passed;
KUnit reported 3 pass, 0 fail, and 0 skip. The final clock-disabled production
frame completed in `110.719829` seconds with return code zero.

## Live frames and decodes

| Frame | Scheduler RF | Provider duration | Capture | Primary `wsprd` result |
|---|---:|---:|---|---|
| 1 | 14.097044 MHz | 110.719043 s | rc 0, 0 overflows | +28 dB, DT −2.5 s, 14.097040 MHz, `AA0NT EM18 20` |
| 2 | 14.097062 MHz | 110.708493 s | rc 0, 0 overflows | +29 dB, DT −4.2 s, 14.097057 MHz, `AA0NT EM18 20` |
| 3 | 14.097071 MHz | 110.695521 s | rc 0, 0 overflows | +29 dB, DT −2.0 s, 14.097069 MHz, `AA0NT EM18 20` |

Each frame came from a new WsprryPi process and a new provider lease, so the
run also exercised the Phase 6Q cross-process generation correction three more
times.

Frames 2 and 3 contained a weaker copy of the same valid payload 120 Hz above
the primary decode. This is recorded as a receiver/image artifact candidate;
it did not prevent or replace the primary decode and is not treated as another
independent transmission.

## Relative frequency and decode conversion

The first offline conversion used only the nominal center-to-dial difference of
26,500 Hz and produced no decode. Raw capture contrast located the actual live
features around −25,679, −25,665, and −25,653 Hz relative to the SDR center,
approximately 600–625 Hz below the nominal capture position. This reproduces
the earlier relative SDR/readback discrepancy.

The same retained captures were translated by a fixed relative 27,120 Hz,
placing all three primary signals near the normal 1,500 Hz WSPR audio passband.
No absolute RF or power calibration was applied or claimed. Each 120-second
capture was independently converted to aligned 12 kHz mono WAV and decoded by
`wsprd`. The primary decoder frequencies were within approximately 2–5 Hz of
the scheduler-reported RF values after this relative translation.

## Spectrum and all-boundary continuity

Relative live-to-baseline contrast was:

- frame 1: 37.87 dB;
- frame 2: 35.54 dB; and
- frame 3: 38.25 dB.

Offline carrier-envelope analysis located four tone clusters in every capture.
The fitted tone-step estimates were affected by within-frame common drift,
especially in frame 1, but the expected narrow four-tone structure and complete
decodable symbol sequence were present in all three captures.

All 161 symbol boundaries in each frame were evaluated using the mean RF
envelope in a centered 20 ms window after detecting each capture's actual RF
start. The worst boundary levels relative to the active-frame median were:

- frame 1: −0.092 dB;
- frame 2: −0.036 dB; and
- frame 3: −0.067 dB.

The fifth-percentile boundary levels were −0.061, −0.028, and −0.047 dB,
respectively. No boundary-scale RF dropout was observed.

## Cleanup and compatibility

The live harness used unconditional cleanup for success, failure, timeout, or
signal. It unloaded the live provider and restored the boot-installed provider
with `live_output=N`. The final verified state was GPIO4 input, GPCLK0
prepare/enable counts zero, and both WsprryPi and SoapyRemote active.

The qualification changes no Pi 4-and-earlier GPIO behavior, Si5351 behavior,
web UI, operator configuration, or power selection. It does not establish
absolute output power or spectral-regulatory compliance.

## Repository state

Phase 6R changed no production source. This report and its compact evidence
summary are uncommitted parent-repository additions. Raw evidence, including
three 240 MB CF32 captures, aligned WAV files, scheduler and capture logs,
decoder output, continuity analyses, safe-state records, and a SHA-256 manifest,
is retained at `/home/pi/phase6r-evidence`.

## Documentation impact

This core-repository engineering report records the qualification result. No
operator documentation was changed in this phase. Operator documentation may
now describe only the specifically qualified Pi 5 GPIO4, 20 m, WSPR, 2 mA path
after the installation and configuration workflow is reviewed. Developer
documentation still needs the kernel build dependencies, persistent module
installation, post-reboot identity checks, and lease-generation contract.

CW qualification and the later operator power-selection workflow remain future
work.
