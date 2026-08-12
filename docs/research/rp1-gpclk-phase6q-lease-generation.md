# Phase 6Q: RP1 generation semantics across process leases

## Outcome

Phase 6Q passed its clock-disabled qualification. Provider generation is now
scoped to each exclusive ownership lease. Two sequential direct-provider
owners and two independent production WsprryPi processes each completed a full
frame using their own initial generation `1`.

`live_output` remained disabled throughout. No RF was transmitted, no SDR
samples were captured, and no WSPR decode was attempted.

## Contract decision

The provider previously retained its last generation after terminal release.
A new WsprryPi process starts its backend generation counter at zero, so its
first generation `1` collided with the provider's retained `1` and `SUBMIT`
returned `EINVAL`.

Generation is now lease-scoped:

- a successful exclusive acquisition resets the provider generation to zero;
- submissions must remain strictly increasing and nonzero within that lease;
- state and stop operations must name the current generation;
- acquisition remains impossible while another owner exists; and
- release remains impossible while the descriptor is running or draining.

Resetting on successful acquisition is safe because no earlier owner or active
descriptor can coexist with the new lease. It requires no UAPI change and
preserves same-lease stale-generation rejection.

## Implementation and tests

The production provider resets `provider->generation` only after the clock
lease is acquired successfully. The portable core mirrors that behavior.
Portable tests now cover terminal release followed by a second independent
owner submitting generation `1`, plus same-lease stale rejection. KUnit adds a
lease-generation contract case. The static contract requires the production
reset.

The following passed:

- portable provider core on Mac and wspr5;
- kernel static contract on Mac and wspr5;
- Pi debug build;
- RP1 planner, lifecycle, transition, production-backend, Linux-provider, and
  scheduler-backend tests using `make -j3`; and
- KUnit: 3 passed, 0 failed, 0 skipped.

An initial direct-harness attempt incorrectly expected generation zero to be
stale immediately after acquisition. Zero is the valid idle-lease generation;
it becomes stale after generation `1` is submitted. The harness was corrected
before a descriptor ran.

## Provider installation and reboot identity

Only the provider and KUnit modules were built for the Pi
5/500/CM5-optimized `6.18.44-v8-16k+` kernel using `-j3`. The previous installed
modules were backed up before replacement. The corrected modules were installed
under `/lib/modules/6.18.44-v8-16k+/kernel/drivers/clk`, followed by `depmod` and
a reboot into the persistent engineered kernel.

- provider source version: `00435149E8EC6D24857F8C1`
- KUnit source version: `272C4EBBF19BBC0181ABB63`
- provider built/installed SHA-256:
  `3fcf38b20de3b1de5c994a31cc28ef21f5edf074d9f3cde837486e8afc22940f`
- KUnit built/installed SHA-256:
  `6377d9512e669c2b74070df82cac8391df3932beb88bcd627e4c8893b251616a`

The loaded module source version and boot-selected installed module matched the
built artifact after reboot.

## Two-owner provider proof

A current-UAPI direct harness opened, acquired, submitted, completed, released,
and closed the provider twice sequentially. Both owners independently used
generation `1`:

```text
owner=1 generation=1 terminal=COMPLETE elapsed=110.643528
owner=2 generation=1 terminal=COMPLETE elapsed=110.646513
two_owner_result=PASS
```

The harness also verified that generation zero is rejected as stale after each
generation `1` submission.

## Independent production-process proof

Two separately invoked production scheduler processes then used the same loaded
provider with `live_output=N`:

```text
owner 1: Completed transmission: 110.739458 seconds; rc=0
owner 2: Completed transmission: 110.706848 seconds; rc=0
```

Both completion callbacks required provider `COMPLETE`. Neither process
reported a backend failure. The second process demonstrates that provider reuse
no longer depends on reloading the module or inheriting userspace state.

## Cleanup and compatibility

The final state was `live_output=N`, GPIO4 input, GPCLK0 prepare/enable counts
zero, and both WsprryPi and SoapyRemote active. Generated portable test binaries
were removed. Pi-side raw evidence and its SHA-256 manifest remain under
`/home/pi/phase6q-evidence`.

The change does not alter the UAPI, Pi 4-and-earlier GPIO behavior, Si5351,
CW, web UI, operator configuration, or RP1 drive selections.

## Repository state

The core repository contains the provider, portable-test, KUnit, static-test,
and kernel-patch changes plus the uncommitted Phase 6P and Phase 6Q engineering
documentation. `src/WSPR-Transmitter` has no Phase 6Q modification. Nothing from
Phase 6P or Phase 6Q has been committed or pushed.

## Documentation impact

This engineering report records the lease-scoped contract and qualification.
No operator documentation was changed because live three-frame WSPR decoding
has not yet passed. Later developer documentation must cover provider module
installation, required kernel build dependencies, post-reboot identity checks,
and lease-scoped generation. CW qualification and the operator power-selection
workflow remain future work.
