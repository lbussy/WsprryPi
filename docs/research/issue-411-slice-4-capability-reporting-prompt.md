# Issue 411 slice 4 execution prompt: capability reporting and validation

## Objective

Make compiled transmission capabilities visible and enforce them before CLI or
persisted configuration can activate an omitted backend.

## Verified starting context

- Branch `codex/issue-411-slice-4-capability-reporting` starts from synchronized
  Slice 3 commit `b0c4777`.
- `BACKENDS` already controls linked objects and factory availability.
- CLI accepts `gpio`, `si5351`, and transient `simulated`; `gpio` maps to RP1
  GPCLK on Raspberry Pi 5 and legacy GPIO elsewhere.
- Persisted configuration accepts `gpio` and `si5351`; simulation remains
  intentionally transient.
- Help/version and `/version` do not yet report compiled capabilities.

## Scope

1. Add one shared runtime query for the canonical compiled-backend string.
2. Add `--list-backends` as an early, hardware-free command.
3. Include compiled backends in `--help` and `--version` output.
4. Add a machine-readable compiled-backends field to `/version`.
5. Reject valid-but-omitted CLI and persisted backend selections with a
   diagnostic distinct from invalid backend spelling.
6. Validate the final configuration candidate before runtime side effects.
7. Extend profile CI with reporting and rejection checks.

## Required behavior

- `--list-backends` prints the canonical build list and exits successfully.
- Unknown backend names retain the existing invalid-name diagnostic.
- A known omitted backend reports that it is unavailable in this build and
  lists compiled backends.
- `gpio` availability follows runtime mapping: RP1 capability is required on
  Raspberry Pi 5; legacy GPIO capability is required elsewhere.
- Existing configurations are never rewritten or normalized to an available
  backend, and there is no fallback.
- Help/version/list operations perform no hardware probing or transmission.
- The default all-backend build remains behavior-compatible.

## Constraints and non-goals

- Do not implement strict ancillary-GPIO removal or backend-specific privilege
  relaxation.
- Do not change persisted backend names or make simulation persistable.
- Do not change UI controls, services, installation, hardware state, I2C,
  GPIO, or RF behavior.
- Do not claim physical-device qualification.
- Use neutral `Related to #411` commit wording and do not change Issue 411.

## Validation

- For every singleton and default profile, verify `--list-backends`, `--help`,
  and `--version` report the exact compiled set.
- Verify a known omitted CLI backend fails with the unavailable diagnostic and
  an unknown backend retains the invalid-name diagnostic.
- Verify persisted known-but-omitted selection is rejected without mutation or
  fallback using hardware-independent configuration tests.
- Run default release/debug, semantics, startup-quiesce, capability generator,
  Make integration, and factory tests on Ubuntu 24.04/GCC 13.
- Parse workflow YAML and review final/staged diffs.

## Adversarial review

Attempt to disprove that early commands are hardware-free, config validation
precedes side effects, Pi 5 selects the correct compiled GPIO capability,
unknown and omitted errors remain distinct, simulation remains transient, and
the `/version` field is stable and machine-readable. Correct findings and
repeat affected validation until clean.

## Exit criteria

- Compiled capabilities are visible through CLI and `/version`.
- CLI and persisted omitted selections fail closed before activation.
- Default behavior and existing configuration names remain compatible.
- The slice is committed and pushed only on
  `codex/issue-411-slice-4-capability-reporting`.
