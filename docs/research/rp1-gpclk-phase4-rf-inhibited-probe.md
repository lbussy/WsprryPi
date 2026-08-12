# RP1 GPCLK Phase 4 RF-inhibited probe

## Disposition

Phase 4 passed for RF-inhibited RP1 resource ownership, disabled-clock rate
control, GPIO isolation, and explicit cleanup on the tested Raspberry Pi 5.

This result does not qualify GPCLK output or RF transmission. Immediate
automatic cancellation cleanup remains a Phase 5 precondition because the
first shell-level cancellation harness deferred cleanup while waiting on a
foreground process.

## Scope and authorization

The probe ran on 2026-08-10 with the radiator physically disconnected from
GPIO4. Nothing remained connected to GPIO4: no radiator, antenna, amplifier,
filter, transmitter, load, or measurement instrument.

The authorized scope was limited to an RF-inhibited inspection of RP1 clock
and pin ownership. No intentional carrier, WSPR tone sequence, WSPR frame,
service change, installation, reboot, persistent boot change, device-tree
change, `/dev/mem` access, or direct register write was performed. Phase 5 was
not started.

## System under test

- host: `wspr5`;
- board: Raspberry Pi 5 Model B Rev 1.0;
- revision: `c04170`;
- architecture: `aarch64`;
- operating system: Debian GNU/Linux 13.6 (trixie);
- kernel: `6.18.34+rpt-rpi-2712`;
- RP1 clock driver: `rp1-clk`;
- clock provider compatible string: `raspberrypi,rp1-clocks`;
- candidate clock: `clk_gp0`, RP1 clock ID 33; and
- output candidate: GPIO4 on `gpiochip0`.

The tested source revisions were:

- WsprryPi: `3b070c2f60d252d473dffcfae6ddad0da795c9a2`; and
- WSPR-Transmitter:
  `6da56219d33a46a45984b93db99d8eb187d898b6`.

## Initial state

Read-only common-clock, GPIO, and pinctrl inspection reported:

- `clk_gp0` rate: 50,000,000 Hz;
- parent: `xosc`;
- enable count: 0;
- prepare count: 0;
- exclusive rate-protection count: 0;
- GPIO4: unclaimed through `gpioinfo`;
- GPIO4: input through `gpioinfo`;
- GPIO4: not muxed to GPCLK; and
- pinctrl function: `none`, with pull-up.

The relevant initial inspections were equivalent to:

```sh
gpioinfo --chip gpiochip0 4
pinctrl get 4
grep -E "clk_gp0 " /sys/kernel/debug/clk/clk_summary
cat /sys/kernel/debug/clk/clk_gp0/clk_rate
cat /sys/kernel/debug/clk/clk_gp0/clk_enable_count
cat /sys/kernel/debug/clk/clk_gp0/clk_prepare_count
cat /sys/kernel/debug/clk/clk_gp0/clk_parent
cat /sys/kernel/debug/clk/clk_gp0/clk_protect_count
```

The common-clock debug files used for observation were read-only. They were
not used as an unsupported rate-control interface.

## GPIO4 ownership probe

GPIO4 was requested through libgpiod 2.2.1 using the kernel GPIO character
device. The first process used an as-is request and the consumer name
`rp1-phase4-owner`:

```sh
gpioget --as-is --chip gpiochip0 \
  --consumer rp1-phase4-owner --hold-period 5s --numeric 4
```

While that request was held, `gpioinfo --chip gpiochip0 4` reported
`consumer="rp1-phase4-owner"`. A competing as-is request using consumer
`rp1-phase4-contender` failed with exit status 1 and:

```text
gpioget: unable to request lines: Device or resource busy
```

The first request returned value 0 and exit status 0. After it exited,
`gpioinfo` showed GPIO4 unclaimed. GPIO4 was never selected as GPCLK or as an
output.

### GPIO restoration caveat

Despite the as-is option, the libgpiod request left GPIO4 explicitly muxed as
an input after release. The initial pinctrl function was `none`; the final
function was `input`. The pull-up remained present.

The `pinctrl set` debug command was deliberately not used to restore `none`
because that tool warns that it writes GPIO control registers directly while
bypassing Linux driver ownership. The final input/pull-up state was safe for
an isolated pin but was not bit-for-bit identical to the initial state.

## Disabled-clock ownership and rate probe

A temporary out-of-tree GPL kernel module was built under `/tmp`, loaded only
for bounded probes, unloaded after each probe, and then removed. It obtained
RP1 clock ID 33 through the kernel common-clock framework and used:

- `of_clk_get_from_provider()`;
- `clk_rate_exclusive_get()`;
- `clk_round_rate()`;
- `clk_set_rate()`;
- `clk_rate_exclusive_put()`; and
- `clk_put()`.

The module did not call a clock-enable, clock-prepare, GPIO, or pinmux API.
Before each load, the shell required rate 50 MHz and zero enable and prepare
counts. During each load, read-only debug state had to retain zero enable and
prepare counts and GPIO4 had to remain input. Module unload restored the
original 50 MHz rate before releasing exclusive ownership.

