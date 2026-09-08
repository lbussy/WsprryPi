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
RF disposition remains pending until the new same-path captures are analyzed.
