# Issue 401: wspr2 legacy GPIO requalification

Date: 2026-08-14

## Outcome

The current Issue 401 build was exercised on `wspr2`, a Raspberry Pi Zero 2 W
using the legacy 500 MHz PLLD GPIO backend. The conducted carrier evidence
passes the coalesced-carrier prerequisite on 2200 m and 12 m for this recorded
hardware and source combination. Neither band passed the separate WSPR gate,
which requires three consecutive correct decodes. Mode-specific keyed or
multitone evidence remains separate from this carrier requalification.

The higher-frequency candidates did not pass the carrier prerequisite. No WSPR
frames were transmitted for those bands.

| Band | Requested frequency | Carrier result | WSPR result | Issue 401 disposition |
|---|---:|---|---|---|
| 2200 m | 137,500 Hz | Pass: -1.526 Hz offset; 99.8855% best-20-Hz share | 1 maximum consecutive correct decode of 3 required | Carrier prerequisite passed; WSPR unqualified |
| 12 m | 24,926,100 Hz | Pass: -31.090 Hz offset; 99.9944% best-20-Hz share | 1 maximum consecutive correct decode of 3 required | Carrier prerequisite passed; WSPR unqualified |
| 6 m | 50,294,500 Hz | Fail: -28,177.834 Hz offset; 0.1817% best-20-Hz share | Suppressed | Unqualified |
| 4 m | 70,092,500 Hz | Fail: +16,523.743 Hz offset; 0.2899% best-20-Hz share | Suppressed | Unqualified |
| 2 m | 144,490,500 Hz | Fail: +14,951.134 Hz offset; 0.5146% best-20-Hz share | Suppressed | Unqualified |
| 1.25 m | 222,101,500 Hz | Fail: -830.269 Hz offset; 1.0872% best-20-Hz share | Suppressed | Unqualified |
| 70 cm | 432,301,500 Hz | Fail: +62,674.904 Hz offset; 3.2870% best-20-Hz share | Suppressed | Unqualified |

Passing carrier evidence establishes a coalesced continuous tone and the
carrier prerequisite used by CW-family review. It does not independently
qualify QRSS keying or the wider tone spacing and transitions used by FSKCW and
DFCW. It is not a WSPR qualification and does not establish calibrated power,
harmonic suppression, occupied bandwidth, or regulatory compliance.

## WSPR evidence

The 2200 m and 12 m carrier gates passed before their WSPR runs. Each retained
coherent capture contained exactly 92,500,000 CF32 samples at 250,000 samples
per second, with zero overflow and zero clipped samples. Each covered three
planned consecutive UTC slots.

- 2200 m slots: 21:20, 21:22, and 21:24 UTC. Maximum consecutive correct
  decodes: 1.
- 12 m slots: 21:44, 21:46, and 21:48 UTC. Maximum consecutive correct
  decodes: 1.

One correct decode demonstrates that a frame was receivable; it does not meet
the three-consecutive-decode qualification contract. Both bands therefore
remain WSPR-unqualified.

## Source and bench binding

- WsprryPi source: `5109d24d39a4f832d31865014e1bfdee21249dd6`
- WSPR-Transmitter source: `332a318417c9e92f5137f6254647a6e656826e02`
- Transmitter: `wspr2`, Raspberry Pi Zero 2 W, GPIO4
- Clock profile: legacy 500 MHz PLLD
- Receiver: SDRplay RSP1B serial `2404058C60` on `wspr5`
- Capture format: CF32, 250 ksps, 200 kHz bandwidth, fixed 10 dB gain
- RF path: direct conducted connection with two inline 10 dB attenuators;
  relative measurements only

The direct numeric `0.136` command path exposed a separate input-resolution
defect during testing: one form incorrectly required the non-amateur override,
and the fully overridden form reached the selector as 0 Hz. The canonical
`2200m` selector emitted 137.500 kHz and was used for the authoritative run.
This CLI defect does not invalidate the canonical-selector RF evidence and is
not corrected by this research record.

## Evidence integrity

The immutable working evidence is retained externally on `wspr5` at:

```text
/home/pi/issue401-wspr2-gpio-wspr-requalification-20260814
```

Its `SHA256SUMS` manifest covers 139 retained artifacts and verified without an
error after finalization. The manifest SHA-256 is:

```text
1bba1df26d9e1f2a71936c8f511a9674e0e9308eae3622cf085776626456f117
```

The bundle retains superseded inconclusive analyses and interrupted-run
artifacts for audit history. The authoritative carrier analyses use the `-v2`
suffix. The final WSPR decisions are in
`2200m-v4-decode-summary.json` and `12m-wspr-decode-summary.json`.

## Qualification boundary

This record applies only to the recorded legacy GPIO hardware, source
revisions, frequencies, receiver, and conducted path. It does not qualify
BCM2711 GPIO, RP1 GPIO, Si5351, another PLL profile, another receiver path, or
another WsprryPi revision.

The separately acquired wspr4 bundle failed known-frequency controls and is
classified as fixture/path-invalid. It is intentionally excluded from this
wspr2 result and makes no change to existing wspr4 qualification status.
