# RP1 GPCLK Phase 6C provider-path DMA proof

## Disposition

**The divider-write proof passed; the cancellation/lifecycle gate failed. No
GPIO output was enabled.**

Tick-paced RP1 DMA writes to GPCLK0 `DIV_FRAC` were conclusively demonstrated
with the clock unprepared and disabled. All four 66,792-entry 20 m profiles
completed at the expected cadence, exact raw DMA readback matched the final
word, and the existing RP1 clock provider independently consumed the same raw
integer and fractional values through its own regmap path.

The first mid-transfer cancellation test then exposed a separate blocker. The
probe disabled the DMA tick and DREQ before calling
`dmaengine_terminate_sync()`. The DW AXI DMA driver reported that its channel
failed to stop because the pending transfer no longer received the handshake
needed to reach a suspend boundary. The channel remained non-idle and the next
probe timed out. No controller reset, driver unbind, or reboot was attempted.

The DMA write architecture is therefore proven but is not ready for backend
integration. A safe RP1-specific abort/termination sequence must be designed
and demonstrated after the harness is recovered under a separate reboot gate.

## Exact environment

The proof ran on `wspr5`:

```text
Linux 6.18.34+rpt-rpi-2712
Debian package 1:6.18.34-1+rpt1
aarch64 SMP PREEMPT
```

The temporary module was built against the installed matching headers. Source
contracts were checked against Raspberry Pi's 6.18 RP1 clock driver, the live
kernel symbols, and the running driver's behavior. The relevant built-in
symbols included `rp1_clock_recalc_rate`, `regmap_read`, and `__clk_get_hw`.

Repository tips at the start were:

- WsprryPi `f128b358949a3dccd6bfff05a765dd87906aeac8`;
- WSPR-Transmitter `7374d0fc84bc487c6e78e09c25123b0e1d64950e`.

Both Mac and wspr5 worktrees were clean and synchronized before the phase.

## Root cause of the Phase 6B zero

Phase 6B used the correct destination address but the wrong register encoding.
The planner represents a 16.16 divider as one logical word. RP1's physical
registers instead store:

- the integer field in `DIV_INT`; and
- the 16-bit fractional field in bits 31:16 of `DIV_FRAC`.

The RP1 provider writes the fractional register as:

```text
DIV_FRAC = logical_fraction << 16
```

Phase 6B sent values such as `0x00008bfc`. RP1 masks the unimplemented low 16
bits, so readback correctly returned zero. Phase 6C sent `0x8bfc0000` or
`0x8bfd0000`. The zero was not evidence that RP1 DMA could not reach the clock
block.

Address translation remained:

```text
CPU physical DIV_FRAC: 0x1f0001817c
RP1 DMA DIV_FRAC:      0xc04001817c
DMA request:           RP1_DMA_DMA_TICK_TICK0 (0x30)
```

## Provider-path raw verification

The temporary probe continued to acquire `clk_gp0` through the common-clock
framework and hold exclusive rate ownership. It never called `clk_prepare()`
or `clk_enable()` and never mapped the clock register block.

For independent provider verification, the probe temporarily resolved and
called the running provider's `rp1_clock_recalc_rate()` using kprobe symbol
resolution. A task-scoped kretprobe on `regmap_read()` captured the two raw
values read by that provider function. This retained the provider's actual
regmap path rather than creating a second clock-register mapping.

Success required all of the following:

1. completion of the tick-paced finite DMA descriptor;
2. a one-word DMA device-to-memory read matching the final packed word;
3. provider-path raw `DIV_INT` equal to 3;
4. provider-path raw `DIV_FRAC` equal to the final packed DMA word; and
5. provider recalculation consistent with that divider.

## Four-profile results

| Tone | Duration | Final raw fraction | Provider raw fraction | Provider rate |
|---:|---:|---:|---:|---:|
| 0 | 682.669484 ms | `0x8bfd0000` | `0x8bfd0000` | 14,097,098 Hz |
| 1 | 682.669465 ms | `0x8bfc0000` | `0x8bfc0000` | 14,097,158 Hz |
| 2 | 682.669446 ms | `0x8bfc0000` | `0x8bfc0000` | 14,097,158 Hz |
| 3 | 682.671446 ms | `0x8bfc0000` | `0x8bfc0000` | 14,097,158 Hz |

