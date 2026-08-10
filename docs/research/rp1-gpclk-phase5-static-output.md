# RP1 GPCLK Phase 5 bounded static-output probe

## Disposition

**Passed on the authorized retry.**

The first kernel-owned RP1 GPCLK0 output on GPIO4 produced a strong, stable
signal in the attached SDR capture and cleaned up to the required clock and
pin state. The kernel delayed-work watchdog disabled the clock 5.247271
seconds after enablement, however, exceeding the authorized five-second
maximum by about 247 ms.

The retry replaced delayed work with an absolute high-resolution timer whose
callback disables the prepared clock through the atomic clock API. Twenty
clock-only trials under four-core CPU load showed 0--1 us timer lateness. The
live retry then disabled output after 3.999999 seconds with 1 us measured timer
lateness and restored every clock and pin state.

The retry again measured the carrier near 14,096,513 Hz. The common-clock
readback is a nominal divider result, not an independent frequency
measurement. A preceding RSP1B/Si5351 reference capture differed by only
+2.29 Hz at 24.9261 MHz, while both RP1 captures were about -41.6 PPM from the
nominal result. The repeatability resolves the apparent 586 Hz readback
disagreement as predominantly reference error in the RP1 50 MHz `xosc` path;
it is not evidence of a divider-planning error. No calibrated power conclusion
is made.

## Scope and physical arrangement

The probe ran on 2026-08-10 on `wspr5`. The user confirmed that GPIO4 was
connected to a shielded, 50-ohm terminated and attenuated SDR measurement
chain with no antenna or unshielded radiator path. The numeric attenuation
was not supplied, so amplitude is reported in dBFS rather than RF power.

Each authorized live output was one static GPCLK0 interval at a requested
14,097,100 Hz, the lowest configured 2 mA drive, and no more than five
seconds. No WSPR symbols, divider transitions while enabled, WSPR frame,
installation, reboot, persistent overlay, `/dev/mem` access, or direct register
write was performed. The retry temporarily stopped the local
`soapyremote-server.service` while the capture process owned the RSP1B and
restored it afterward.

## System and source revisions

- board: Raspberry Pi 5 Model B Rev 1.0, revision `c04170`;
- operating system: Debian GNU/Linux 13.6, `aarch64`;
- kernel: `6.18.34+rpt-rpi-2712`;
- initial and final kernel taint: 4096 from the earlier out-of-tree probe;
- RP1 clock: `clk_gp0`, clock ID 33;
- output pin: GPIO4 on `gpiochip0`;
- WsprryPi: `0a5d1de5445b4941be141b000a5b4967ab96ca0e`; and
- WSPR-Transmitter:
  `6da56219d33a46a45984b93db99d8eb187d898b6`.

## Temporary probe design

The temporary, out-of-tree GPL platform driver was bound through a runtime
device-tree overlay. The overlay supplied:

- `clk_gp0` through the RP1 common-clock provider;
- a GPIO4 input-safe pinctrl state with pull-up and 2 mA drive; and
- a GPIO4 `gpclk0` pinctrl state with bias disabled and 2 mA drive.

The driver acquired the clock and pinctrl resources through kernel APIs,
selected the safe state, obtained exclusive rate protection, configured the
disabled clock, armed delayed work, selected the GPCLK pin state, and then
called `clk_prepare_enable()`. Cleanup disabled and unprepared the clock
before selecting the safe pin state, restoring 50 MHz, and releasing rate
protection.

The delayed-work watchdog was armed before output activation and was
independent of SSH and the SDR capture process. Runtime overlay removal and
module unload provided the explicit secondary stop path. The probe did not
use `/dev/mem`, `/dev/gpiomem`, writable debugfs, `pinctrl set`, or persistent
device-tree configuration.

## Pre-output validation

The module and overlay compiled successfully against the running kernel. The
module compiled with the kernel build's warning policy, and the SDR capture
program compiled as C++20 with `-Wall -Wextra -Werror`.

Dry-run validation never selected GPCLK or enabled the clock. It covered:

- normal acquisition, rate configuration, and restoration;
- injected failure after safe-pin selection;
- injected failure after exclusive clock acquisition;
- injected failure after disabled-clock rate configuration;
- explicit cancellation by runtime-overlay removal; and
- independent 100 ms kernel-watchdog expiry.

Every dry-run path finished with:

- `clk_gp0` at 50,000,000 Hz from `xosc`;
- enable count 0;
- prepare count 0;
- rate-protection count 0; and
- GPIO4 input, pull-up, and unclaimed.

The explicit-cancellation dry run held the disabled clock at 14,097,098 Hz
with protection count 1 before overlay removal restored the final state. The
100 ms dry-run watchdog independently restored the same final state.

## SDR capture configuration

- receiver: SDRplay RSP1B serial `2404058C60`;
- driver: SoapySDR `sdrplay`;
- sample format: CF32;
- sample rate: 250,000 samples/s;
- capture duration: 8 seconds;
- samples: 2,000,000;
- overflows: 0;
- receiver center: 14,122,100 Hz;
- bandwidth: 200,000 Hz;
- AGC: disabled;
- gain: 25 dB; and
- retained capture size: 16,000,000 bytes.

The capture began before the output probe was loaded. Its SHA-256 digest is:

```text
daf0c7e9b49803d39d9b8ac513884857fcf05dbc18073f87f494d8633ad6378d
```

The capture log SHA-256 digest is:

```text
e773870f82aabf40326cdd11deb588115e1d8e4df2af5509c786cf5101cf176b
```

## Live-output evidence

The supervisor armed output at `2026-08-10T07:45:32.551569854-05:00`. The
runtime overlay returned at `2026-08-10T07:45:32.575550991-05:00` after the
driver enabled output.

