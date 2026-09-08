# Si5351 adversarial review log

## Orientation and measurement setup

- Source base: a523904; bounded harness setup: 36d69f8. Existing installed RF
  baseline binary differs from devel; acquired fresh 40m/2m controls before
  modifying synthesis. Original carrier baselines remain immutable.
- Initial SDR helper invocation used numeric booleans; rejected before Si5351
  transmission. Corrected to `false`, retained failed attempt, verified cleanup.
- Repaired analysis to include transition/key edges in their spectra rather
  than selecting only steady interiors. Added exact receiver-setting and
  GPSDO locked-interval coverage checks. No changed qualification thresholds.
- Baseline Linux component planner, fake-I2C transitions, startup qualification
  tests passed. Local planner passed; local fake-I2C build is unsupported because
  Linux I2C headers are absent, so those tests ran on wspr4 without hardware I/O.

## Step 1: PLL readiness and optional compatible PLL-only changes

Implementation: bounded cancellable readiness checks for SYS_INIT, LOL_A and
LOS_XTAL. Default full inhibited transitions preserved. Explicit maintainer
PLL-only option requires matching integer MultiSynth register image and R-divider;
startup and first tone still receive full programming/reset.

Adversarial findings repaired:

- Active-output lock loss must fail immediately, not wait for relock while RF
  remains active. Active paths now disable and fail; initially inhibited paths
  retain bounded readiness polling.
- Recheck cancellation after status reads and before re-enable.

Tests include full 162-symbol default reset ordering, first-only reset in the
opt-in path, bounded unlocked startup, readiness interruption, failed PLL writes,
lock loss during fast retuning, and output-disable cleanup. Planner, transition
and startup qualification tests passed; affected tests were rerun after repairs.
Both bands captured all four cases with shutdown verified. The 2 m transition
trace loses the repeated deep notches with PLL-only tuning; retain it as an
explicit experiment, not a production default. The 40 m default strategy is
unchanged. Carrier drift is reported separately, with fixed PPM throughout.

## Step 2: common even integer MultiSynth experiment

A typed opt-in planner setting selects a common even MultiSynth and R-divider
for the complete TONE/WSPR set, with PLL limits checked for every tone. Other
modes are not broadened. Default parked-PLL planning remains available.

Adversarial assessment covered incompatible tone sets, unsupported modes,
nonfinite/negative/zero frequencies, small frequencies needing R division,
packed divider equality, VCO limits and realized frequency error. Tests cover
these cases. The common-divider search fails closed when no valid choice exists.

Measurement review replaced box averaging with a 300 Hz passband / 450 Hz stopband
channel filter before 1 kHz decimation to reject GPSDO leakage. All captures are
reprocessed identically. The 5 ms envelope smoother limits gap resolution; phase
steps are exploratory extrapolations, not calibrated phase-noise measurements.
Capture cleanup now attempts every device/service restoration independently and
records errors even if an earlier cleanup fails.