All four provider reads returned integer divider 3. Each descriptor was within
4.779 microseconds of the 682.666667 ms WSPR symbol duration, and the four-run
spread was 2.000 microseconds. The result proves deterministic clock-register
writes while disabled; it is not GPIO, RF, or decode qualification.

## Cancellation failure

The cancellation fixture requested termination approximately 100 ms into tone
2. Its cleanup sequence was:

1. clear DMA-tick DREQ enable;
2. stop the tick generator; and
3. call `dmaengine_terminate_sync()`.

The DW AXI DMA driver logged:

```text
dma dma2chan2: dma2chan2 failed to stop
dma dma2chan2: dma2chan2 is non-idle!
```

The provider's first post-cancellation raw read succeeded, but a second
instrumented read returned `-EINVAL` after the failed termination path. The
probe treated this as a stale-write verification failure, restored the clock,
removed the overlay, and unloaded. A subsequent injected-failure fixture could
not start because the same channel was still non-idle and timed out after two
seconds.

This does not disprove paced DMA writes. It disproves the tested cancellation
ordering and means cancellation, injected failure, and no-stale-write behavior
remain unqualified. The shared DMA controller must not be reset or unbound as
an incidental test action.

Likely next candidates are:

- request DMA suspension/termination while the tick handshake remains active,
  then disable DREQ only after the channel acknowledges idle;
- use RP1 DMA-TICK `FINISH_CLEAR` with the DMAC finish signal;
- determine whether the RP1 DMA-TICK force-disable/abort contract can safely
  cooperate with the DW AXI DMAC suspend sequence; or
- add a narrow RP1-specific termination operation to the DMA driver if the
  generic DMAengine contract cannot express the required ordering.

Each candidate must define a hard bound on additional writes after
cancellation and restore the final divider intentionally.

## Cleanup and safety state

After the failed cancellation and blocked injected-failure attempt:

- GPIO4 was input, pull-up, high;
- GPCLK0 enable count was 0;
- GPCLK0 prepare count was 0;
- GPCLK0 protection count was 0;
- GPCLK0 was restored to 50,000,000 Hz;
- no runtime overlay was loaded;
- no temporary module was loaded;
- no boot, service, or persistent system configuration changed; and
- no GPIO clock or RF output occurred.

The selected DMA channel may remain non-idle until controller recovery or
reboot. No further DMA qualification should run on this boot.

## Validation and evidence

Completed checks:

- temporary module build against exact running headers: passed;
- four corrected clock-disabled tone profiles: passed;
- exact DMA raw readback: passed for all four profiles;
- provider-path raw readback: passed for all four profiles;
- provider recalculated-rate check: passed for all four profiles;
- mid-transfer cancellation: failed because the channel did not stop;
- injected post-DMA failure: blocked by the non-idle channel;
- final GPIO4/GPCLK0/overlay/module audit: passed;
- final probe source with `cancel_ms` hard-disabled: compile passed; and
- final source whitespace check: passed.

Evidence is retained on `wspr5` at:

```text
/home/pi/rp1-phase6c-provider-dma-validation/
```

It contains source, module, overlay, four tone logs, cancellation and failure
logs, final state, and `SHA256SUMS`.

The final checked-in probe source, which rejects nonzero `cancel_ms`, received
a separate compile-only validation at:

```text
/home/pi/rp1-phase6c-source-validation/
```

## Supported conclusion and next gate

Phase 6C proved that correctly packed, tick-paced RP1 DMA writes reach
GPCLK0's fractional divider and are visible through the existing clock
provider's raw regmap path. It also proved that the attempted fail-closed
cancellation ordering is unsafe for the DW AXI DMA channel.

The next gate requires an authorized reboot of `wspr5` to recover the DMA
controller, followed by a strictly clock-disabled Phase 6D investigation of
RP1/DW AXI DMA suspension, finish, abort, and no-stale-write semantics. No live
output phase should be rendered or executed before that lifecycle gate passes.