During output, readback reported:

- requested rate: 14,097,100 Hz;
- rounded and observed common-clock rate: 14,097,098 Hz;
- parent: `xosc`;
- enable count: 1;
- prepare count: 1;
- rate-protection count: 1;
- GPIO4 function: `GPCLK0`;
- GPIO4 drive strength: 2 mA;
- GPIO4 bias: disabled; and
- pinctrl output enable: active.

Kernel monotonic log timestamps were:

```text
179197.749756 output enabled
179202.997027 kernel watchdog expired
```

The interval was therefore 5.247271 seconds. Cleanup immediately logged clock
disable, safe-pin selection result 0, and successful restoration to 50 MHz.

Quarter-second SDR FFT windows showed a stable feature at approximately
14,096,512 Hz from capture time 0.50 seconds through 5.75 seconds. Full-on
windows measured approximately -48.7 dBFS, about 63.4 dB above the spectral
median. The before/after windows did not contain a comparable feature.

The measured frequency differs from common-clock readback by about -586 Hz,
or -41.6 PPM. No receiver-reference calibration or numeric RF-chain
attenuation was supplied, so the result proves the presence and bounded
timing of a strong signal but not absolute frequency or conducted power.

## Final state and cleanup

After the watchdog, runtime-overlay removal, and module unload:

- temporary module: unloaded;
- runtime overlays: none;
- `clk_gp0` rate: 50,000,000 Hz;
- parent: `xosc`;
- enable count: 0;
- prepare count: 0;
- rate-protection count: 0;
- GPIO4: input, pull-up, high, and unclaimed;
- SDR capture: completed normally with zero overflows; and
- temporary probe and capture processes: none.

The kernel taint value remained 4096. No reboot was authorized or performed.

## Authorized retry

The retry used the same Pi, GPIO4 output, RSP1B, sample rate, receiver center,
bandwidth, fixed receiver gain, and relative-only measurement policy. Power
was not calibrated. WsprryPi was at
`d8e1b38570513f8954820d7a25f4bc8fe2743aff`; WSPR-Transmitter remained at
`6da56219d33a46a45984b93db99d8eb187d898b6`. The revised probe used separate
`clk_prepare()` and
`clk_enable()` calls so the high-resolution timer callback could invoke the
non-sleeping `clk_disable()` path. Deferred work then unprepared the clock,
selected the input-safe pin state, restored the original rate, and released
exclusive ownership.

Before live output, 20 clock-only 100 ms trials ran while four SHA-256 workers
saturated the four CPU cores. GPIO4 remained input/pull-up. All 20 trials
reported 99.998--100.000 ms active time and 0--1 us deadline lateness. A
separate cancellation test removed the overlay about 100 ms into a four-second
clock-only interval; no later timer callback occurred and cleanup completed.

The live retry reported:

```text
enabled_ns=180971067770091
deadline_ns=180975067767517
cutoff_ns=180975067769173
active_us=3999999
deadline_late_us=1
```

The 8-second CF32 capture contained 2,000,000 samples and zero overflows.
Averaged on/off spectra placed the transmitter-added feature at approximately
14,096,513 Hz, -587 Hz relative to the 14,097,100 Hz request, with 64.99 dB
on/off contrast and 64.59 dB above the analyzed-band median. Half-second
windows during output ranged from approximately -54.78 to -54.58 dBFS. These
are relative receiver measurements, not RF power readings.

After the timer cutoff, overlay removal, and module unload:

- runtime overlays: none;
- GPIO4: input, pull-up, high;
- `clk_gp0`: 50,000,000 Hz from `xosc`;
- enable, prepare, and rate-protection counts: 0; and
- `soapyremote-server.service`: active.

Retry evidence is retained on `wspr5` in
`/home/pi/rp1-phase5-retry-validation/`. Key SHA-256 digests are:

```text
2531097b4445591a86d62f9b45210eb3cc26d2738a45e6b4d2adc7ec5add0b06  rp1-phase5-retry-capture.cf32
b87ea125c80c41df585a58939849dcad01eeffb7970d03c0ea09c06c71790ecb  rp1-phase5-retry-kernel.log
494ddd746dc83888ef52a44fbdb19e7d3442dd487f30a032cfec9058709a72c2  rp1_phase5_probe.c
```

## Supported conclusions

These probes establish only that the tested kernel-owned design can:

- acquire RP1 GPCLK0 and GPIO4 through Linux resource APIs;
- select GPCLK0 on GPIO4 at the configured 2 mA drive;
- enable and disable the clock;
- produce a strong static feature in the attached SDR;
- return GPIO4 to an input-safe state; and
- restore the clock rate, parent, and ownership state; and
- enforce the tested static-output interval with the revised high-resolution
  cutoff under the measured conditions.

It does not establish calibrated absolute frequency placement, RF power,
spectral purity, divider-transition
atomicity, modulation timing, WSPR tone spacing, WSPR decode performance,
2 m support, or production readiness.

## Required work before Phase 6

1. Promote the demonstrated high-resolution cutoff and lifecycle into a
   maintainable RP1 resource adapter rather than the temporary probe.
2. Define the four-divider transition mechanism and prove fail-closed
   cancellation before enabling transitions.
3. Keep frequency and amplitude findings relative unless a later test
   explicitly requires calibrated instrumentation.
4. Preserve 2 mA as the safe default; a later backend phase must expose RP1's
   supported 2, 4, 8, and 12 mA drive-strength choices.

Phase 6 clock-only divider-transition testing subsequently started but failed
its timing gate. See `rp1-gpclk-phase6-clock-only-transitions.md`. No live
divider transition was attempted.
