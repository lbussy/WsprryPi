# Issue 401: wspr4 BCM2711 GPIO requalification

Date: 2026-08-15

## Outcome

The current Issue 401 build was exercised on `wspr4`, a Raspberry Pi 4 using
the BCM2711 750 MHz PLLD GPIO backend. A manual correction of 10.8511 PPM moved
the fresh 20 m control from +152.969 Hz to +13.733 Hz. The closing control was
+12.779 Hz, confirming that the conducted receive path remained valid.

| Band | TONE carrier result | QRSS | FSKCW/DFCW | WSPR | Disposition |
|---|---|---|---|---|---|
| 12 m | Failed: -17,131.424 Hz; 11.03% best-20-Hz share | Suppressed | Suppressed | Suppressed | Unqualified |
| 6 m | Passed 3/3: +63.324, +58.556, and +59.509 Hz; minimum 58.32% share | Untested | Untested | Failed: 2 consecutive correct decodes of 3 required | Partial; TONE only |
| 4 m | Failed: +2,146.149 Hz; 0.0515% share | Suppressed | Suppressed | Suppressed | Unqualified |
| 2 m | Failed: -51,022.148 Hz; 0.0593% share | Suppressed | Suppressed | Suppressed | Unqualified |
| 1.25 m | Planner rejected before RF | Unavailable | Unavailable | Unavailable | Unavailable |
| 70 cm | Planner rejected before RF | Unavailable | Unavailable | Unavailable | Unavailable |

Each carrier analysis used the formal ±100 Hz and 50% best-20-Hz share gates.
Every analyzed pair had zero clipped samples. WSPR was attempted only on 6 m,
the sole questionable band whose carrier prerequisite passed.

A coalesced continuous tone qualifies TONE. It does not qualify QRSS keying or
the wider tone spacing and transitions used by FSKCW and DFCW. The maintained
qualification harness does not yet produce positive live keyed-mode evidence,
so those three modes remain untested on 6 m.

## WSPR evidence

The retained 6 m capture contains exactly 92,500,000 CF32 samples at 250,000
samples per second, with zero overflow and zero clipped samples. Independent
`wsprd` processing correctly decoded the 23:40 and 23:42 UTC frames. The 23:44
frame did not decode, leaving a maximum of two consecutive correct decodes and
failing the three-consecutive-decode gate.

## Source and bench binding

- WsprryPi source under test: `854b39d37433c5b98d4ed43784f0b9819cf6143e`
- WSPR-Transmitter source under test: `332a318417c9e92f5137f6254647a6e656826e02`
- Transmitter: `wspr4`, Raspberry Pi 4, GPIO4, 750 MHz PLLD
- Receiver: SDRplay RSP1B serial `2404058C60` on `wspr5`
- Capture: CF32, 250 ksps, 200 kHz bandwidth, fixed 10 dB gain
- RF path: conducted connection with two inline 10 dB attenuators
- Correction: `--gpio-manual-ppm 10.8511`

These are relative captured-span measurements. They do not establish absolute
power, harmonic suppression, occupied bandwidth, or regulatory compliance.

## Evidence integrity and adversarial review

The immutable working evidence is retained on `wspr5` at:

```text
/home/pi/issue401-wspr4-gpio-qualification-20260815
```

Its 111-entry `SHA256SUMS` manifest verified without error. The manifest
SHA-256 is:

```text
c71cc19d8dee1bea870f602768351168ec8a448a6572f34b6a6bbfb22931f5dd
```

The earlier directory
`/home/pi/issue401-wspr4-gpio-wspr-requalification-20260814` remains
path-invalid and contributes no qualification claim. The fresh session is
bracketed by passing 20 m controls, so its candidate failures are not
reclassified as path failures. Planner rejection before RF is recorded as
Unavailable rather than as a failed transmission, and experimental overrides
must not bypass it. Each bounded run returned GPIO4 to input; after the session
the wspr4 service was inactive and no transmitter process remained.

## Qualification boundary

This record applies only to the recorded BCM2711 GPIO hardware, source
revisions, correction, frequencies, receiver, and conducted path. It does not
qualify the legacy 500 MHz PLLD profile, RP1 GPIO, Si5351, another receiver
path, or another WsprryPi revision.
