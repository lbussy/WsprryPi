# RP1 GPCLK Phase 6E production-backend contract

## Disposition

**Passed the clock-disabled userspace/backend contract gate. Kernel-provider
delivery remains the next gate. No GPIO output or clock was enabled.**

Phase 6E replaces register-shaped application logic with an address-independent
provider interface. WSPR-Transmitter supplies finite tone programs and lifecycle
requests; the future RP1 clock provider owns register addressing, divider
packing verification, clock-provider locking, DMA/tick resources, exclusive
ownership, stable final readback, and restoration.

The running wspr5 kernel does not expose that provider interface. Consequently,
this phase does not install a production provider, route normal transmissions
through RP1, or claim end-to-end production readiness.

## Implemented contract

`Rp1GpclkBackend` provides:

- an abstract provider API with no register addresses or companion-owned clock
  registers;
- exact 20 m symbol invariants: 50 MHz planning input, 66,792 divider writes,
  tick divider 511, and `DIV_FRAC` in the upper 16 bits;
- four exact planner-derived tone programs;
- monotonically increasing generation identifiers;
- cancellation and timeout requests that both select finite completion;
- refusal to release a descriptor while the provider reports running or
  draining;
- release and channel reuse after complete or failed terminal state; and
- 2, 4, 8, and 12 mA drive profiles, with 2 mA as the default and rejection of
  every other value.

The parent configuration now persists the RP1-specific value as
`GPIO.RP1 Drive mA`. This deliberately does not reinterpret the existing
`GPIO.Power Level` 0-7 setting used by Raspberry Pi 1-4.

Hardware application of drive strength is deferred to the kernel-provider
implementation. Phase 6E only validates and transports the value.

## Cancellation and teardown

One 66,792-write symbol descriptor remains the atomic cancellation unit. Once a
provider accepts a program, cancellation or timeout requests finite stop for
that generation. Cleanup fails while the provider reports `running` or
`draining`; it releases ownership only after `complete` or `failed`.

The API intentionally has no immediate-abort operation. It therefore cannot
reintroduce the Phase 6C tick-first termination failure.

## Validation

Mac hardware-independent tests passed:

```text
make rp1-gpclk-backend-test rp1-gpclk-planner-test \
  rp1-gpclk-transition-test rp1-gpclk-lifecycle-test
```

The Mac emitted its known Linux-specific Makefile discovery warnings. All four
requested binaries passed. A broader Mac `make semantics-test` could not compile
because clang treats the GNU-only `-fmax-errors=10` option as an error.

On wspr5, a temporary source snapshot outside the synchronized checkout passed:

```text
make rp1-gpclk-backend-test semantics-test
```

This covered backend packing, all four tones, all four drive profiles, invalid
drive rejection, generation reuse, cancellation, timeout, cleanup, parent JSON
persistence, and the existing parent source/runtime regressions.

An initial execution of a semantics binary copied with stale objects
segfaulted because the configuration struct layout had changed. A clean rebuild
of the temporary tree removed the ABI mismatch and the complete suite passed.
This was a harness preparation error, not a product runtime failure.

Final hardware readback was:

```text
GPIO4  input
clk_gp0 prepare_count=0 enable_count=0 rate=50000000
```

The real wspr5 checkout remained clean and synchronized at the Phase 6D tips.
No pin mux, clock prepare/enable, RF, service, overlay, module, `/dev/mem`, reboot,
reset, unbind, or persistent configuration change occurred.

## Documentation impact

- Updated: this engineering report and the Phase 6E evidence summary.
- Considered but unchanged: operator documentation, because Pi 5 GPIO output is
  not selectable or operational yet.
- Still required: operator configuration and safety documentation after the
  provider is integrated and hardware-qualified.

## Next gate

Implement the provider side in the RP1 clock-driver ownership domain, expose a
versioned address-independent UAPI, build it against the exact wspr5 kernel,
and integrate a concrete WSPR-Transmitter client. Its first validation must
remain clock-disabled. Installation or reboot requires a separately authorized
phase.
