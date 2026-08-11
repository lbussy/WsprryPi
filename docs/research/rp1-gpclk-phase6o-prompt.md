# Phase 6O prompt: persist the current provider and fail closed in userspace

Continue Issue #399 on `wspr5.local` and the current
`codex/issue-399-rp1-gpclk` repositories. Do not commit or push unless
separately instructed.

Use the confirmed Phase 6N diagnosis. Keep scope limited to two corrections:
persist the current 304-byte-program RP1 provider as the module selected by the
Pi 5 boot kernel, and make every unsuccessful RP1 execution result propagate as
a controlled scheduler/application failure. Do not enable live output,
transmit, capture SDR samples, decode WSPR, implement CW, change power
selection, modify the web UI, or update operator documentation.

Preserve existing work. Verify the parent, transmitter, kernel tree, patch,
overlay, built-module, and installed-module identities before changing
anything. The current userspace submit command is `0x4130b701`; the stale
boot-installed module recognizes `0x4030b701`. Build only the Raspberry Pi
5/500/CM5-optimized 64-bit kernel modules, using all Pi 5 processors as already
authorized. Install the provider and KUnit modules through the established
kernel module installation path, update module dependencies, and reboot
`wspr5` if necessary. After reboot, prove that the module selected from
`/lib/modules/$(uname -r)` is the current artifact, has the expected source
version, and recognizes `0x4130b701`.

In userspace, preserve the distinction between an execution failure and the
additional `faulted` recovery classification, but never treat `ok=false` as a
successful transmission. Propagate preparation, acquisition, submission,
state-query, provider-terminal, and cleanup failures through a controlled
transmitter failure state. Do not emit a completed-transmission callback for a
failed execution. Direct CLI operation must exit nonzero, and managed service
operation must report the actionable backend error while remaining safely idle
or inhibited under its existing lifecycle contract. Do not introduce an
uncaught transmit-thread exception as the failure mechanism.

Add focused regression tests proving that:

- a non-faulted `ok=false` execution result cannot become `COMPLETE`;
- RP1 `ACQUIRE` and `SUBMIT` failures preserve their error text;
- an `ENOTTY` submission failure produces no completion callback and a nonzero
  direct-CLI result;
- managed mode reports the backend failure safely;
- provider ownership, GPIO4, GPCLK counts, and scheduler cleanup are balanced
  after every failure; and
- the built, installed, and post-reboot provider UAPI identities match.

Run the portable provider, static contract, KUnit, focused RP1 backend,
scheduler, lifecycle, and applicable source-regression tests. Then run one
production scheduler-originated complete frame with `live_output=N`. Require a
provider `COMPLETE` terminal state inside the existing 110.592-second plus or
minus 6.75 ms cadence contract and a successful process result. If it fails,
stop without enabling RF.

Finish with `live_output=N`, GPIO4 input in the safe 2 mA state, zero GPCLK
prepare/enable counts, and both WsprryPi and SoapyRemote services active.
Produce durable Phase 6O evidence covering source and artifact identity,
installation and reboot persistence, tests, controlled failure behavior,
clock-disabled cadence, cleanup, compatibility, documentation impact, and
repository state. Render the next prompt, but do not begin Phase 6M live decode
qualification or commit/push without separate instruction.
