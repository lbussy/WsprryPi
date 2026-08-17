# Issue 411 slice 5 execution prompt: strict I2C-only profile

## Objective

Add an explicit strict I2C-only build profile that compiles the Si5351
transmission path without requiring or linking libgpiod, and that rejects every
ancillary-GPIO request instead of silently ignoring it.

## Verified starting context

- Branch `codex/issue-411-slice-5-strict-i2c` starts from synchronized Slice 4
  commit `604aef2`.
- `BACKENDS=si5351` already excludes Raspberry Pi and RP1 transmission
  implementations, but common LED, amplifier, shutdown-button, band-selector,
  GPIO resolver, and libgpiod support remain.
- Ancillary GPIO is independent of the transmission-backend set and therefore
  needs its own explicit build capability.
- Backend-specific privilege relaxation is a separate security and operator
  workflow decision and remains deferred.

## Scope

1. Add a validated `ANCILLARY_GPIO=0|1` build setting, defaulting to `1`.
2. Include the setting in generated capabilities and profile artifact identity.
3. For `ANCILLARY_GPIO=0`, omit the real GPIO input/output/resolver
   implementations and all libgpiod compiler/linker discovery and flags.
4. Supply link-safe unavailable implementations for shared cleanup/control
   call sites; they must never access hardware.
5. Reject enabled LED, amplifier, shutdown-button, band GPIO, and per-frequency
   `@GPIO` configuration before runtime side effects.
6. Make help/version and `/version` disclose ancillary-GPIO availability.
7. Add build, behavior, dependency, and symbol regressions for the strict
   `BACKENDS=si5351 ANCILLARY_GPIO=0` profile.

## Required behavior

- The default build remains unchanged with ancillary GPIO compiled.
- A strict build succeeds without libgpiod development packages installed.
- The strict executable has no dynamic libgpiod dependency and no unresolved
  or retained `gpiod` API symbols.
- Disabled/default ancillary settings remain valid for compatibility.
- Any enabled ancillary GPIO selection fails with a stable unavailable-in-this-
  build diagnostic; configuration is not rewritten and no fallback occurs.
- `--help`, `--version`, and `/version` report whether ancillary GPIO is
  compiled.
- `--list-backends` retains its exact transmission-backend-only contract.

## Constraints and non-goals

- Do not relax the root requirement for physical backends.
- Do not alter Si5351 I2C transactions, output parking, cleanup, frequency
  planning, or RF behavior.
- Do not persist simulation or automatically select any backend.
- Do not change UI controls, installation, services, hardware state, GPIO,
  I2C, or RF output.
- Do not claim physical adapter, Si5351, frequency, or RF qualification.
- Use neutral `Related to #411` wording and do not change Issue 411.

## Validation

- Run generator and Make integration regressions for both ancillary settings
  and invalid values.
- Build and test the default profile with its normal libgpiod dependency.
- In a clean Ubuntu 24.04 x86_64 environment without libgpiod development
  packages, build `BACKENDS=si5351 ANCILLARY_GPIO=0` under GCC 13 and `-Werror`.
- Verify CLI reporting and rejection of LED, amplifier, shutdown, band GPIO,
  and `@GPIO` requests.
- Audit `ldd`, undefined symbols, and retained symbols for libgpiod/GPIO
  implementation leakage.
- Run safe Si5351 planner, transition, startup-quiesce, controller, and cleanup
  regressions where applicable.
- Parse workflow YAML and review final and staged diffs.

## Adversarial review

Attempt to disprove that the strict profile is isolated from cached default
objects, builds without libgpiod headers or libraries, rejects every ancillary
configuration entry point before activation, preserves default behavior, and
does not weaken the physical-backend privilege boundary. Correct findings and
repeat affected validation until clean.

## Exit criteria

- Strict Si5351 compilation has no libgpiod build or link dependency.
- Ancillary GPIO requests fail closed with explicit capability diagnostics.
- Capability reporting distinguishes transmission backends from ancillary
  GPIO support.
- Default profile regressions remain green.
- The slice is committed and pushed only on
  `codex/issue-411-slice-5-strict-i2c`.
