# Issue 369: 70 cm WSPR by third-harmonic generation

## Disposition

**Failed qualification — will not fix.**

The tested Si5351 third-harmonic design did not produce a decodable 70 cm
WSPR frame. WsprryPi will not implement or support this approach. The
experimental source changes are not release candidates and should not be
committed as a feature.

Other architectures, such as a conventional lower-frequency Si5351 WSPR
exciter followed by an external mixer and stable 400 MHz local oscillator,
would constitute different hardware and a separate feature proposal.

## Outcome

Third-harmonic WSPR is mathematically and digitally feasible, but the tested
Si5351 system did not pass RF qualification. WsprryPi generated complete WSPR
frames from a 144.100500 MHz fundamental and produced the expected signal near
432.301500 MHz. None of three independently captured frames decoded, however.
The captured four-tone set had the correct 1.46484375 Hz radiated spacing but
walked several hertz during each frame. Therefore 70 cm remains unqualified.

## Frequency plan

The explicit harmonic multiplier is 3:

- Radiated WSPR center: 432,301,500 Hz
- Generated fundamental center: 144,100,500 Hz
- Radiated tone spacing: 1.46484375 Hz
- Generated tone spacing: 0.48828125 Hz

| Tone | Requested fundamental (Hz) | Si5351 planned fundamental (Hz) | Fundamental error (Hz) | Radiated error (Hz) |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 144,100,499.267578125 | 144,100,499.267589688 | +0.000011563 | +0.000034690 |
| 1 | 144,100,499.755859375 | 144,100,499.755852371 | -0.000007004 | -0.000020981 |
| 2 | 144,100,500.244140625 | 144,100,500.244159549 | +0.000018924 | +0.000056744 |
| 3 | 144,100,500.732421875 | 144,100,500.732417196 | -0.000004679 | -0.000014067 |

These Si5351 values use the qualification configuration: a nominal 27 MHz
reference and `2.353615654` PPM correction. The largest planned radiated error
is about 56.7 microhertz, negligible relative to WSPR tone spacing.

## Si5351 plan and device limits

The existing 850 MHz parked-PLL plan cannot synthesize 144.100500 MHz because
it would require a multisynth divider below 6. A fixed 900 MHz PLL does not
solve the problem: 900 / 144.1005 is not a legal integer multisynth ratio, and
fractional multisynth operation at this range is outside the intended plan.

The safe plan reuses the qualified 2 m strategy:

1. Use an integer multisynth divider of 6.
2. Retune PLLA per tone to approximately 864.603 MHz around the four
   requested tones.
3. Inhibit the selected clock output for the complete PLL/multisynth write and
   PLL reset.
4. Re-enable the output only after the programmed state is complete.
5. Disable all clock outputs during cleanup.

The resulting PLL frequencies are approximately 864.603 MHz, inside the
Si5351 planner's 600-900 MHz PLL range. The integer multisynth divider is at
its legal minimum of 6. Dry-run planning accepted all four tones.

## Effect on existing bands

The parked PLL remains 850 MHz. Existing supported bands retain their current
planner, calibration semantics, register programming, and tone switching. The
alternate PLL plan is selected only when the parked plan cannot represent the
requested tone and the integer divide-by-6 candidate is legal.

Calibration continues to modify the effective reference frequency before the
fractional PLL ratio is calculated. The per-tone plan necessarily resets PLLA
and cannot promise phase continuity across a tone change. Output inhibition
prevents intermediate register states from being radiated, but phase noise,
settling, switching transients, and third-harmonic amplitude remain RF
qualification questions. The successful 2 m result does not establish those
properties at the third harmonic.

## Software prototype

The research implementation adds an explicit `harmonic_multiplier` to the
legacy request, typed tone/WSPR payloads, and compiled execution plan. For the
canonical 70 cm WSPR band, the scheduler selects multiplier 3. The execution
compiler divides both radiated center frequency and WSPR tone spacing by the
multiplier while preserving the requested radiated frequency as metadata.
Backend frequency adjustments are scaled back into radiated-frequency
metadata. Tone and full-frame routes both propagate the multiplier.

The multiplier is deliberately restricted to 1 or 3. It is explicit execution
metadata rather than an inference made by a hardware backend. Existing bands
use 1 and are unchanged. Because RF qualification failed, this prototype must
not be treated as supported production behavior without a fail-closed gate or
additional successful qualification work.

## Qualification evidence

Qualification used `wspr5`, Raspberry Pi 5, the configured Si5351 at I2C bus 1
address `0x60`, CLK0, 27 MHz reference, minimum 2 mA drive, a shielded 50-ohm
load, 30 dB series attenuation, and the local RSP1B receiver at 250 ksps with
fixed 25 dB gain. The managed transmitter and SoapyRemote services were stopped
during each bounded test and restored afterward.

- Four-tone Si5351 dry run: passed.
- Continuous 70 cm carrier gate: passed. The measured peak was
  432,301,548.399 Hz and 92.61 dB above the capture median.
- Three complete WSPR transmissions: completed in 110.649491,
  110.600233, and 110.600237 seconds.
- Continuous 370-second IQ capture: 92,500,000 samples, zero overflows.
- Independent `wsprd` result: 0 of 3 decoded, including deep wideband passes.
- Captured tone spacing: correct at approximately 1.46484375 Hz.
- Captured frequency stability: the tone set moved several hertz during a
  frame, despite microhertz-scale static planner error.
- Cleanup: transmitter and capture returned success, Si5351 register 3 was
  `0xff`, and both normal services were active.

The carrier gate demonstrates that a third harmonic exists. It does not prove
a decodable WSPR waveform or acceptable spectral compliance.

## Filtering and further qualification

A supported station would require a 70 cm band-pass or high-pass/low-pass
network that selects 432 MHz while strongly rejecting the 144 MHz fundamental,
the 288 MHz second harmonic, and higher unwanted products. Qualification must
measure conducted power, fundamental rejection, harmonic and spur levels,
occupied bandwidth, tone spacing, switching transients, frequency drift, and
thermal behavior with representative filtering and load conditions.

Before reconsidering support:

1. Identify and correct the several-hertz within-frame drift at 432 MHz. Check
   reference warm-up, TCXO behavior, calibration method, PLL settling, supply
   stability, and receiver-reference error separately.
2. Repeat a continuous-carrier stability gate after thermal equilibrium.
3. Capture and independently decode at least three consecutive WSPR frames.
4. Repeat at the intended drive levels and with the production 70 cm filter.
5. Measure the 144 MHz fundamental and all relevant spurious products with a
   calibrated spectrum analyzer or equivalent conducted setup.
6. Document the exact qualified Si5351 board/reference, Pi model, filter, and
   RF chain. Do not generalize one assembly's result to every breakout board.

## Relationship to the 2 m crash

Shared findings:

- Planner inputs must be validated before hardware activation.
- Unsupported requests must fail cleanly.
- The integer multisynth divide-by-6, per-tone PLL-retune path is reusable.
- Output inhibition and cleanup around PLL changes are required.
- Backend-specific qualification cannot be inferred from another backend.

70 cm-only findings:

- Fundamental frequency and tone spacing must both be divided by 3.
- Radiated metadata must remain distinct from the generated fundamental.
- Third-harmonic selection filtering is mandatory.
- Drift, phase behavior, and spurs must be assessed at 432 MHz.
- A valid 144 MHz plan and a detectable 432 MHz carrier do not establish a
  decodable 70 cm WSPR frame.

The 2 m crash remains a fail-cleanly defect. Intentional 70 cm harmonic
operation is a separate feature and qualification problem.
