# RP1 GPCLK Phase 6B deterministic update engine

## Disposition

**Failed the final clock-disabled register-acceptance gate. No GPIO output was
enabled.**

RP1's DMA0 tick can pace a preloaded finite transfer at the required WSPR
cadence without a sleep-capable operation per update. A temporary kernel probe
submitted all four 20 m profile buffers on `wspr5` while GPCLK0 remained
unprepared and disabled and GPIO4 remained an input. DMA completion alone did
not prove that the GPCLK0 fractional-divider register accepted those writes.

A final DMA readback returned zero after the clock framework had first placed
a known nonzero value in `DIV_FRAC`. The test therefore cannot distinguish an
unsupported DMA read from a DMA path that cannot access this clock register,
and direct CPU mapping of the clock provider's already-owned register window
was deliberately not used. Tick-paced DMA remains the leading candidate, but
Phase 6B does not select it as the production mechanism.

The candidate production architecture would be:

1. the RP1 clock provider owns GPCLK0 and its divider registers;
2. it reserves a DMA tick channel and preloads one finite fractional-word
   sequence per WSPR tone;
3. it writes the shared integer divider only while the clock is disabled;
4. DMA writes only the 32-bit `DIV_FRAC` register while a symbol is active; and
5. generation cancellation disables the tick/DREQ, synchronously terminates
   DMA, disables GPCLK output, and only then releases resources.

The next proof must be implemented inside the RP1 clock provider so that it can
perform an owned register readback and keep DMA setup, locking, and teardown
inside the provider rather than a companion driver.

## Primary-source findings

