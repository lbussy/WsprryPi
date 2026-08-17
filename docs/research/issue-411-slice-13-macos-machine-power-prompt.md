# Issue 411 Slice 13 execution prompt: disable machine power control on macOS

## Objective

Make the strict Si5351 profile compile truthfully on macOS by treating host
reboot and power-off as unavailable, while preserving the existing Linux
direct-`reboot(2)` behavior exactly.

## Verified starting point

- Issue 411's compile-time backend profiles are merged into `devel` at
  `bb4311d2d51f42f32a20381f94e0275a8d2da2b4`.
- The hardware-reduced profile is `BACKENDS=si5351 ANCILLARY_GPIO=0`.
- With the documented Apple-clang-only handling for the pre-existing LCBLog
  pessimizing-move warning, the macOS build reaches
  `scheduling.cpp:91:10: fatal error: 'linux/reboot.h' file not found`.
- `scheduling.cpp` directly contains both Linux reboot headers and the reboot
  and power-off syscall implementations.

## Required implementation

1. Add one small typed compile-time platform boundary for machine reboot and
   power-off rather than scattering conditional compilation.
2. On Linux, retain filesystem synchronization, the direct `reboot(2)` calls,
   `LINUX_REBOOT_CMD_RESTART`, `LINUX_REBOOT_CMD_POWER_OFF`, privilege
   behavior, sequencing, diagnostics, and error handling.
3. On macOS and other unsupported hosts, compile no Linux reboot header or
   constant and perform no reboot syscall, command, subprocess, privilege
   escalation, or emulation. Return an explicit unsupported result and emit a
   clear operation-specific diagnostic.
4. Keep ordinary application shutdown, signal handling, scheduler cleanup,
   thread joins, and process exit distinct and unchanged. Do not log that the
   machine is rebooting or powering off on an unsupported host.
5. Add no CLI, INI, environment, query-string, UI, or other runtime bypass.

## Hardware-free tests

Prove that Linux retains both direct syscall constants; the unsupported source
contains no Linux headers, syscall, synchronization call, or external-command
path; both unsupported operations return the typed unavailable result; caller
logging respects the capability; coordinated cleanup precedes machine-power
requests; and no production bypass was introduced. Run the relevant existing
shutdown/cleanup regression without ever requesting machine power.

## Qualification

From `src`, run the focused test and this exact macOS build:

```sh
make release \
  BACKENDS=si5351 \
  ANCILLARY_GPIO=0 \
  COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' \
  SUDO=
```

Continue only until it succeeds or reaches the next exact unrelated diagnostic.
Also run an applicable Linux/GCC hardware-free strict build and cleanup suite,
then `git diff --check` and a complete diff review.

## Constraints

Do not reboot or power off any host; install software; manage services; access
GPIO, I2C, transmitter hardware, or RF; change Issue 414 support-bundle
behavior; alter backend-selection semantics; weaken warnings globally; suppress
unrelated diagnostics; redesign scheduling or UI; or fix the next portability
blocker exposed by this slice.

## Completion contract

Record Linux and macOS platform behavior, the exact qualification result, and
the next blocker. Correct actionable adversarial findings, commit only this
slice, push only its Issue 411 branch, update Issue 411 without changing its
status, and report the commit, branch, push, worktree, validation, documentation
impact, and remaining qualification honestly.
