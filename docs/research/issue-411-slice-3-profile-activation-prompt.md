# Issue 411 slice 3 execution prompt: backend profile activation

## Objective

Activate truthful compile-time transmission-backend profiles using the
capability foundation from Slice 2. A selected profile must control linked
backend implementations and factory availability without changing ancillary
GPIO policy or silently substituting another backend.

## Verified starting context

- Branch `codex/issue-411-slice-3-profile-activation` starts from synchronized,
  pushed Slice 2 commit `9307563`.
- Slice 2 generates validated predicates for `rpi-gpio`, `rp1-gpclk`, `si5351`,
  and `simulated`, but intentionally rejects reduced profiles.
- `WsprTransmitter::createBackend()` and its backend includes are unconditional.
- The constructor unconditionally selects legacy Raspberry Pi GPIO before
  application configuration is parsed.
- Ancillary LED, amplifier, shutdown-button, and band-selector GPIO remains a
  separate libgpiod concern and is not removed by this slice.

## Scope

1. Convert the explicit source inventory into conditional backend source
   selection driven by validated `BACKENDS`.
2. Compile backend includes, factory constructors, and backend-specific helper
   calls only when their generated capability is enabled.
3. Make startup select the first compiled backend in canonical order rather
   than unconditionally constructing legacy GPIO.
4. Reject selection of an omitted backend with an explicit diagnostic that
   names the requested backend and lists the compiled backend set.
5. Add hardware-free regression coverage for all single-backend profiles,
   representative combinations, the default profile, invalid selections,
   profile switching without `clean`, and omitted implementation symbols.

## Required behavior

- Default `BACKENDS=rpi-gpio,rp1-gpclk,si5351,simulated` preserves current
  startup selection and runtime behavior.
- `BACKENDS=si5351` builds and links without legacy GPIO transmitter, Mailbox,
  RP1 GPCLK, or simulator implementation objects.
- `BACKENDS=simulated` builds without physical transmitter implementations.
- Every nonempty subset is structurally supported; test at least every singleton
  and representative multi-backend combinations in this slice.
- Selecting an omitted `BackendKind` throws; it never selects the first compiled
  backend as a runtime fallback.
- The initial compiled-backend choice constructs no hardware and performs no
  startup quiesce or transmission by itself.
- Existing CLI/configuration acceptance and capability reporting remain for a
  later slice; factory rejection is the enforcement boundary here.

## Constraints and non-goals

- Do not remove or condition ancillary libgpiod code or dependencies.
- Do not add automatic hardware detection or fallback.
- Do not change backend names, persisted configuration, CLI/UI controls,
  privilege policy, services, installation, hardware state, I2C, GPIO, or RF.
- Do not claim physical Si5351, GPIO, RP1, timing, or RF qualification.
- Preserve component boundaries and default all-backend compatibility.
- Use neutral `Related to #411` commit wording; do not change Issue 411.

## Validation

- Generator and Make integration regressions from Slice 2.
- Ubuntu 24.04 GCC 13 release builds for default, every singleton, and
  representative combinations, without intervening `make clean` where the
  switching contract is being tested.
- Symbol/object audit for `BACKENDS=si5351` and `BACKENDS=simulated` proving
  omitted transmitter implementations are not linked.
- A focused factory test proving compiled selections succeed and omitted
  selections fail with the compiled-backend list.
- Default debug build, `semantics-test`, and `startup-quiesce-parent-test`.
- YAML parse, final diff checks, staged-diff review, and clean pushed branch.

## Adversarial review

Attempt to disprove that:

- omitted sources cannot re-enter through recursive discovery;
- conditional headers or helper calls cannot create unresolved backend symbols;
- changing `BACKENDS` without cleaning cannot reuse a misleading executable;
- constructor selection and explicit runtime selection are distinct, with no
  fallback;
- default behavior remains legacy-GPIO-first;
- ancillary GPIO was not accidentally represented as removed; and
- test execution cannot touch transmitter hardware.

Correct actionable findings and repeat the affected checks until clean.

## Exit criteria

- Reduced backend executables are truthfully linked and factory-aware.
- The Si5351-only executable is buildable on ordinary Ubuntu without Raspberry
  Pi transmitter implementation objects.
- Runtime CLI/configuration reporting remains explicitly deferred.
- The slice is committed and pushed only on
  `codex/issue-411-slice-3-profile-activation` with evidence reported.