The [RP1 peripherals specification](https://datasheets.raspberrypi.com/rp1/rp1-peripherals.pdf)
states that DMA is the exception among non-processor bus masters that can
access peripheral registers. It also documents:

- two 9-bit, `clk_ref`-based DMA tick generators whose divisors can be changed
  at runtime;
- DMA tick DREQ generation with finish-clear and a short bus-cycle dwell;
- an eight-channel Synopsys AXI DMAC intended to pace arbitrary transfers; and
- GPCLK0 routing to GPIO4 and GPIO20.

The [Raspberry Pi Linux RP1 clock driver](https://github.com/raspberrypi/linux/blob/rpi-6.18.y/drivers/clk/clk-rp1.c)
defines GPCLK0 at clock-block offset `0x174`, with `DIV_INT` at `+0x4` and
`DIV_FRAC` at `+0x8`. Its normal rate operation writes the integer and
fractional registers separately under a global spinlock. It exposes no queued
or DMA-backed transition API.

The [RP1 device tree](https://github.com/raspberrypi/linux/blob/rpi-6.18.y/arch/arm64/boot/dts/broadcom/rp1.dtsi)
describes the clock block at RP1 system address `0xc040018000` and the
eight-channel DMA controller. The
[RP1 binding header](https://github.com/raspberrypi/linux/blob/rpi-6.18.y/include/dt-bindings/mfd/rp1.h)
assigns DMA request `0x30` to DMA tick 0. The
[DW AXI DMA driver](https://github.com/raspberrypi/linux/blob/rpi-6.18.y/drivers/dma/dw-axi-dmac/dw-axi-dmac-platform.c)
supports fixed-destination memory-to-device scatter/gather transfers, maps the
configured CPU physical destination into the DMA address space, and selects
the device-tree handshake number.

PIO was rejected as the divider writer. The public RP1 PIO interface can pace
FIFO transfers and manipulate pins, but PIO instructions do not issue
arbitrary APB writes. Using PIO would still require DMA or CPU intervention to
reach `DIV_FRAC` and would add another ownership boundary.

## Sequence and cadence analysis

The Phase 6 planner used 65,536 words per 0.682667-second WSPR symbol, implying
96,000 writes per second. The RP1 DMA tick's slowest setting is 511 xosc cycles:

```text
50,000,000 / 511 = 97,847.358 writes/second
```

The DMA tick dwell field cannot bridge this difference because it only
prevents handshakes from occurring too close together; the 511-cycle tick is
already much longer than the maximum dwell.

A numerical search found that a sequence as short as 413 entries happens to
meet the four-tone 0.01 Hz average-error requirement at 14.0971 MHz, but the
hardware tick cannot run that slowly. Reducing the sequence therefore does not
reduce the actual DMA cadence. Instead, sizing the sequence to the slowest
hardware tick gives 66,792 writes per symbol. Its four finite profiles are:

| Tone | Lower word/count | Upper word/count | Average error |
|---:|---:|---:|---:|
| 0 | 232445 / 66312 | 232446 / 480 | -0.000279 Hz |
| 1 | 232444 / 1134 | 232445 / 65658 | +0.000389 Hz |
| 2 | 232444 / 2747 | 232445 / 64045 | +0.000153 Hz |
| 3 | 232444 / 4360 | 232445 / 62432 | -0.000083 Hz |

All endpoints retain integer divider 3. The candidate DMA stream therefore
targets only fractional values 35836 or 35837 in one aligned 32-bit transfer.

## Clock-disabled prototype

The research-only source is in `tools/rp1_gpclk_dma_probe`. Its runtime overlay
contains no pinctrl or GPIO node. The driver:

- acquires `clk_gp0` and exclusive rate ownership;
- verifies the original rate is 50 MHz;
- never calls `clk_prepare` or `clk_enable`;
- claims only the DMA0 tick and DMA-tick register windows;
- requests DMA handshake 0x30 through DMAengine;
- submits one fixed-destination finite transfer;
- disables DREQ and the tick before terminating DMA; and
- restores 50 MHz through the common-clock framework on every exit.

An initial run supplied RP1's internal system address to DMAengine. The driver
rejected the descriptor with an invalid destination before performing a
transfer. The corrected overlay supplies Linux CPU physical address
`0x1f0001817c`; DMAengine translates it to RP1 system address `0xc04001817c`.

The corrected 65,536-word baseline DMA descriptor completed in 669.823163 ms.
Four 66,796-word descriptors completed in 682.710 ms, confirming the expected
sequence-length scaling.
The refined 66,792-word runs were:

| Tone | DMA completion time | Error from 682.666667 ms |
|---:|---:|---:|
| 0 | 682.669438 ms | +2.771 us |
| 1 | 682.668790 ms | +2.123 us |
| 2 | 682.670697 ms | +4.030 us |
| 3 | 682.670865 ms | +4.198 us |

The four-run spread was 2.075 us. These times include submission/start and
completion-observation latency, so they establish deterministic bounded DMA
descriptor execution at the required scale; they do not establish successful
divider writes and are not an RF timing measurement.

The subsequent readback diagnostic first called `clk_set_rate(14097098)` while
GPCLK0 remained disabled, making the expected fractional field nonzero. After
the paced descriptor, a one-word device-to-memory DMA read of the same
translated `DIV_FRAC` address returned zero rather than 35836. The probe failed
closed with `-EIO` and restored 50 MHz. Because the generic clock API caches
the selected rate, `clk_get_rate()` cannot serve as independent raw-register
readback. Resolving this requires clock-provider-owned instrumentation.

Every run ended with:

- GPIO4 input, pull-up, high;
- GPCLK0 enable count 0;
- GPCLK0 prepare count 0;
- GPCLK0 restored to 50,000,000 Hz;
- the runtime overlay removed; and
- the temporary module unloaded.

The kernel remains tainted by temporary out-of-tree probes. No reboot,
persistent overlay, service change, GPIO mux, GPCLK output, or RF emission was
performed.

## Tests and evidence

The planner regression now fixes the 66,792-entry 20 m counts and requires
each tone's numerical average error to remain below 0.0005 Hz.

- Mac `make -C src rp1-gpclk-planner-test`: passed;
- wspr5 `make -C src rp1-gpclk-planner-test`: passed;
- Mac and wspr5 `git diff --check`: passed;
- wspr5 four-tone clock-disabled DMA timing probe: passed;
- wspr5 known-divider DMA readback: failed with observed value zero; and
- final GPIO4 and GPCLK0 state check: passed.

Hardware evidence is retained on `wspr5` at:

```text
/home/pi/rp1-phase6b-dma-validation/
```

The directory contains both sequence experiments, final state, source,
module, overlay, and `SHA256SUMS`.

## Production requirements and next gate

Phase 6B does not make the research probe a backend or prove DMA access to
`DIV_FRAC`. The next implementation phase must:

1. add a temporary clock-provider-owned GPCLK DMA self-test or equivalent RP1
   driver extension that can perform raw owned readback while output is
   disabled;
2. prevent normal CCF rate changes while a sequence is armed or running;
3. expose finite start, completion, cancellation, cutoff, and error semantics
   to the existing generation-safe transition lifecycle;
4. preload all four 66,792-entry profiles and switch only at symbol boundaries;
5. retain 2 mA as the eventual GPIO drive-strength default while providing the
   planned operator-selectable 2, 4, 8, and 12 mA control; and
6. first prove paced writes by raw owned register readback, then validate the
   production path while the clock remains disabled before any separately
   authorized live-output phase.

No live-output prompt should be executed from this result. The immediate next
prompt is an RP1 clock-driver-owned, clock-disabled DMA write/readback proof.

## Phase 6C resolution

Phase 6C established that the Phase 6B zero readback was caused by incorrect
register packing, not a lack of DMA access. RP1 stores the 16-bit GPCLK
fractional divider in bits 31:16 of `DIV_FRAC`; Phase 6B wrote the logical
fraction into bits 15:0, which hardware masks to zero. Correctly shifted DMA
words were subsequently confirmed by exact DMA readback and by the existing
RP1 provider's raw regmap reads. See
`rp1-gpclk-phase6c-provider-dma-proof.md` for the separate cancellation failure
that now gates production work.
