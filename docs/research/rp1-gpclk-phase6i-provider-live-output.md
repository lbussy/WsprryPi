# RP1 GPCLK Phase 6I: provider-owned minimum-power live output

## Outcome

Phase 6I passed its bounded provider-level live-output gate on `wspr5`. The
provider applied the requested minimum 2 mA drive state, selected GPCLK0 on
GPIO4 only while a program was active, produced four distinct divider tones,
and restored GPIO4 and GPCLK0 after normal completion, STOP/drain completion,
close during activity, module removal, and reload in fail-closed mode.

This is provider qualification for the Raspberry Pi OS 64-bit BCM2712 kernel
on Raspberry Pi 5, Pi 500, and CM5. It is not WSPR scheduler integration, a
complete WSPR transmission, calibrated power measurement, or product RF
qualification.

## Implementation

The provider now owns the complete output lifecycle:

- `live_output` is a read-only module parameter and defaults to false;
- probe resolves safe and 2, 4, 8, and 12 mA GPCLK0 pinctrl states and selects
  the safe GPIO input state;
- submission preloads the first fractional divider while the output is safe;
- the live gate selects the requested drive state and GPCLK0 function before
  enabling the clock through the active `clk-rp1` lease;
- completion and every cleanup path disable the leased clock before restoring
  the safe GPIO state; and
- module removal performs the same ordered cleanup.

The lease records its clock-enable state and permits clock prepare/enable or
disable/unprepare only for the lease owner performing the provider operation.
Unrelated common-clock rate or enable attempts still return `-EBUSY`. No RP1
register address is present in userspace, the UAPI, or device tree.

## Clock-disabled regression gate

Before energizing GPIO4, the rebuilt kernel ran with `live_output=N` and passed:

- both KUnit provider-contract cases;
- version, structure-size, drive, flags, tick, and generation rejection;
- single-owner enforcement;
- early, middle, and near-end STOP-to-drain completion;
- close-active deferred cleanup and reacquisition;
- repeat submission; and
- lease exclusion, with conflicting common-clock rate and enable operations
  returning `-EBUSY`.

GPIO4 remained an input and GPCLK0's prepare, enable, and protect counts were
zero throughout this gate.

## Minimum-power live-output results

The authorized harness was the existing GPIO4 radiator and an SDRplay RSP1B.
All live runs requested 2 mA, the lowest exposed RP1 pinctrl drive value. State
monitoring observed GPCLK0 on GPIO4 and a 2 mA drive setting only during active
output, with GPIO input and zero clock counts after each case.

The four-divider run completed generations 1 through 4. An uncalibrated SDR
analysis found a live peak near 14,096,464 Hz at approximately -63.8 dBFS
against a baseline near -143.7 dBFS, an on/off contrast of approximately
79.9 dB. Per-symbol phase estimates were 14,096,466.27, 14,096,467.83,
14,096,469.24, and 14,096,470.98 Hz. The relative steps were approximately
1.55, 1.41, and 1.74 Hz. These short-capture estimates demonstrate distinct
transitions and are not an absolute-frequency calibration.

Early, middle, and near-end STOP requests completed generations 5 through 7;
generation 8 then completed normally. The observed finite output intervals
were approximately 0.74 to 0.75 seconds, consistent with provider drain to the
end of the submitted descriptor rather than an immediate output cut. Closing
generation 9 while active also drained and cleaned up; generation 10 then
reacquired and completed, demonstrating repeatability.

## Cleanup, installed state, and rollback

After the live cases, provider removal left GPIO4 as input and GPCLK0 fully
disabled. The provider was then reloaded with its default `live_output=N`, and
the SoapyRemote service was restored to active. Final observed state was:

- kernel `6.18.44-v8-16k+ #3`;
- GPIO4 input;
- GPCLK0 prepare, enable, and protect counts all zero;
- provider `live_output=N`; and
- `soapyremote-server.service` active.

The side-by-side test image remains selected through the existing one-shot
`tryboot.txt` arrangement. Normal `config.txt` was not modified. An ordinary
reboot remains the rollback to the packaged kernel path; that rollback was not
exercised during this phase.

## Remaining gate

The current provider accepts one finite divider program at a time. The test
harness deliberately left gaps between its four descriptors, so this result
does not establish the continuous 162-symbol cadence required by WSPR. Before
scheduler integration, the provider/UAPI must support a complete uninterrupted
symbol sequence, or an equivalent demonstrably gap-free queue, while preserving
STOP/drain and fail-safe cleanup semantics.

Phase 6J should implement and test that continuous multi-symbol contract in
clock-disabled mode first. It should then connect the existing RP1 Linux
provider backend to WsprryPi's scheduler, expose the operator drive selection
with a 2 mA default, and keep live full-frame/RF qualification as a separate
authorized gate.

Exact compact logs, hashes, timings, and relative SDR observations are in
[`rp1-gpclk-phase6i-evidence/summary.txt`](rp1-gpclk-phase6i-evidence/summary.txt)
and the adjacent raw log files.
