# RP1 GPCLK Phase 6D bounded cancellation

## Disposition

**Passed the clock-disabled cancellation/lifecycle gate using bounded finite
completion. No GPIO output was enabled.**

The RP1 DMA channel was recovered by one authorized reboot. A normal
66,792-write descriptor then passed exact DMA and provider-path readback,
confirming recovery.

Immediate active-descriptor abort was rejected. The generic DW AXI DMA pause
operation returned `-EAGAIN` under an active RP1 tick handshake, and the Phase
6C tick-first terminate ordering had already demonstrated that removing DREQ
before termination can leave the channel non-idle.

The validated contract instead treats one WSPR-symbol descriptor as the atomic
hardware cancellation unit. Once submitted, it drains to normal hardware-idle
completion. Cancellation prevents any subsequent descriptor, waits for the
finite completion, disables tick/DREQ, verifies the final raw divider after a
50 ms stability interval, and restores the original 50 MHz clock state. The
hard cancellation bound is the remainder of one 0.682667-second symbol, never
more than 66,792 divider writes.

## Recovery

Before reboot, evidence captured:

- parent repository `8467befa93ffb85cf092458fe7a0c1bfb3d72cc3`;
- WSPR-Transmitter `7374d0fc84bc487c6e78e09c25123b0e1d64950e`;
- clean synchronized worktrees;
- GPIO4 input and GPCLK0 disabled at 50 MHz; and
- the Phase 6C `dma2chan2 failed to stop` and non-idle warnings.

One authorized `sudo reboot` was issued. After wspr5 returned:

- the same kernel and repository revisions were present;
- no overlay or temporary module was loaded;
- GPCLK0 prepare, enable, and protection counts were zero; and
- GPIO4 was explicitly restored to input with pull-up.

The recovery smoke descriptor completed in 682.665831 ms with exact raw
`0x8bfc0000` DMA/provider readback. No additional reboot, controller reset,
driver unbind, or persistent system change occurred.

## Termination-contract analysis

The running DW AXI DMA driver implements `terminate_all` by clearing the
channel-enable bit and polling it for up to 50 ms. It does not first invoke the
driver's hardware-suspend operation. Phase 6C removed the RP1 handshake before
that operation, so the channel could not reach the state needed to clear
enable.

The driver's separate pause operation asserts the channel suspend control and
polls for a suspended interrupt in roughly 40 microseconds. The Phase 6D
pause-before-disable experiment left tick/DREQ active, but pause returned
`-EAGAIN`. Its cleanup emitted no non-idle warning, and a subsequent normal
descriptor passed, proving channel reuse. Because pause acknowledgment was not
obtained, this path was not used for further qualification.

RP1 DMA-TICK `FINISH_CLEAR` remains useful for normal finite completion, but it
does not by itself provide an independently demonstrated active-transfer abort.
Force-disable remains unsuitable because the RP1 specification warns that it
races DMAC activity.

## Validated bounded-final-descriptor contract

The final research probe implements these invariants:

1. each symbol is one finite, preloaded 66,792-write DMA descriptor;
2. cancellation marks the generation and prevents future descriptor submission;
3. the current descriptor retains tick/DREQ until its normal completion IRQ;
4. cleanup disables DMA-TICK request and the tick generator after completion;
5. generic termination is then applied only to an already-idle channel;
6. a provider-path raw read after 50 ms must match the known final packed word;
7. the original 50 MHz parent/rate state is restored; and
8. the next test must allocate and successfully reuse a DMA channel.

Timeout handling uses the same rule. An initial wait timeout marks cancellation
but grants the finite descriptor a second one-second completion window. It does
not attempt an unsafe active-channel abort.

The RP1 DMA driver's residue is block-granular for this descriptor and commonly
reported no partial progress. The probe therefore records a nominal write
estimate from elapsed xosc cycles for diagnostics. Safety does not depend on
that estimate: the enforced bound is always the full finite descriptor.

## Results

| Case | Outcome | Evidence |
|---|---|---|
| Recovery smoke | Passed | 682.665831 ms; exact provider raw readback |
| Early cancellation, 10 ms | Passed | Drained one finite symbol; stable final divider; reuse passed |
| Mid cancellation, 100 ms | Passed | Drained one finite symbol; stable final divider; reuse passed |
| Near cancellation, 600 ms | Passed | Estimated request at write 60,703; 6,089 further writes; reuse passed |
| Repeated cancellation | Passed | Ten consecutive 100 ms cycles; no stop/non-idle warning |
| Reuse after repetitions | Passed | Normal descriptor completed |
| Injected post-DMA failure | Passed cleanup | Restored 50 MHz; following descriptor passed |
| Initial wait timeout, 100 ms | Passed | Converted to finite drain; stable final divider; reuse passed |
| Hardware pause attempt | Rejected | `-EAGAIN`; channel reuse nevertheless passed |

The successful cancellation descriptors completed between 682.670325 and
682.671510 ms in the captured runs. No test emitted `failed to stop` or
`non-idle` after the recovery reboot.

## Safety and final state

Every accepted path finished with:

- GPIO4 input, pull-up, high;
- GPCLK0 prepare count 0;
- GPCLK0 enable count 0;
- GPCLK0 protection count 0;
- GPCLK0 restored to 50,000,000 Hz;
- DMA tick and DREQ disabled;
- no runtime overlay loaded; and
- no temporary probe module loaded.

No GPIO mux, GPCLK output, RF emission, boot configuration, service, or
persistent system state was changed.

## Validation and evidence

- exact-kernel temporary module builds: passed;
- recovery descriptor and provider raw readback: passed;
- early, mid, near, repeated, timeout, and injected-failure cases: passed under
  the bounded-final-descriptor contract;
- channel reuse after every test class: passed;
- final source whitespace check: passed;
- Mac planner regression: passed.

Evidence is retained at:

```text
/home/pi/rp1-phase6d-cancellation-validation/
```

It contains pre/post-reboot state, source/build artifacts, individual runtime
logs, reuse logs, and `SHA256SUMS`.

## Supported conclusion and next gate

Tick-paced RP1 DMA now has demonstrated divider writes and a bounded,
fail-closed cancellation contract suitable for production-backend design,
provided product semantics explicitly define cancellation at the current WSPR
symbol boundary. Immediate mid-symbol abort remains unsupported and is not
required by the validated contract.

The next phase should implement the production kernel/provider and
WSPR-Transmitter backend interfaces while retaining a clock-disabled validation
gate. GPIO drive selection must be carried through that design with 2 mA as the
safe default and operator-selectable 2, 4, 8, and 12 mA values. No live-output
phase should begin until the production clock-disabled backend passes.
