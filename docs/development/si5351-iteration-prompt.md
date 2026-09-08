# Si5351 execution prompt and initial review

## Objective and authority

Implement and evaluate the five agreed Si5351 investigations in order, with
adversarial review, repair and reassessment after each slice. Commit and push
reviewed work on `codex/si5351-comparison-iterations`, created from refreshed
`origin/devel` a523904. Preserve the unrelated dirty WTP checkout. The user has
authorized the implementations and conducted tests; no reboot, physical RF-chain
change or unrelated component/UI work is in scope.

## Orientation review

The current implementation is in `src/WSPR-Transmitter`. Recent component
history includes c32e5f7 (detected Si5351 address selection), lifecycle/failure
handling and RP1 integration. The PLL/divider architecture below still matches
the earlier review; these five optimizations have not already landed.

- Planner: bounded rational approximation; default 850 MHz PLL, fractional
  MultiSynth, common R-divider. Direct 2m TONE/WSPR uses per-tone fractional PLL
  with divide-by-6, full parameter/control/reset writes and output inhibition.
- Backend: caches active tone index, inhibits 2m changes, checks cancellation
  between individual writes, cleans up on failure. No explicit PLL-ready wait.
- Device: individual I2C transactions; writeRegisters loops; cache does not
  suppress unchanged writes. Failures require careful invalidation.
- Envelope: RF duty-cycle slicing, not analog amplitude shaping. Its waits add
  software/I2C overhead; ordinary carrier captures do not test this path.
- Frequency reporting: ordinary path uses requested parked PLL rather than
  the rationally realized PLL. The retuned PLL path already uses its realized
  ratio.

## Fixed experiment contract

wspr4, external 27 MHz TCXO, I2C1/0x60, CLK0, 2 mA. CLI comparison correction
+3.470680 PPM, unchanged between experiments. wspr5 RSP1B 2404058C60 at 250 ksps,
200 kHz bandwidth, 20 dB gain, AGC/bias off. GPSDO LBE-1421 0673ED0FA107 Output 1,
low drive, locked, 10 kHz above the requested carrier; Output 2/PPS off. Each
source has 20 dB attenuation before the combiner, then shared 40 dB into SDR.
No physical-chain changes.

40m: transmitter 7040100 Hz, reference 7050100 Hz, SDR center 7015100 Hz.
2m: transmitter 144490500 Hz, reference 144500500 Hz, SDR center 144465500 Hz.

Saved originals: `/Users/lbussy/GitHub/WsprryPi/dist/si5351-baseline-20260907`,
indexed by wspr4-baselines.json. Preserve those captures. The 2m formal campaign
is unqualified only at its uncalibrated absolute-offset gate; separate GPSDO
analysis and tone stability passed. Never rewrite an original failed gate.

Stage an isolated build on wspr4 without replacing its installed application.
Bind each test to source, executable hash, exact settings and raw IQ. Re-run
three two-second carrier bursts at both bands after every slice, and add bounded
four-tone transition and envelope captures where needed. Acquire a same-build
control before changing synthesis; the old installed binary differs from devel.
Capture the simultaneous GPSDO, verify lock and all outputs off afterward,
restore service state, and use bounded process lifetimes and cleanup ownership.

## Sequential slices

1. Add bounded, cancellable PLLA/initialization readiness checks while RF is
   inhibited. Evaluate an explicitly selected PLL-only transition when adjacent
   plans share the integer MultiSynth/R/control configuration. First tone and
   incompatible plans retain full inhibited programming/reset. Keep the fast
   path opt-in unless transition RF evidence supports promotion. Test errors,
   timeout, cancellation and exact writes; measure gaps, settling and phase.
2. Evaluate a common even integer MultiSynth/R-divider for a complete tone set,
   retuning only the fractional PLL. Reject a candidate when any tone violates
   VCO/divider/reference limits. Preserve the fixed-PLL default and existing
   unsupported-mode boundaries unless broader evidence justifies a change.
3. Implement bounded contiguous parameter-block I2C bursts with explicit ordering;
   keep reset/control semantics separate. Avoid unchanged stable control writes
   only when cache ownership is sound. Invalidate on uncertain writes/failures.
   Test short writes, boundaries, disabled caching and stop handling. No atomic
   update claim based solely on burst transfer.
4. Measure no fade versus current linear/raised-cosine duty chopping with the
   same physical output chain, including sidebands and edge timing. Correct any
   demonstrated software timing defect; do not claim analog shaping or add
   external amplitude hardware. A measured decision to retain an existing
   behavior is a valid result; record limitations clearly.
5. Compute ordinary reported frequency from the complete realized reference /
   PLL / MultiSynth / R chain. Test packed-register reconstruction, calibration
   signs, rational edge cases and tone spacing. Re-run both RF baselines; do not
   chase absolute frequency by recalibrating every run.

## Validation and adversarial assessment

Use the component's existing planner, fake-I2C transition, startup/cleanup tests
and appropriate parent integration coverage. Inspect target recipes before use.
No hardware-free test is RF evidence. For each slice review invalid inputs,
cache invalidation, interrupted/partial programming, output inhibition, mode
bounds, source identity and whether the RF capture actually exercises the change.
Repair actionable findings, re-run affected checks and repeat assessment before
moving on. Do not silently relax RF thresholds. Distinguish configured TONE
qualification from exploratory transition/noise/envelope analysis and from
absolute frequency calibration.

Experimental strategies belong to typed component configuration and explicit
maintainer-harness options, not new persisted/UI controls. Do not add user-facing
configuration surface merely to conduct a comparison. An optimization that lacks
sufficient RF evidence remains opt-in with its limitation documented.

## Deliverables and completion

Commit the prompt, code/tests, component documentation and review/results report.
Save raw IQ and exact replay/analysis scripts under ignored experiment artifacts;
keep compact source-bound result summaries and links in the report. Render 40m
and 2m comparisons after each of the five slices. Include a disposition for each
candidate, measured changes, adverse results, tests, unresolved limits, hardware
shutdown verification, branch/commit/push parity and Documentation Impact.
Operator documentation in the sibling repository is read-only unless separately
authorized; identify exact follow-up if final production behavior requires it.

Sources: https://www.qrp-labs.com/images/news/dayton2018/fdim2018.pdf (pp.14,
16–17,25–26,35); https://www.skyworksinc.com/-/media/Skyworks/SL/documents/public/application-notes/AN619.pdf.
