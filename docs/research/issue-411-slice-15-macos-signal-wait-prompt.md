# Issue 411 Slice 15 execution prompt: macOS synchronous signal waiting

## Objective

Make the reusable Signal-Handler component compile and behave correctly on
macOS while preserving Linux's `sigwaitinfo()` signal-number, mask, wake,
callback, diagnostic, and lifecycle behavior.

## Verified starting point

- Current `devel` is `cca83839dc24722aa9890e20a56289f9a29426be`.
- The strict `BACKENDS=si5351 ANCILLARY_GPIO=0` macOS build compiles the prior
  machine-power and secure-randomness corrections, then fails at
  `Signal-Handler/src/signal_handler.cpp:337:19: error: use of undeclared
  identifier 'sigwaitinfo'`.
- Linux provides `sigwaitinfo()`. macOS provides POSIX `sigwait()`, whose error
  number is its return value rather than `errno`.
- `stop()` directs the already-blocked `SIGUSR1` wake signal to the worker with
  `pthread_kill`, then joins it. The signal set and ownership must not change.

## Required implementation

1. Add a small typed internal synchronous-wait result and compile-time wrapper.
2. On Linux call `sigwaitinfo()`, preserve its received signal number, classify
   `EINTR` from `errno`, and preserve all handled signals and mask ownership.
3. On macOS call `sigwait()`, consume its direct error return, and never consult
   `errno`, `siginfo_t`, or Linux-only APIs in that compiled branch.
4. Normalize received, interrupted, and failed results once. Do not introduce
   runtime fallback, CLI, INI, environment, query, or UI selection.
5. Preserve the targeted `pthread_kill` wake, prompt stop/join, repeated-stop
   safety, destruction, terminal restoration, and inline callback behavior.
6. Make an unexpected persistent wait failure exit the worker rather than
   busy-loop, while preserving later joining and object lifetime safety.
7. Keep changes inside the reusable component plus necessary parent-facing
   tests and research records. Do not modernize unrelated component code.

## Hardware-free tests

- Prove source selection uses `sigwaitinfo()` only for `__linux__`, `sigwait()`
  only for `__APPLE__`, and fails compilation for unclassified platforms.
- Deterministically prove both API return conventions normalize correctly,
  including a macOS-style error that differs from ambient `errno`.
- Exercise a bounded handled `SIGTERM` callback, targeted stop wakeup, repeated
  stop, destructor join, and one injected persistent failure that must produce
  exactly one wait call rather than a tight loop.
- Run the component standalone with validation-only Apple-clang overrides for
  its pre-existing `-fmax-errors=10`, `-lstdc++fs`, and `-latomic` limitations.
- Run relevant parent shutdown tests and the exact strict macOS release build,
  stopping at the next unrelated diagnostic.
- Run the same component and applicable parent validation in a clean,
  unprivileged Linux/GCC environment.

## Constraints

Do not change production signal selection, process shutdown, machine power,
Issue 414 support bundles, backend profiles, warnings policy, UI, installation,
services, GPIO, transmitter hardware, or RF. Tests must not send destructive or
terminal-affecting signals and must remain bounded.

## Completion contract

Correct all actionable race, lifecycle, error-convention, platform-selection,
and busy-loop findings. Persist an exact result, inspect the complete component
and parent diff, commit only attributable work, push only
`codex/issue-411-macos-signal-wait`, and report all validation and limitations.
