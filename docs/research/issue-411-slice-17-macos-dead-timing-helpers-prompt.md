# Issue 411 Slice 17 execution prompt: remove dead shared timing helpers

## Objective

Remove the two unused timing-helper definitions that stop the strict macOS
Si5351/GPIO-free build, while preserving every active timing, scheduling, and
backend contract.

## Verified starting point

- Slice 16 is pushed at `8236f1a4af94966901acf0d93595898fbb601738`.
- Its strict macOS profile compiles the typed unavailable thread-affinity path
  and then stops because `busy_wait_until()` and `add_ns()` in the shared
  `wspr_transmit.cpp` anonymous namespace are unused under `-Werror`.
- Repository-wide reference inspection finds no caller for that
  `busy_wait_until()` definition and no caller for that shared `add_ns()`
  definition.
- The active shared transmit wait path uses `diff_ns()` directly.
- The Raspberry Pi and Si5351 backend translation units contain their own
  separate `add_ns()` implementations with active callers. Those backend-local
  implementations are outside this slice.
- The dead definitions arrived with the historical WSPR-Transmitter component
  import; current Git history contains no later active use to preserve.

## Required implementation

1. Delete only the unused shared `busy_wait_until()` and `add_ns()` definitions
   from `src/WSPR-Transmitter/src/wspr_transmit.cpp`.
2. Retain the shared `diff_ns()` helper and all of its active call sites.
3. Do not add platform conditionals, warning annotations, warning suppression,
   replacement helpers, or public interfaces for dead code.
4. Do not modify either backend-local `add_ns()` implementation or any timing,
   sleep, spin, event-offset, scheduling, priority, affinity, cancellation, or
   backend-selection behavior.

## Hardware-free validation

- Prove by repository-wide reference inspection that only the intended shared
  definitions are removed and active backend-local timing helpers remain.
- Run applicable standalone WSPR-Transmitter contract tests.
- Run the exact strict macOS Si5351/GPIO-free release and stop at success or the
  next unrelated compiler or linker diagnostic.
- In a clean unprivileged Linux/GCC environment, run the strict Si5351/GPIO-free
  release and applicable transmitter and parent cleanup regressions.
- Run `git diff --check` and adversarially review the complete diff for any
  active timing or backend change.

## Constraints

Do not execute transmission, test tones, RF, GPIO, I2C, machine power,
installation, services, or privileged scheduling. Do not alter prior Issue 411
slices, Issue 414 behavior, UI, operator settings, backend semantics, global
warning policy, or the next unrelated portability failure.

## Exit criteria

Persist an exact result, commit only attributable files, push only
`codex/issue-411-macos-dead-timing-helpers`, and report changed component files,
validation, limitations, documentation impact, clean remote state, and the next
observed diagnostic.
