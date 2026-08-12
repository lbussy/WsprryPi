# Phase 6O: persistent provider and fail-closed execution

## Outcome

Phase 6O passed its clock-disabled gate on `wspr5.local`.

- The current 304-byte-program provider and KUnit modules are installed in the
  engineered kernel module tree and survive boot.
- Every unsuccessful execution result now enters a controlled `FAILED` state;
  `faulted` remains additional recovery classification rather than a success
  condition.
- The direct CLI reports a provider submission failure and exits nonzero.
- Managed mode reports the failure, performs terminal cleanup, and inhibits
  further transmission without shutting down the service process.
- A production scheduler-originated WSPR frame reached provider completion in
  `110.740249` seconds and the process exited zero with `live_output=N`.

No GPIO clock was enabled and no RF was transmitted.

## Implementation

The transmitter now exposes a `FAILED` terminal state and callback. A failed
RP1 prepare or execution result can no longer proceed to `COMPLETE`. Submit
failure cleanup releases an acquired provider lease while preserving the
original provider error, including `ENOTTY`.

The parent scheduler logs the actionable backend error, performs the normal
terminal GPIO cleanup, and publishes a failed transmit state. Direct CLI mode
requests shutdown and makes `wspr_loop()` return false. Managed INI mode keeps
the process alive but inhibits transmission until configuration reload or
service restart.

## Persistent kernel/provider identity

The Pi 5/500/CM5-optimized 64-bit provider targets were built from the retained
`6.18.44-v8-16k+` tree. The broad `M=drivers/clk modules` target encountered
unrelated existing modpost errors in HifiBerry/Si5351 modules, so the exact
provider and KUnit targets were built successfully instead.

Persistent boot configuration selects:

```text
kernel=kernel_2712_phase6h.img
dtoverlay=rp1-gpclk-provider
auto_initramfs=0
```

The first normal reboot exposed that the engineered image had previously been
selected only through one-shot `tryboot.txt`. After the persistent settings
were added, the soft reboot did not return to the network and required the
operator's hard boot. The resulting boot was successful and selected:

- kernel: `6.18.44-v8-16k+ #3`
- provider source version: `E2B807F9BED056FF0867C9B`
- KUnit source version: `DE8E139164D0D59B31CD957`
- submit ioctl recognized by the installed provider: `0x4130b701`

Built and installed decompressed module hashes match:

```text
3faa1fe02e36ec4637e7b977dd2caaf36675bb6d6f8f0d9617210b9b424fa801  rp1-gpclk-provider.ko
672f3de7ebe0a8e2c794b8dc3e4093fa92a39aaac7b13b7b848945e282e1368f  rp1-gpclk-provider-kunit.ko
```

The kernel image hash is
`9851d1cb28482b5edfc18c4d887d5704f780d1bc10adcd233607126b2221eda1`;
the overlay hash is
`d3d42232d3bbd43b9bf376ceb130b5c3607639e57d6569131c09fc4a37b83e30`.

## Controlled failure proof

With the stale provider deliberately retained and `live_output=N`, its
304-byte `SUBMIT` rejection produced:

```text
Transmission failed: Could not submit RP1 GPCLK program: Inappropriate ioctl for device
state=failed, tx_state=failed
Shutdown requested: transmission backend failure
```

There was no completed-transmission callback and the direct CLI returned 1.
Focused backend tests also proved injected acquire and submit errors retain
their text, a failed acquire does not release an unowned lease, and a failed
submit releases the acquired lease.

The scheduler semantics test directly injected a managed backend `FAILED`
callback and proved that managed transmission becomes inhibited without setting
the process-exit request.

## Clock-disabled production cadence

After the persistent current provider boot, a one-frame production scheduler
run used GPIO4, 2 mA drive, and `live_output=N`. It started on the normal WSPR
boundary, reported:

```text
Started transmission: 14.097036 MHz.
Completed transmission: 110.740249 seconds.
state=finished, tx_state=complete
phase6o_rc=0
```

Provider `COMPLETE` is required for this path to emit the completion callback.
The provider's existing cadence enforcement accepted the frame; otherwise it
would have returned `FAILED`. This establishes clock-disabled production-path
completion but is not RF or decode qualification.

An initial attempt used a pseudo-terminal and was stopped by terminal job
control before scheduling began. Its identified processes were terminated,
the service was restored, and the test was rerun without a pseudo-terminal.
That artifact produced no frame and is excluded from cadence evidence.

## Validation

The following passed on `wspr5`:

- debug application build;
- RP1 planner, lifecycle, transition, production-backend, Linux-provider, and
  scheduler-backend tests;
- portable provider-core tests;
- provider static-contract tests;
- KUnit: 2 passed, 0 failed, 0 skipped;
- full `make -j3 semantics-test`, including the managed backend-failure
  regression, UI/source regression, GPIO band policy, log timestamp, and update
  comparison tests;
- installed-versus-built module hash and post-boot source-version checks; and
- one clock-disabled production scheduler frame with process return code 0.

No UI source was changed and no visual review was required.

## Cleanup and compatibility

The final audit found the engineered kernel active, `live_output=N`, GPIO4 as
input, provider prepare/enable counts zero, and both `wsprrypi.service` and
`SoapySDRServer.service` active. No kernel RP1/GPCLK warning, oops, or bug was
logged during the successful frame.

The changes do not alter Pi 4-and-earlier GPIO execution, Si5351 behavior, CW,
the web UI, operator configuration, or the established RP1 drive choices of
2, 4, 8, and 12 mA. CW qualification and any later power-selection workflow
remain separate future work.

## Repository state

- Parent base: `eff73a8a25506becfc170ea3561178a5f7f8804a` on
  `codex/issue-399-rp1-gpclk`.
- Transmitter base: `7c234796cf523657ea3c7d1806d3c6f70ee84ef2` on
  `codex/issue-399-rp1-gpclk-divider-planner`.
- Modified submodule: `src/WSPR-Transmitter`.
- Modified parent files: scheduler failure propagation, scheduler semantics
  coverage, this report, evidence summary, and the next prompt.
- No Phase 6O change has been committed or pushed.

## Documentation impact

This engineering report and evidence summary were added to the core repository.
No operator documentation was changed because live RF/decode qualification has
not yet passed. Later developer documentation must describe installation of the
provider for the boot-selected kernel, post-reboot UAPI identity verification,
and the required kernel build dependencies. Operator documentation remains
deferred until Pi 5 GPIO qualification succeeds.

Pi-side raw evidence remains at `/home/pi/phase6o-evidence`.
