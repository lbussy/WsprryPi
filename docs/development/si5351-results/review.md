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


## Step 3: bounded parameter bursts and control-cache experiment

Contiguous parameter writes are grouped within one PLL/MultiSynth block only.
Reset and output-enable operations are always sent. A partial write fails without
retry and invalidates all cached state; every device error and reopen invalidates
cache. The opt-in assumes the harness owns the device without another writer.
The backend observes stop requests between bus transactions, including before
re-enable; a physical I2C transfer cannot be made atomic by this optimization.

Adversarial repair: initial runtime grouping allocated a vector for each single
write. Single writes now retain the direct call. Drive bits are merged during
plan preparation, repairing their previous overwrite while avoiding runtime
plan copying. Tests verify all four requested drive settings before RF enable,
block boundaries, short writes, no retry, command writes, cache invalidation,
mid-burst stop/failure and cleanup. Legacy paths and optimized paths are tested.
Programming-duration debug logging covers the complete tone application,
including readiness, not just wire time.


Live adversarial finding: `step3-40m` completed carrier capture but rejected a
later active PLL update at the readiness gate. Bus failures would have retained
a device error; the generic error implicates the non-ready status path, but the
exact register value was not logged in this attempt. Both RF sources were
verified off and the installed service/identities restored. The attempt remains
rejected, not averaged into successful runs. Bursts are now restricted to RF-off
or explicitly inhibited programming, with individual transactions retained while
RF is active. Added active-transaction assertions and explicit status logging.
This closes the unsafe combination conservatively; it does not establish atomic
or uninterrupted active burst tuning.


## Step 4: fade timing and measured spectrum

The wide-channel baseline edge view shows full-amplitude chopping, and the
narrow-threshold 2 m on interval is about 531 ms for a nominal 500 ms key,
versus about 502 ms without fading. The requested 20 ms fades are stretched by
I2C and relative waits. Slice deadlines now use the original event schedule;
expired pulses are skipped and a requested fade-out ends disabled even if its
last slice was missed. None/linear/raised-cosine semantics remain duty-cycle
shaping. No analog envelope or external hardware has been introduced.

Adversarial checks use output transactions deliberately slower than a slice and
cancel after enable. These verify bounded event duration and no subsequent
re-enable. Review also repaired readiness selection for an RF-off compatible PLL
retune: it may wait while inhibited, whereas active RF still fails immediately.
The initial off-retune fixture had only two unique WSPR tones and correctly failed
configuration; corrected it to the required complete four-tone set.

## Step 5: complete realized frequency chain

Ordinary frequency reporting now uses the programmed PLL ratio as well as the
MultiSynth/R-divider, including calibration of the reference. Tests independently
decode the packed bytes over both calibration signs, low-frequency R-divider
cases, HF and 2 m. Reviewed 40 m/2 m spacing remains checked separately. A fixture
asserts that it actually exposes a nonzero omitted PLL residual. This corrects
reporting, not the transmitted register image or reference calibration.

An initial test incorrectly imposed the reviewed-band spacing limit on arbitrary
100 MHz fixed-PLL requests. Complete-chain reconstruction remains checked there;
spacing acceptance is tied to the actual reviewed bands. No claim of WSPR
qualification at 100 MHz is made. The old planner is separately compiled against
the new tests to confirm that reconstruction assertions detect the original bug.

Final RF runs return to ordinary fixed-PLL planning on 40 m so this correction
is exercised. The 2 m divide-by-6 PLL experiment remains selected. This deliberate
strategy difference is recorded; step 5 versus step 4 is not a single-variable
40 m synthesis comparison. Carrier register programming for step 5 matches the
ordinary reference strategy, with the tiny reporting residual checked in software.
