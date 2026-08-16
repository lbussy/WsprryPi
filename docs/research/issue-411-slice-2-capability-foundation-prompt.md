# Issue 411 slice 2 execution prompt: backend capability foundation

## Objective

Establish the build-system foundation for compile-time transmission-backend
profiles without claiming that reduced executables are implemented yet.

## Verified starting context

- Branch `codex/issue-411-slice-2-capabilities` is based on current
  `origin/devel` and contains the completed Ubuntu GCC 13 portability slice.
- The parent Makefile recursively discovers all production transmitter sources.
- Backend construction and initial selection remain unconditional in
  `WsprTransmitter`; changing those runtime contracts belongs to the next slice.
- The supported backend names are `rpi-gpio`, `rp1-gpclk`, `si5351`, and
  `simulated`.
- The default executable must continue to contain all four backends.

## Scope

1. Add one deterministic generator for a C++ capability header.
2. Validate backend names, reject empty selections and duplicates, and emit a
   canonical order independent of input order.
3. Wire the generated header into normal Make builds and object freshness.
4. Replace implicit discovery of production backend implementations with
   explicit per-backend source groups while continuing to link every group in
   the default build.
5. Add hardware-free regression coverage for generation, validation,
   no-rewrite behavior, and Make integration.
6. Run the regression in Debian non-hardware CI.

## Constraints and safety contract

- Preserve the existing default build and runtime behavior.
- Do not conditionally omit backend objects in this slice.
- Reject any non-default `BACKENDS` value at the parent Make boundary with a
  clear message explaining that reduced profiles are not enabled yet. This
  prevents a generated capability declaration from misrepresenting the linked
  executable.
- Do not change backend factories, initial selection, CLI/configuration
  validation, privilege policy, ancillary libgpiod behavior, UI, installation,
  services, hardware state, I2C devices, GPIO, or RF output.
- Use neutral `Related to #411` commit wording and do not change Issue 411.

## Requirements

- Default Make value:
  `BACKENDS=rpi-gpio,rp1-gpclk,si5351,simulated`.
- Generated header path:
  `src/build/generated/backend_capabilities.hpp`.
- Stable generated predicates for every supported backend plus the canonical
  backend list.
- Atomic replace-if-changed output so repeated builds do not rebuild objects.
- Read-only `--check` support suitable for Make dry-run/query handling.
- Every normal production release/debug object depends on the generated header.
- Source inventory names shared transmitter sources and the four backend source
  groups explicitly; test and qualification sources remain excluded.

## Validation and evidence

- Generator regression: default, reordered input, selected subsets, duplicate,
  unknown, empty, check/current/stale, and unchanged-file timestamp.
- Make integration regression: default capability header is generated;
  unchanged rerun does not rebuild; generator changes invalidate objects; and a
  non-default parent build fails before compilation.
- Parent `make release SUDO=` and `make debug SUDO=` on a suitable Linux build
  host or the established Ubuntu 24.04 container.
- Existing hardware-free semantics and startup-quiesce parent tests.
- YAML parse and final `git diff --check` / staged-diff review.

## Adversarial review

Before committing, attempt to disprove:

- that the header describes the actually linked default executable;
- that a non-default value can accidentally produce a misleading executable;
- that every backend implementation is accounted for by an explicit group;
- that switching generated selections can leave stale content;
- that normal repeated builds recompile because the generator rewrites an
  unchanged header; and
- that tests or CI can touch transmitter hardware.

Correct actionable findings and repeat the relevant checks until clean.

## Exit criteria

- The default all-backend executable remains buildable and behavior-compatible.
- Capability generation and explicit source grouping are implemented and tested.
- Reduced executable profiles remain explicitly unavailable, not partially or
  misleadingly implemented.
- Changes are committed and pushed only on
  `codex/issue-411-slice-2-capabilities`.
- The report separates completed foundation work, deferred profile activation,
  documentation impact, hardware qualification not performed, and repository
  state.
