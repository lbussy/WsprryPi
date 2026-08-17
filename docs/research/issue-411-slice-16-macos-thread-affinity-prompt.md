# Issue 411 Slice 16 execution prompt: macOS transmit-thread CPU affinity

## Objective

Remove the Linux-only CPU-affinity compilation dependency from the shared
WSPR-Transmitter path while preserving Linux transmit-thread pinning and its
existing non-fatal diagnostics exactly. On macOS, report that affinity is
unsupported and continue without CPU pinning; do not emulate it with Mach APIs.

## Verified starting point

- Slice 15 is pushed at `a2b3a644491268a67eab26cfec9dfa9b2697e789`.
- Its strict macOS Si5351/GPIO-free build compiles the Apple `sigwait()` path
  and stops at `wspr_transmit.cpp:2112:9: error: unknown type name 'cpu_set_t'`.
- The shared transmit-thread entry path attempts affinity only when more than
  one online CPU is present. Linux builds a `cpu_set_t`, selects the configured
  CPU, calls `pthread_setaffinity_np()` for the current thread, logs a debug
  failure, and continues.
- CPU affinity is an optional jitter-reduction measure, not a transmission
  start gate. macOS does not provide Linux CPU sets or
  `pthread_setaffinity_np()`.
- The Raspberry Pi backend has a separate watchdog affinity implementation. It
  is excluded from the strict Si5351 profile and is not part of this shared
  transmit-thread slice.

## Required implementation

1. Add a small typed component boundary returning `Applied`, `Unsupported`, or
   `Failed` plus an explicit error number.
2. Compile the existing Linux `cpu_set_t`, `CPU_ZERO`, `CPU_SET`,
   `pthread_self`, and `pthread_setaffinity_np` sequence only for `__linux__`.
3. Preserve Linux CPU selection, error-number handling, debug wording, and
   non-fatal continuation.
4. On `__APPLE__`, return `Unsupported` without changing thread policy,
   scheduling priority, affinity, QoS, or other host state. Emit one truthful
   debug callback and continue without pinning.
5. Fail compilation for other unclassified operating systems. Provide no
   runtime fallback, CLI, INI, environment, query, or UI control.
6. Do not alter CPU counting, configured CPU clamping, spin timing, priority,
   backend selection, transmission planning, or the separate Raspberry Pi
   watchdog affinity path.

## Hardware-free tests and qualification

- Compile and execute a forced-unavailable test proving the macOS result has no
  side effect and returns `Unsupported` with no error.
- Source-check Linux's exact affinity primitives, their absence from the Apple
  branch, explicit unsupported-host failure, and absence of environment
  selection.
- Run applicable standalone WSPR-Transmitter tests without transmission or
  hardware activity.
- Run the exact strict macOS parent release and stop at success or the next
  unrelated diagnostic.
- In a clean unprivileged Linux/GCC environment, run the focused test, strict
  Si5351/GPIO-free warnings-as-errors release, and applicable transmitter and
  parent cleanup regressions.
- Run `git diff --check` and adversarially review the full component and parent
  diff for accidental affinity calls, timing changes, or warning suppression.

## Constraints

Do not execute transmission, test tones, RF, GPIO, I2C, machine power,
installation, services, or privileged scheduling. Do not modify the Raspberry
Pi watchdog affinity path, prior signal/machine-power work, Issue 414 behavior,
backend semantics, global warnings, UI, or the next unrelated portability
failure.

## Exit criteria

Persist an exact result, commit only attributable files, push only
`codex/issue-411-macos-thread-affinity`, and report component files, validation,
limitations, documentation impact, clean remote state, and the next diagnostic.
