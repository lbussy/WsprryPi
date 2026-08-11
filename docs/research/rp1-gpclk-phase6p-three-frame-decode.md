# Phase 6P: three-frame Pi 5 GPIO WSPR decode qualification

## Outcome

Phase 6P stopped at the required clock-disabled scheduler preflight. No live
output was enabled, no RF frame was transmitted, and no SDR capture or WSPR
decode was attempted.

The provider, userspace, portable-core, static-contract, and KUnit checks
passed. The production scheduler then failed closed when the provider rejected
`SUBMIT` with `EINVAL`:

```text
Started transmission: 14.097091 MHz.
Transmission failed: Could not submit RP1 GPCLK program: Invalid argument
clock_disabled_rc=1
```

Unlike the Phase 6M false-success defect, this failure emitted no completion
callback and returned a nonzero process result.

## Identity and preflight

- Parent committed tip: `830154fc517a7f42b44f953e3709e6f220c7c43f`.
- Transmitter committed tip: `d6eb8bb6568d612483b48c6ccf7181449cfaa06e`.
- The preserved Pi worktree remains based on the preceding commits, but every
  modified implementation and test file compared byte-for-byte equal to the
  corresponding pushed tip.
- Kernel: `6.18.44-v8-16k+ #3`.
- Provider source version: `E2B807F9BED056FF0867C9B`.
- Provider UAPI submit command: `0x4130b701`.
- Initial state: `live_output=N`, GPIO4 input, GPCLK0 prepare/enable counts
  zero, WsprryPi and SoapyRemote active.
- Intended identity and path: `AA0NT`, `EM18`, `20 dBm`, 20 m, GPIO4, 2 mA.
- Receiver path: attached SDRplay RSP1B at 250 ksample/s, centered at
  14.122100 MHz, with independent `wsprd` decoding planned.

The debug build and RP1 planner, lifecycle, transition, backend,
Linux-provider, and scheduler-backend tests passed using `make -j3`. Portable
provider-core and static kernel contract tests passed. KUnit reported 2 pass,
0 fail, and 0 skip.

Two preflight harness corrections were required: the kernel `drivers/clk`
directory has neither the portable test target nor a `clean` target. The
portable test actually lives under `tools/rp1_gpclk_provider`. These command
errors changed no provider or GPIO state; the corrected tests passed.

## Root cause

The provider generation counter persists across ownership leases. The
successful Phase 6O process submitted generation `1`, and releasing the
provider did not reset `provider->generation`.

Each new WsprryPi process constructs a new `Rp1GpclkBackend` whose local
`generation_` starts at zero. Its first frame therefore also submits generation
`1`. Provider validation requires a submitted generation to be nonzero and
strictly greater than the retained provider generation. The first Phase 6P
submission was consequently rejected with `EINVAL` before DMA preparation or
output activation.

This means the current implementation supports a first process after module
load but not a later independent process using the same loaded provider. Phase
6O passed because it was the first successful submission after the persistent
provider boot; Phase 6P exposed the cross-process lease mismatch.

The smallest correction should make generation semantics consistent across
independent ownership leases. One candidate is to scope provider generation to
the lease and reset it only when a new owner safely acquires the idle provider.
An alternative is a UAPI mechanism that returns the current provider generation
to the acquiring client. The choice must preserve stale-state rejection and
must be tested across sequential owners, cancellation, release, and delayed
completion before another scheduler run.

## Qualification status

- Live frames attempted: 0 of 3.
- SDR captures attempted: 0 of 3.
- Independent WSPR decodes attempted: 0 of 3.
- Pi 5 GPIO WSPR three-decode gate: not met.

## Cleanup and compatibility

The final state was `live_output=N`, GPIO4 input, GPCLK0 prepare/enable counts
zero, and both services active. No RF was transmitted. Generated portable test
output was cleaned, while Pi-side raw evidence and its SHA-256 manifest remain
under `/home/pi/phase6p-evidence`.

No Pi 4-and-earlier GPIO, Si5351, CW, web UI, operator configuration, or power
selection behavior was changed.

## Repository state

No Phase 6P source change was made. This report, evidence summary, and next
prompt are uncommitted parent-repository additions. The existing preserved Pi
worktree state was not reset, staged, or overwritten.

## Documentation impact

This core-repository engineering report records the failed qualification gate.
No operator documentation was changed because Pi 5 GPIO WSPR remains
unqualified. Developer documentation must eventually cover persistent provider
installation, build dependencies, and the finalized cross-process generation
contract. CW qualification and the operator power-selection workflow remain
future work.