The bounded operations were equivalent to:

```sh
sudo /sbin/insmod /tmp/rp1-phase4-probe/rp1_phase4_probe.ko \
  target_rate=<requested-rate>
cat /sys/kernel/debug/clk/clk_gp0/clk_rate
cat /sys/kernel/debug/clk/clk_gp0/clk_enable_count
cat /sys/kernel/debug/clk/clk_gp0/clk_prepare_count
cat /sys/kernel/debug/clk/clk_gp0/clk_parent
cat /sys/kernel/debug/clk/clk_gp0/clk_protect_count
pinctrl get 4
gpioinfo --chip gpiochip0 4
sudo /sbin/rmmod rp1_phase4_probe
```

Observed results were:

| Requested rate | Observed rate | Parent |
| ---: | ---: | --- |
| 475,700 Hz | 475,700 Hz | `xosc` |
| 10,140,200 Hz | 10,140,198 Hz | `pll_sys` |
| 14,097,100 Hz | 14,097,098 Hz | `xosc` |
| 70,092,500 Hz | 70,092,353 Hz | `pll_sys` |

At every point:

- `clk_gp0` enable count remained 0;
- prepare count remained 0;
- exclusive rate-protection count was 1 while the module held the clock;
- GPIO4 remained input and disconnected from GPCLK;
- unloading restored 50 MHz from `xosc`; and
- the exclusive rate-protection count returned to 0.

Kernel messages confirmed each requested and observed rate and reported a
successful 50 MHz restoration with result 0.

## Cancellation and cleanup incident

The first shell-level cancellation exercise loaded the module for a bounded
10,140,200 Hz probe, installed an `EXIT HUP INT TERM` cleanup trap, and then
waited synchronously on `sleep 30`. Sending `TERM` to that shell did not invoke
the cleanup immediately: Bash deferred the trap while it waited on the
foreground process.

The residual state was detected immediately:

- the temporary module remained loaded;
- the observed clock rate was 10,140,198 Hz from `pll_sys`;
- rate-protection count remained 1;
- enable and prepare counts remained 0; and
- GPIO4 remained input.

Fail-closed cleanup then explicitly unloaded `rp1_phase4_probe` before
terminating only the identified temporary shell and sleep processes. Module
unload restored 50 MHz from `xosc` with result 0. Final inspection found no
loaded probe module, retained clock protection, or temporary probe process.

This incident qualifies the explicit module-unload restoration path, not the
shell trap. A later hardware harness must use a supervisor or signal design
that invokes cleanup immediately; it must not rely on a deferred trap around
a foreground wait.

## Final state

Final inspection recorded:

- temporary module: unloaded;
- `clk_gp0` rate: 50,000,000 Hz;
- parent: `xosc`;
- enable count: 0;
- prepare count: 0;
- exclusive rate-protection count: 0;
- GPIO4: input, pull-up, high, and unclaimed;
- GPCLK mux on GPIO4: not selected;
- temporary probe processes: none;
- temporary probe files: removed; and
- core, submodule, and operator-documentation repositories: clean.

Loading the out-of-tree module set kernel taint value 4096. That diagnostic
flag persists until reboot. No reboot was authorized or performed.

## Supported conclusions

The observations support these limited conclusions for the tested Pi 5,
kernel, and revisions:

- the RP1 clock provider is accessible through kernel common-clock APIs;
- `clk_gp0` can be held with exclusive rate protection;
- its rate can be changed and read back while it remains disabled;
- the tested low-frequency requests can use `xosc`;
- the tested higher requests can reparent to `pll_sys`;
- none of the tested rate changes incremented enable or prepare counts;
- GPIO4 can be acquired exclusively through libgpiod;
- GPIO4 remained disconnected from GPCLK output throughout; and
- explicit module unload restored the original clock rate and ownership
  state.

## Non-goals and unresolved work

Phase 4 did not establish:

- actual clock output on GPIO4;
- enabled divider-update atomicity;
- transition timing or cleanliness;
- deterministic WSPR symbol deadlines;
- oscillator calibration accuracy;
- RF frequency placement or spectral purity;
- sideband or harmonic performance;
- a valid WSPR transmission;
- direct 2 m support; or
- production readiness.

The test module was a temporary evidence probe, not a production backend.
Its rate observations do not establish that Linux integer-hertz rate requests
can replace exact divider-word control for WSPR.

## Phase 5 prerequisites

Phase 5 requires a separate authorization and all of the following:

1. GPIO4 connected only to a shielded 50-ohm load, with no radiator or
   antenna attached.
2. An agreed test frequency and minimum practical drive.
3. The measurement instrument, attenuation, and RF chain identified.
4. A bounded output duration.
5. An immediate cleanup supervisor that does not depend on a deferred shell
   trap.
6. An independent stopping procedure.
7. Before, during, and after clock and GPIO evidence.
8. Explicit authorization for GPIO output and bounded RF generation.

Phase 5 must not be inferred as authorized by this Phase 4 result.
