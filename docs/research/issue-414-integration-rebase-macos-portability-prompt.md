# Issue 414 Integration Rebase and macOS Portability Prompt

## Objective

Rebase the Issue 414 integration branch onto current `devel`, reconcile the
Makefile intentionally, and qualify the two macOS portability limitations
recorded by Slice 30 before beginning the endpoint slice.

## Scope

1. Preserve current `devel` backend-profile and host-resource Make behavior.
2. Preserve every Issue 414 focused target while consolidating cumulative
   `.PHONY` declarations into maintainable groups.
3. Keep warnings as errors, but apply GCC-only diagnostic flags only when the
   selected compiler is not clang.
4. Replace Linux-only `getrandom` job-ID generation with an already-linked,
   cryptographically secure primitive supported on Linux and macOS.
5. Make host-specific linker/tool discovery deliberate where focused tests
   expose it: link `libatomic` only on Linux and resolve the installed
   `age-keygen` executable rather than assuming `/usr/bin`.
6. Run the Issue 414 focused suite and attempt a complete macOS release build.

## Constraints and non-goals

- Do not begin endpoint activation or add network behavior.
- Do not change installer, service, Raspberry Pi, GPIO, transmitter, or RF
  state.
- Do not weaken warnings-as-errors.
- Do not modify reusable components merely to make an unrelated complete
  macOS build warning disappear; report that boundary separately.
- Rewrite only the authorized Issue 414 integration branch, using a
  force-with-lease that protects the previously observed remote tip.

## Exit criteria

- The branch is rebased onto current `devel` without unresolved conflicts.
- Make target declarations are consolidated without losing targets.
- The GCC-only flag is absent under Apple clang.
- Secure production job IDs compile, link, and pass their runtime test on
  macOS.
- Focused Issue 414 tests pass, with skips and unrelated build limitations
  reported accurately.
- The final diff passes whitespace checks and the rebased branch is committed
  and pushed safely.
