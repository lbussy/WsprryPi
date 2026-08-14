# Pre-Pi 5 GPIO DKMS Feasibility Contract

Related issue: [#413](https://github.com/WsprryPi/WsprryPi/issues/413)

## 1. Purpose

Determine whether the Raspberry Pi 1–4 GPIO transmission backend can be
replaced by a stock-kernel-compatible driver distributed as source and built
locally through DKMS.

The effort is successful only if the proposed driver materially improves
resource ownership, failure isolation, cleanup, and security while preserving
WsprryPi's currently supported transmission behavior.

This contract authorizes investigation and bounded prototype work only when a
later prompt explicitly authorizes the applicable phase. It does not authorize
hardware operation, installation, service changes, transmission, or RF output.

## 2. Durable objectives

The candidate architecture should, where stock kernel interfaces permit:

- remove production userspace access to `/dev/mem`;
- replace fixed DMA-channel assumptions with DMAengine allocation;
- move GPCLK, PWM pacing, DMA, pinmux, and cleanup ownership into the kernel;
- expose a bounded, versioned userspace interface;
- build locally against the installed kernel through DKMS;
- fail closed when required resources or compatible kernels are unavailable;
- preserve GPIO4 and GPIO20 as explicitly supported output routes;
- improve process-death, cancellation, unload, and failed-startup cleanup;
- reduce or eliminate the need to blacklist `snd_bcm2835`; and
- reuse architecture proven by the RP1 work where the hardware contracts
  genuinely match.

## 3. Governing decision rule

Proceed beyond feasibility only if the kernel module provides a material
improvement over the existing backend.

A favorable result requires evidence that the driver can:

1. acquire DMA through a kernel ownership interface;
2. claim only the selected pin through pinctrl;
3. coordinate GPCLK0 through the common-clock framework;
4. obtain exclusive or defensibly isolated PWM pacing resources;
5. avoid resetting, disabling, or restoring resources it did not acquire;
6. perform bounded divider sequencing;
7. cancel or drain safely;
8. restore a defined safe state after every partial failure;
9. operate without unrestricted userspace peripheral mapping; and
10. preserve the required transmission timing and frequency behavior.

If the design still depends on broad, uncoordinated manipulation of DMA, PWM,
GPCLK, or GPIO behind existing kernel drivers, it should be rejected as
insufficiently better than the legacy implementation.

## 4. Scope

### Included platforms

Investigate separately:

- BCM2835-class systems;
- BCM2836 and BCM2837 systems;
- BCM2711 systems;
- supported 32-bit Raspberry Pi OS kernels; and
- supported 64-bit Raspberry Pi OS kernels.

No result may be generalized across these groups without evidence that their
DMA, address, clock, PWM, and kernel-driver contracts are equivalent.

### Included resources

- GPCLK0;
- GPIO4;
- GPIO20;
- BCM DMA;
- PWM pacing and its DMA request;
- clock-manager divider sequencing;
- DMA-visible program memory;
- pinctrl;
- common-clock APIs;
- module lifetime;
- userspace ownership; and
- DKMS build and update lifecycle.

### Included modes

The architecture must eventually accommodate all modes currently supported by
the legacy GPIO backend, including:

- WSPR;
- QRSS;
- FSKCW;
- DFCW;
- applicable finite TONE operation; and
- cancellation and shutdown during each mode.

Each mode retains its own qualification requirements.

## 5. Explicit non-goals

This effort must not:

- redesign the RP1 module around legacy BCM hardware;
- make the legacy investigation a prerequisite for RP1 support;
- create or distribute a custom kernel;
- retain `/dev/mem` as a silent production fallback;
- accept arbitrary GPIO selection;
- assume that DKMS compilation proves runtime compatibility;
- infer GPIO20 qualification from GPIO4;
- infer one Pi generation's qualification from another;
- replace working legacy behavior before equivalent evidence exists;
- weaken current failure, cleanup, timing, or RF requirements; or
- perform hardware or RF activity without separate authorization.

## 6. Architectural boundary

The intended arrangement is:

```text
WsprryPi planner and scheduler
             |
      shared versioned UAPI
             |
   legacy BCM DKMS module
     |        |        |
 DMAengine  pinctrl  common-clock
     |
 validated PWM-paced GPCLK divider execution
```

Userspace should retain:

- transmission planning;
- mode semantics;
- frequency and timing policy;
- configuration validation;
- scheduling;
- status presentation; and
- compatibility-state handling.

The kernel module should own:

- resource acquisition;
- bounded program validation;
- DMA memory;
- DMA submission;
- PWM pacing;
- selected GPIO pinctrl state;
- GPCLK preparation and gating;
- cancellation;
- safe cleanup;
- process and file lifetime; and
- removal and fault handling.

Physical addresses, DMA-channel numbers, raw control blocks, and unrestricted
register operations must not be exposed through the UAPI.

## 7. Reuse boundary with RP1

The following should be shared or aligned where practical:

- UAPI versioning conventions;
- bounded program structures;
- generation identifiers;
- completion states;
- terminal reasons;
- single-owner acquisition;
- cancellation semantics;
- route capability reporting;
- compatibility states;
- userspace provider abstraction;
- fail-closed backend selection; and
- diagnostics and support-bundle concepts.

The following must remain hardware-specific:

- register layouts;
- DMA address translation;
- DMA controller behavior;
- DMA request selection;
- PWM pacing;
- clock-provider behavior;
- pinctrl functions;
- cleanup ordering;
- supported kernel identities; and
- target qualification.

"Shared UAPI" must not mean "identical hardware assumptions."

## 8. Output-route contract

Only these routes are candidates:

```text
BCM_GPCLK_ROUTE_GPIO4  = 1
BCM_GPCLK_ROUTE_GPIO20 = 2
```

Requirements:

- arbitrary GPIO numbers are rejected;
- route identities are stable and versioned;
- only the selected pin is claimed;
- the inactive route remains safe and unclaimed where the kernel permits;
- persisted selection must match the bound driver capability;
- mismatches fail closed rather than silently selecting another pin;
- each route has independent compatibility and qualification state; and
- route switching must not occur during an active or draining transmission.

Whether route changes can be performed safely at runtime or require
administrative configuration and reprobe or reboot is a feasibility question.
Safety takes precedence over convenience.

## 9. Resource-ownership questions

The investigation must classify every resource as:

- kernel-enforced exclusive;
- arbitrated among cooperating drivers;
- detectable but racy;
- operator-policy only; or
- unprotected.

The analysis must cover:

| Resource | Required determination |
| --- | --- |
| DMA channel | Can DMAengine allocate a suitable channel exclusively? |
| DMA request | Can the required PWM DREQ be selected without bypassing ownership? |
| PWM peripheral | Can pacing be configured without disrupting audio or another PWM consumer? |
| PWM clock | Can its rate and ownership be managed through the clock framework? |
| GPCLK0 rate | Can exclusive-rate protection prevent cooperating changes? |
| GPCLK0 enable | What shared-reference limitations remain? |
| GPCLK0 divider | Can DMA target it safely while normal clock APIs are protected? |
| GPIO route | Can pinctrl claim only GPIO4 or GPIO20? |
| DMA program memory | Can coherent DMA allocation replace mailbox memory allocation? |
| Module/device lifetime | Can unbind, unload, and open descriptors be handled safely? |
| Direct-MMIO programs | What interference remains impossible to detect? |

## 10. Sound-driver conflict criterion

The study must not begin from the assumption that `snd_bcm2835` must remain
blacklisted.

It must establish:

- which exact DMA, PWM, clock, and pin resources the sound path uses;
- whether kernel arbitration exposes a real conflict;
- whether the proposed module can request resources and receive `-EBUSY`;
- whether sound can coexist when it does not use the required resources;
- whether disabling sound remains necessary on any platform; and
- whether other audio, PWM, overlay, HAT, or DMA users create equivalent
  conflicts.

If blacklisting remains necessary, it must be:

- platform-specific;
- evidence-based;
- explicit and reversible;
- applied only after operator acknowledgment; and
- insufficient by itself to claim safe coexistence.

## 11. Compatibility states

The backend must use these states:

- **Qualified:** Exact platform, kernel, architecture, route, module, modes,
  cleanup, timing, and RF evidence passed.
- **Experimental:** Clock-disabled gates passed and the administrator
  explicitly accepts unresolved coexistence risk.
- **Compatible-unqualified:** Build and identity checks pass, but live output
  is disabled.
- **Unavailable:** Required build, signing, API, resource, or device-tree
  contract is absent.
- **Rejected:** Known incompatibility, conflict, unsafe state, or validation
  failure exists.

A kernel update must demote the backend to `Compatible-unqualified` or
`Unavailable` unless the project's compatibility policy explicitly recognizes
it.

No failure may select `/dev/mem` or another physical backend automatically.

## 12. Phased execution

### Phase L1 - Source and kernel-interface feasibility

Research only:

- decompose the current backend;
- identify every direct register operation;
- map operations to DMAengine, pinctrl, common-clock, PWM, and DMA allocation
  APIs;
- compare Pi generations and kernel configurations;
- document unsupported assumptions; and
- define a candidate ownership model.

Exit gate: at least one credible stock-kernel design exists that could
materially reduce direct peripheral access.

### Phase L2 - Offline contract prototype

No target binding or hardware operation:

- define the route-neutral UAPI;
- extract or reuse the portable validator and lifecycle core;
- draft the module architecture;
- build against representative kernel headers;
- add static and portable tests; and
- define all partial-acquisition cleanup paths.

Exit gate: representative builds pass and no design depends on an unidentified
or unavailable kernel mechanism.

### Phase L3 - Clock-disabled target feasibility

Requires separately authorized administrative Pi access, but no GPIO or RF
output:

- bind the candidate module;
- acquire and release DMA, clock, PWM, and pinctrl resources;
- keep GPCLK unprepared or disabled as required;
- execute divider writes only with the output disconnected and disabled;
- test conflicts, process death, cancellation, unbind, unload, and recovery;
  and
- verify that unrelated resources remain unchanged.

Exit gate: all target resource, cleanup, and repeatability tests pass for each
candidate platform.

### Phase L4 - Controlled timing qualification

Requires separately authorized hardware operation:

- enable bounded output into an identified safe measurement chain;
- measure divider cadence, gating, timing, jitter, and cancellation;
- compare against the current backend;
- qualify GPIO4 and GPIO20 independently; and
- test each supported mode separately.

Exit gate: the module is at least behaviorally equivalent to the qualified
legacy backend within explicit limits.

### Phase L5 - Migration and packaging

- integrate DKMS source packaging;
- implement signing, update, downgrade, removal, and rollback behavior;
- preserve the legacy backend until migration evidence passes;
- define an explicit migration and fallback policy;
- update operator documentation; and
- requalify supported platform and mode combinations.

Exit gate: installation and migration do not weaken safety or strand supported
operators.

## 13. Required failure tests

At minimum:

- DMA channel unavailable;
- PWM already claimed;
- selected GPIO already claimed;
- GPCLK rate protected by another consumer;
- GPCLK already enabled;
- DMA allocation failure;
- partial program submission;
- cancellation during an active descriptor;
- userspace process death;
- file descriptor closed during execution;
- module removal requested while open or active;
- platform unbind;
- invalid UAPI version or size;
- invalid route;
- unsupported SoC;
- unsupported kernel;
- module-signing rejection;
- DKMS rebuild failure;
- stale callback after terminal completion; and
- cleanup or state-readback failure.

Every test must define the expected terminal state and the evidence required to
confirm it.

## 14. Safe terminal state

After success, cancellation, failure, close, unbind, or removal:

- no DMA descriptor remains active;
- no successor descriptor can start;
- PWM pacing is stopped;
- WsprryPi-owned DMA, PWM, clock, and pin resources are released;
- the selected GPIO is a defined safe input;
- GPCLK0 is not producing output through the selected pin;
- no stale generation can reactivate hardware;
- unrelated consumers' state has not been reset or restored incorrectly; and
- userspace receives an unambiguous terminal reason.

The module must restore only state it changed under valid ownership. It must
not overwrite another consumer's later changes with an old snapshot.

## 15. Evidence requirements

Each conclusion must identify:

- exact Pi model and revision;
- SoC;
- architecture;
- operating system;
- kernel package and version;
- kernel configuration;
- module source identity;
- UAPI version;
- selected GPIO route;
- mode and frequency;
- drive strength;
- commands or harness version;
- before, during, and after state;
- cleanup evidence;
- kernel warnings and faults; and
- exclusions and unqualified combinations.

Build evidence, clock-disabled evidence, timing evidence, and RF evidence must
remain separate.

## 16. Adversarial review

At the end of every phase, perform a separate adversarial assessment that
attempts to falsify:

- kernel API availability;
- claimed ownership strength;
- lack of conflicts;
- cleanup completeness;
- address translation;
- DMA termination behavior;
- clock and PWM coexistence;
- GPIO route isolation;
- module and file lifetime;
- DKMS update safety;
- behavioral equivalence; and
- qualification scope.

Every failure must be injected into the phase specification and the affected
work repeated. A phase passes only when the revised evidence supports every
claim.

## 17. Final decision

The final recommendation must be one of:

- **Adopt:** The DKMS module materially improves ownership and matches required
  behavior.
- **Adopt for selected platforms:** Only explicitly qualified SoC and kernel
  combinations migrate.
- **Retain as experimental:** The module is useful but cannot replace the
  legacy backend.
- **Reject:** Kernel interfaces cannot provide sufficient improvement or
  equivalent behavior.

The existing backend remains authoritative until the relevant adoption gate
passes. No repository or runtime change should be represented as migration
merely because a module compiles.
