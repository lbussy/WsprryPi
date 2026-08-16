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
| 6 m | Passed 3/3: +63.324, +58.556, and +59.509 Hz; minimum 58.32% share | Qualified: 3/3 keyed repetitions | FSKCW qualified: 3/3 shifted-tone repetitions; DFCW unqualified after its closing carrier control failed | Failed: 2 consecutive correct decodes of 3 required | Partial; TONE, QRSS, and FSKCW |
| 4 m | Failed: +2,146.149 Hz; 0.0515% share | Suppressed | Suppressed | Suppressed | Unqualified |
| 2 m | Failed: -51,022.148 Hz; 0.0593% share | Suppressed | Suppressed | Suppressed | Unqualified |
| 1.25 m | Planner rejected before RF | Unavailable | Unavailable | Unavailable | Unavailable |
| 70 cm | Planner rejected before RF | Unavailable | Unavailable | Unavailable | Unavailable |

Each carrier analysis used the formal ±100 Hz and 50% best-20-Hz share gates.
Every analyzed pair had zero clipped samples. WSPR was attempted only on 6 m,
the sole questionable band whose carrier prerequisite passed.

A coalesced continuous tone qualifies TONE; it does not by itself qualify a
keyed or shifted mode. Follow-up acquired-IQ testing independently qualified
QRSS and FSKCW on 6 m. DFCW remains unqualified because the final control after
its corrective captures exceeded the ±100 Hz carrier-offset gate.

## 6 m keyed-mode follow-up

Three QRSS `AE T` repetitions reproduced the expected dots, dashes, element
gaps, character gaps, and word gap with at least 99.94% template agreement.
Three FSKCW `AE T` repetitions kept RF active and shifted at every expected
transition. Their median local mark/space separations were 4.574, 5.047, and
5.007 Hz; all 18 transitions moved in the commanded direction. Every retained
positive capture contained exactly 7,500,000 CF32 samples with zero clipping
and zero overflow.

The keyed session opened with a passing 6 m control at +74.768 Hz and 86.32%
best-20-Hz share. The control bracketing QRSS and FSKCW closed at +95.749 Hz
and 89.37% share. Corrective DFCW `AA AA` captures showed the expected timing,
gaps, and 5 Hz separation, but their final control failed at +107.193 Hz. A
post-cooldown replacement opening control also failed at +109.100 Hz, so no
further DFCW candidate run was permitted and no DFCW qualification is claimed.

The keyed follow-up used WsprryPi revision
`71bef1d4b38d3810cab974bd3ddcd1928dd1a273` and WSPR-Transmitter revision
`b80766cb1841f43bbc3a2bb5220a0f31337c54fd`.

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

The keyed-mode follow-up evidence is retained at:

```text
/home/pi/issue401-wspr4-6m-keyed-qualification-20260815
```

Its 124 artifacts include raw IQ, capture metadata, analyses, plans, logs,
failed-attempt records, and the manifest. The verified `SHA256SUMS` file has
SHA-256 `3cab95700bab6c0d99fa3c61f7333474620c581993952a487a3de1732fa74c72`.
The audit excludes scheduled-mode idle attempts, overlapping capture failures,
and DFCW captures that were not bracketed by a passing closing control.

## Qualification boundary

This record applies only to the recorded BCM2711 GPIO hardware, source
revisions, correction, frequencies, receiver, and conducted path. It does not
qualify the legacy 500 MHz PLLD profile, RP1 GPIO, Si5351, another receiver
path, or another WsprryPi revision.
