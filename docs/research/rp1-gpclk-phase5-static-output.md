# RP1 GPCLK Phase 5 bounded static-output probe

## Disposition

**Failed the bounded-duration gate.**

The first kernel-owned RP1 GPCLK0 output on GPIO4 produced a strong, stable
signal in the attached SDR capture and cleaned up to the required clock and
pin state. The kernel delayed-work watchdog disabled the clock 5.247271
seconds after enablement, however, exceeding the authorized five-second
maximum by about 247 ms. No second live-output attempt was made.

The uncalibrated SDR measurement also placed the strongest captured feature
at 14,096,512 Hz, 586 Hz below the 14,097,098 Hz common-clock readback. That
disagreement is unresolved and does not qualify absolute frequency placement.

## Scope and physical arrangement

The probe ran on 2026-08-10 on `wspr5`. The user confirmed that GPIO4 was
connected to a shielded, 50-ohm terminated and attenuated SDR measurement
chain with no antenna or unshielded radiator path. The numeric attenuation
was not supplied, so amplitude is reported in dBFS rather than RF power.

The authorized live output was one static GPCLK0 interval at a requested
14,097,100 Hz, the lowest configured 2 mA drive, and no more than five
seconds. No WSPR symbols, divider transitions while enabled, WSPR frame,
service change, installation, reboot, persistent overlay, `/dev/mem` access,
or direct register write was authorized or performed.

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

## Supported conclusions

This probe establishes only that the tested kernel-owned design can:

- acquire RP1 GPCLK0 and GPIO4 through Linux resource APIs;
- select GPCLK0 on GPIO4 at the configured 2 mA drive;
- enable and disable the clock;
- produce a strong static feature in the attached SDR;
- return GPIO4 to an input-safe state; and
- restore the clock rate, parent, and ownership state.

It does not establish compliance with the five-second bound, absolute
frequency placement, RF power, spectral purity, divider-transition
atomicity, modulation timing, WSPR tone spacing, WSPR decode performance,
2 m support, or production readiness.

## Required work before another live-output phase

1. Replace delayed work on a general workqueue with a hard maximum-duration
   mechanism whose worst-case disable latency is demonstrated below the
   authorized bound, such as an hrtimer-triggered fail-closed path with
   measured scheduling margin.
2. Test the revised watchdog without output, including CPU-load and
   cancellation cases.
3. Define and record the SDR reference calibration or use a calibrated
   frequency counter/spectrum analyzer to resolve the 586 Hz discrepancy.
4. Record the numeric attenuation and conducted RF chain so power can be
   reported safely.
5. Obtain separate authorization before another GPIO4 output interval.

Phase 6 divider-transition testing was not started.
