# Issue 411 Slice 13: macOS machine-power boundary result

## Result

The Linux-only host reboot and power-off implementation now sits behind a typed
compile-time platform boundary. Linux retains direct `reboot(2)` requests with
`LINUX_REBOOT_CMD_RESTART` and `LINUX_REBOOT_CMD_POWER_OFF`, including the
existing filesystem synchronization, error diagnostics, and privilege model.

On macOS and other unsupported hosts, the selected implementation returns
`MachinePowerStatus::Unsupported` without a system call, synchronization call,
subprocess, external command, or runtime override. The scheduler emits a clear
operation-specific diagnostic after ordinary coordinated application cleanup,
and the caller no longer prints `Rebooting.` or `Shutting down.` when machine
power control is unavailable.

## Qualification

The focused hardware-free test passed on macOS. It checks both typed
unsupported results, compile-time source selection, retained Linux syscall
constants and direct calls, absence of command/syscall paths in the unsupported
implementation, caller capability guarding, cleanup ordering, and absence of a
production runtime bypass.

The required strict macOS build was run as:

```sh
make release \
  BACKENDS=si5351 \
  ANCILLARY_GPIO=0 \
  COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' \
  SUDO=
```

It passed the former `linux/reboot.h` blocker and stopped at the next unrelated
diagnostic:

```text
support_bundle_runtime.cpp:20:36: error: use of undeclared identifier 'getrandom'
```

The complete macOS application build is therefore not yet qualified. Per the
slice boundary, the Issue 414 support-bundle implementation and this newly
exposed portability defect were not changed.

A clean disposable Debian/GCC 14 container copy passed the focused test and the
strict `BACKENDS=si5351 ANCILLARY_GPIO=0` warnings-as-errors release build,
including compilation and linkage of the Linux machine-power implementation.
The scheduler's `selector-shutdown-cleanup-test` also passed under its normal
default backend profile. A broader strict-profile `semantics-test` attempt
reached execution but stopped at its GPIO estimate conflict assertion, and a
strict-profile attempt to run the selector cleanup regression correctly
rejected that test's request for its required `gpio` backend. Neither finding
belongs to this machine-power boundary. No machine power request or hardware
operation was executed.

## Product boundary

- Linux continues to support host reboot and power-off when the process has the
  required privilege.
- macOS deliberately does not support host reboot or power-off.
- Ordinary application shutdown, signal handling, runtime cleanup, thread
  joins, and process exit remain supported on macOS.
- The next macOS portability slice begins at the `getrandom` diagnostic above.

## Safety and remaining qualification

No reboot, power-off, installation, service operation, GPIO, I2C, transmitter,
test tone, or RF activity occurred. This is software portability evidence only;
CP2112 and physical Si5351 qualification remain pending adapter availability.
