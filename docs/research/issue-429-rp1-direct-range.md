# Issue 429: RP1 direct-frequency range

RP1 TONE, WSPR, QRSS, FSKCW, and DFCW now use the numerical planner's shared
100 MHz direct-output ceiling instead of a separate 40 MHz application cap.
The nominal parent remains 200 MHz, source-rate PPM is applied once, and the
0.01 Hz numerical average-error tolerance is unchanged. All generated tones
must satisfy the ceiling and share one integer divider; integer-divider
boundary plans still fail closed.

This permits direct planning of 6 m as well as 12 m and the other lower bands.
It does not authorize RF output or qualify any band. RP1 6 m and 2 m remain
`untested` for every mode and require the existing experimental frequency
opt-in. Route-specific development authorization and provider lifecycle gates
are unchanged for both GPIO4 and GPIO20.

2 m remains numerically rejected by this direct-only backend. A third-harmonic
implementation would need explicit RF-to-clock frequency and modulation-spacing
conversion, correction ownership, filtering considerations, and separate
validation. No such conversion or automatic fallback is introduced here. The
absence of that implementation is not a measured failure of RP1 hardware.

The current Issue 429 conducted campaign measures intrinsic clock-frequency
accuracy against the GPSDO, with alternating reference and RP1 measurements.
It must explicitly include 12 m and retain band/route-specific results.
Spectral quality and mode qualification are outside this accuracy campaign;
they are not acceptance gates for its clock-frequency measurements. Software
tests with a fake provider do not establish measured frequency accuracy.

Operator documentation follow-up (separate repository, not modified here):
update `Wsprry_Pi_Docs/docs/Experimental/rp1_gpio.md` with the direct-range and
untested-band distinction once this implementation is integrated. Do not
replace qualification records with numerical planning results.

## Conducted diagnostic observations, 2026-08-30

The GPIO20 accuracy attempt used wspr5 and its SDRplay RSP1B `2404058C60`,
with alternating Leo Bodnar `0673ED0FA107` references controlled exclusively
through `/Users/lbussy/GitHub/lbgpsdo`. Both sources were disabled between
segments. SDR settings were 250 ksps, 200 kHz bandwidth, gain 20, AGC and bias
tee off. Manual transmitter correction was zero; Chrony was checked before
and after each completed bracket. These are diagnostic output-frequency
observations, **not intrinsic clock calibration values**:

| Band | Requested Hz | GPSDO-bracketed error ppm | Capture UTC identifier |
| --- | ---: | ---: | --- |
| 20 m | 14097100 | -47.46 | 20260830T195505Z |
| 12 m | 24926100 | -47.11 | 20260830T200127Z |
| 2200 m | 137500 | +202.96 | 20260830T200323Z |
| 630 m | 475700 | -3127.50 | 20260830T200415Z |

The evidence directories on wspr5 are
`/home/pi/wsprrypi-qualification-runs/issue429-rp1-accuracy-<UTC>-<Hz>`.
Local control logs are under `/private/tmp/issue429-rp1-accuracy-<UTC>-<Hz>`.
The 20 m binary SHA-256 was
`282c7fc363d487200d23dd5fc8b0875703151a28dca3eb59e05f2706dc54f7b3`;
the subsequent points used
`8c0e73b9429905b3424a3242c92adc0a720c155f36b03d3fb727c1ffebd19336`,
which additionally repairs explicit-tone experimental-policy propagation.
Neither executable replaced the installed application.

Three findings interrupted the planned sweep:

- An 8-second finite RP1 TONE failed immediately with provider `-EIO` and
  kernel `cleanup=0`. Two-second finite tones completed. The precise
  long-buffer failure remains unresolved; it is not a band disqualification.
- Explicit tone planning accepted experimental-band opt-in, but selector
  preparation dropped its policy flags. Copying both flags from the same
  accepted snapshot repairs this rejection. A scheduler-suppressed regression
  checks both default rejection and explicit opt-in for named/custom 12 m.
- During the repeat 630 m tone (`20260830T200820Z`), the live clock tree
  showed enabled `clk_gp0` beneath **50 MHz `xosc`**, not 200 MHz `pll_sys`.
  The deployed module source `4a51061e4494cdd1ce348674e078dd9f7c78a6c0`
  checks the parent before `clk_set_rate()`, but not afterward. Its fractional
  divider stream still assumes the 200 MHz plan. This runtime parent mismatch
  invalidates a single-parent intrinsic-clock calibration campaign.

At 475700 Hz, the 200 MHz plan has divider approximately 420.433046.
Using its fractional part with the 50 MHz parent's integer divider 105
predicts approximately 474234.615 Hz, or -3080.48 ppm before oscillator error.
This supports the parent-switch explanation but does not replace a reviewed
module repair and fresh measurements. The repeat capture's automatic interval
analysis was rejected; its clock-tree snapshot, not a new frequency estimate,
is the evidence for parent selection.

The 630 m frequency estimate above uses the unique reference/tone/reference
duration pattern and records an additional later burst outside that pattern.
FFT and phase-slope estimates were cross-checked; synthetic data recovered
a known -669.05 Hz bracketed offset within 0.002 Hz, including an extra
post-bracket burst. No receiver correction was applied a second time.

6 m, 4 m, and the remaining unsampled bands were not measured in this attempt;
2 m remains untested without harmonic support. No calibration constant or
mode qualification was promoted. The isolated process was stopped, RP1
output disabled, and both GPSDO outputs disabled before pausing for module
repair authorization.
