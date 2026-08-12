# Phase 6M: three-frame Pi 5 GPIO WSPR decode qualification

## Outcome

Phase 6M stopped at the required clock-disabled preflight gate. No live RF
frame was transmitted and no SDR capture or WSPR decode was attempted.

The portable provider core, static kernel contract, focused RP1 planner and
backend tests, native Pi debug build, and KUnit contract all passed. The first
complete scheduler-originated clock-disabled frame did not, however, exercise
the expected 110.592-second provider cadence. The scheduler reported:

```text
Started transmission: 14.097081 MHz.
Completed transmission: 0.042406 seconds.
```

The process returned zero, but no new provider timing observation appeared in
the kernel log. This is a provider/scheduler preflight regression relative to
Phase 6L, where the same clock-disabled production path completed full frames
and the provider callback cadence was inside its enforced timing window.
Phase 6M required stopping further live work on any provider, DMA, timing, or
cleanup regression, so `live_output=1` was not set.

## Verified identity and test path

- Host: `wspr5.local`, Raspberry Pi 5 Model B Rev 1.0.
- Kernel: `6.18.44-v8-16k+ #3 SMP PREEMPT Mon Aug 10 16:19:21 CDT 2026`.
- Parent source: `ad25d2a2744fc8984d7a919928213cfa12319a3c` on
  `codex/issue-399-rp1-gpclk`.
- Transmitter source: `7c234796cf523657ea3c7d1806d3c6f70ee84ef2` on
  `codex/issue-399-rp1-gpclk-divider-planner`.
- Provider module: in-tree `rp1_gpclk_provider`, source version
  `C09AA574CBFF079D4B5A6FA`.
- WSPR identity: `NXXX`, `ZZ99`, reported power `20 dBm`.
- Path: 20 m, GPIO4, RP1 GPCLK provider, minimum 2 mA drive.
- Intended receiver: attached SDRplay RSP1B, 250 ksample/s capture centered at
  14.122100 MHz, decoded independently with `wsprd`.

## Preflight results

- Portable provider arithmetic/lifecycle tests: pass.
- Static ownership, submission, and cadence contract: pass.
- Native Pi debug build at parent `ad25d2a`: pass.
- RP1 planner, production backend, Linux provider client, and scheduler backend
  tests: pass.
- KUnit provider contract: 2 pass, 0 fail, 0 skip.
- Clock-disabled provider gate: fail; scheduler-visible duration was
  0.042406 seconds rather than a complete WSPR frame.
- Live frames: 0 of 3 attempted.
- Independent WSPR decodes: 0 of 3 attempted.

The Pi-side evidence is retained under `/home/pi/phase6m-evidence`. Its
`SHA256SUMS` file seals the preflight logs, KUnit output, GPIO/clock snapshots,
repository state, and final state.

## Cleanup and compatibility

The run ended with `live_output=N`, GPIO4 restored to input under the
provider's safe 2 mA pinctrl state, GPCLK0 prepare and enable counts at zero,
and both `wsprrypi.service` and `soapyremote-server.service` active. The parent
and transmitter worktrees on `wspr5` are clean and synchronized with origin.

Pi 4-and-earlier GPIO behavior, Si5351 operation, web UI behavior, power
selection, and operator configuration were unchanged.

## Qualification status and next work

Pi 5 GPIO WSPR has **not** met the three-independent-decode qualification
gate. Before repeating Phase 6M, determine why the production clock-disabled
provider/scheduler path terminates immediately and ensure that a provider
frame failure is surfaced as a nonzero scheduler/process result rather than a
successful completion.

CW remains future work and was not exercised by this phase.

## Documentation impact

No operator documentation was changed. The implementation is not yet
qualified, and Phase 6M introduced no operator-visible behavior. Operator
documentation for Pi 5 GPIO support remains deferred until WSPR qualification
passes; CW documentation remains deferred until the later CW implementation
and qualification work.
