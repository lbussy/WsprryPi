# Phase 6Q prompt: correct RP1 generation semantics across process leases

Continue Issue #399 on `wspr5.local` and the current
`codex/issue-399-rp1-gpclk` repositories. Do not commit or push unless
separately instructed.

Use the Phase 6P clock-disabled `EINVAL` diagnosis. Keep scope limited to the
generation mismatch between an RP1 provider that retains its last generation
after release and a new WsprryPi process whose backend counter starts at zero.
Do not enable live output, transmit, capture SDR samples, decode WSPR, implement
CW, change power selection, modify the web UI, or update operator documentation.

Preserve existing work and verify the parent, transmitter, kernel, provider,
UAPI, and installed-module identities. Reproduce the defect with a
clock-disabled provider harness using two sequential independent owners: owner
one submits generation 1, reaches a terminal state, and releases; owner two
then submits its own generation 1 and currently receives `EINVAL`.

Define and implement the smallest coherent generation contract. Prefer
lease-scoped generation if resetting the provider generation during a safe new
acquisition cannot race a transfer, delayed verification, release, or stale
state query. Otherwise extend acquisition so userspace can seed its next
generation from the provider. Do not weaken same-lease stale-generation
rejection, terminal-state ownership rules, or fail-closed behavior.

Add focused portable-core, KUnit, Linux-provider, fake-backend, and scheduler
regressions proving:

- two sequential independent owners can each submit their initial frame;
- generation remains strictly increasing within one ownership lease;
- stale state and stop requests remain rejected;
- a new owner cannot acquire while an earlier transfer or delayed completion is
  active;
- cancellation, terminal completion, release, and reacquisition balance the
  lease and restore GPIO/clock state; and
- submission validation errors still produce `FAILED`, no completion callback,
  and a nonzero direct-CLI result.

Build only the Raspberry Pi 5/500/CM5-optimized 64-bit provider modules using
the established processor rule. If the kernel provider changes, install the
exact provider and KUnit artifacts through the persistent module path and
reboot if required. Prove built, installed, loaded, and UAPI identities match.

Run the two-owner clock-disabled harness and two separate clock-disabled
production scheduler processes. Require both processes to complete full frames
inside the existing cadence contract and return zero. Stop on any provider,
DMA, cadence, ownership, cleanup, or fail-closed regression. Do not enable RF.

Finish with `live_output=N`, GPIO4 input in the safe 2 mA state, zero GPCLK
prepare/enable counts, and both WsprryPi and SoapyRemote services active.
Produce durable Phase 6Q evidence covering the contract decision,
implementation, tests, artifact and reboot identity, both independent process
runs, cleanup, compatibility, documentation impact, and repository state.
Render the next prompt but do not resume Phase 6P live decode qualification or
commit/push without separate instruction.
