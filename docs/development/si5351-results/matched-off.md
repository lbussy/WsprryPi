# Matched comparison with experiments off — 2026-09-08

**Both bands completed all four scenarios with the three experimental switches off.** This fills the missing matched before/after pair for the final default behavior. It does not erase the earlier intermittent SYS_INIT failures or establish release reliability.

The final source is 369a53d, executable SHA-256 `69dd940d1d90c30729bc5fdfa10440ae3e6b1e07091ad84f0d42df19b3241dba`. No source, installed binary, or installed configuration changed during this follow-up. The same final build contains fade timing, drive retention, full-chain frequency reporting, and normal readiness checks. Experimental PLL-only, integer-MS and burst/cache options were all false (`extra: []` in each identity record).

The controls are the existing `before-r2-40m` and `before-r2-2m` captures from the expanded devel baseline. The after records are `matched-off-r1-40m` and `matched-off-r2-2m`. All requested RF/receiver settings match: wspr4 CLK0 at 2 mA, external 27 MHz TCXO, +3.470680 PPM; fixed user-confirmed attenuation/combiner path; wspr5 RSP1B 2404058C60; simultaneous locked GPSDO Output 1. Frequencies, bandwidth, gain and sampling match [the campaign protocol](README.md#baseline-and-fixed-conditions). This is comparison against the saved baseline, not a new interleaved before/after temperature-controlled trial.

| Metric | 40 m before | 40 m after | 2 m before | 2 m after |
|---|---:|---:|---:|---:|
| GPSDO-corrected carrier offset (Hz) | +0.3217 | -0.2542 | +6.4754 | -5.9864 |
| Transition gap metric (ms) | 0 | 0 | 3 | 3 |
| Extrapolated absolute phase step (rad) | 0.0136 | 0.0184 | 1.0968 | 2.0505 |
| Hard-key detected on interval (ms) | 502.0 | 502.0 | 502.0 | 501.0 |
| Fade detected on interval (ms) | 538.0 | 486.5 | 531.0 | 494.0 |
| Carrier concentration (%) | 95.7002 | 98.4367 | 99.9671 | 99.5811 |
| Transition concentration (%) | 95.5086 | 98.2784 | 98.8919 | 98.5554 |
| Hard-key concentration (%) | 91.0678 | 95.6842 | 98.8633 | 98.4832 |
| Fade concentration (%) | 89.3028 | 93.2642 | 96.8681 | 97.6356 |

The gap metric counts 1 ms samples below -10 dB after 5 ms smoothing; zero means no resolved gap by this method. Key intervals are narrow-channel threshold measurements, not exact programmed event duration. Concentration is power within +/-20 Hz of the indicated carrier divided by power within +/-5 kHz, including noise, not occupied bandwidth or calibrated phase noise.

## What this supports

- The combined default behavior ran successfully at both bands. Fade timing no longer accumulated the previous overrun: the detected intervals shortened from 538 to 486.5 ms at 40 m and from 531 to 494 ms at 2 m for nominal 500 ms keys. Hard-key timing stayed about 501–502 ms.
- With PLL-only disabled, the 2 m reset/inhibition gaps remain: the median gap metric is 3 ms both before and after. The improved transition continuity from the earlier experimental runs is not part of these three retained fixes.
- Fading remains full-amplitude chopping. Its concentration improved versus the old fade, but is still lower than hard keying in the same after run. The wide-channel edge plots show the pulses directly.
- The 2 m carrier and hard-key controls contain stronger periodic spurs than the original baseline; similar variation occurred in earlier stages. The 40 m relative noise floor differs too. These observations prevent a blanket cleaner-signal claim or attributing all spectral differences to the three fixes.
- Drive was held at 2 mA, which already matched the old overwritten default. This matched run does not demonstrate the higher-drive retention fix; that remains supported by the earlier register-level tests at 2/4/6/8 mA.
- The full-chain calculation fixes reported frequency without changing the ordinary programmed register image. The measured carrier offsets changed with frozen PPM; they do not show that the reporting fix moved or calibrated the RF carrier.

## Captures, rejection, and shutdown

Eight successful captures were validated for raw hashes, exact receiver settings/identity, retained sample counts, zero overflow/clipping, and GPSDO lock coverage. [The manifest](matched-off-manifest.json) binds the IQ paths and executable; [machine-readable measurements](matched-off-measurements.json) retain the table values.

The first 2 m setup (`matched-off-r1-2m`) failed receiver identity resolution before any Si5351 transmission or sample read. Its [failure metadata](matched-off-r1-2m/receiver-failure.json) and [cleanup](matched-off-r1-2m/cleanup.json) are retained. Subsequent SoapySDR enumeration found exactly the expected RSP1B serial; a separate new attempt completed. No service restart, USB reset, receiver substitution, or RF-setting change was used to obtain it.

Both completed runs had no readiness failure. All attempts verified Si5351 register 3 = 255, GPSDO Output 1/2 and PPS off, wspr4 service active, and unchanged installed-file hashes. Earlier SYS_INIT failures remain unresolved evidence; these successful captures do not close that reliability question.

## Rendered comparisons

- [40 m before/after PDF](matched-off-r1-40m/versus-before.pdf) and [wide-channel key edges](matched-off-r1-40m/keying-edges.pdf).
- [2 m before/after PDF](matched-off-r2-2m/versus-before.pdf) and [wide-channel key edges](matched-off-r2-2m/keying-edges.pdf).

Used the unchanged campaign tools: `analyze.py DIRECTORY`, `compare.py BEFORE AFTER`, and `edges.py DIRECTORY`, with NumPy/Matplotlib. Both before/after overlays and the 2 m wide-channel edge plot were visually inspected. No software tests were rerun because no application or analysis implementation changed. Earlier component and parent test results remain source-bound to this executable version.

## Adversarial reassessment

Checked all experiments were off, exact executable identity matched, before/after settings matched, every successful raw hash/health gate passed, all cleanup records passed, and the failed receiver setup was preserved. Kept higher-drive and frequency-calculation claims separate from the fixed-2 mA RF evidence; retained spectral deterioration and historical SYS_INIT failures. No further actionable report finding remained after these qualifications.

## Documentation Impact

Added this follow-up, plots, identities and capture records; linked it from the campaign report. No component or operator behavior changed in this follow-up. The previously listed sibling operator-documentation follow-up remains outstanding. The feature branch contains evidence changes only; the original working checkout is preserved.
