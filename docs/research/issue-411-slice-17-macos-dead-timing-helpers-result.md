# Issue 411 Slice 17 result: remove dead shared timing helpers

## Outcome

The unused shared `busy_wait_until()` and `add_ns()` definitions were removed
from `wspr_transmit.cpp`. Repository-wide reference inspection confirmed that
neither definition had a caller. The active shared `diff_ns()` helper and both
backend-local `add_ns()` implementations remain unchanged.

No platform guard or warning suppression was introduced, and no active timing,
scheduling, sleep, spin, event-offset, priority, affinity, cancellation, or
backend-selection behavior changed.

## Validation

All validation was hardware-free. No transmission, test tone, RF, GPIO, I2C,
installation, service, power-control, or privileged scheduling operation was
performed.

### Reference inspection

- `busy_wait_until` has no remaining definition or reference under
  `src/WSPR-Transmitter/src`.
- The shared `diff_ns()` definition retains active wait-path callers in
  `wspr_transmit.cpp`.
- The Raspberry Pi and Si5351 backend-local `add_ns()` definitions and their
  active callers remain present and unchanged.

### macOS host

- WSPR-Transmitter `make thread-affinity-test`: passed.
- WSPR-Transmitter `make transmission-controller-contract-test`: not runnable
  with the existing component Makefile on macOS because `-lstdc++fs` is passed
  during compilation and Clang reports it as an unused linker argument under
  `-Werror`. This pre-existing build-system diagnostic is unrelated to the
  changed source.
- Strict parent build:
  `make -C src release BACKENDS=si5351 ANCILLARY_GPIO=0 COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' SUDO=`:
  compiled `wspr_transmit.cpp` successfully and reached the next unrelated
  diagnostic in `si5351_device.cpp`: `<linux/i2c-dev.h>` is unavailable on
  macOS.

The complete macOS executable is therefore not yet qualified by this slice.

### Unprivileged Debian/GCC container

After copying the checkout and cleaning host build products:

- WSPR-Transmitter `make thread-affinity-test`: passed.
- WSPR-Transmitter `make transmission-controller-contract-test`: passed.
- Parent strict Si5351/GPIO-free release with
  `COMMON_FLAGS='-Wall -Werror -MMD -MP'`: passed.
- Parent `make selector-shutdown-cleanup-test SUDO=`: passed. Expected
  non-Raspberry-Pi device-tree and chrony diagnostics were present.

## Qualification boundary

This evidence covers compilation and software contracts only. It does not
qualify Raspberry Pi timing, GPIO, I2C/CP2112, Si5351 electrical behavior or
output, installation, services, RF, or a physical transmitter chain.

## Documentation impact

The execution prompt and this result record were added to the repository.
Operator documentation was considered but is unchanged because removal of
unreachable internal helpers creates no operator setting or workflow. macOS
remains an incomplete development build target, and physical CP2112
instructions remain deferred until a buildable candidate and adapter are
available for qualification.

## Next observed diagnostic

The next bounded Issue 411 slice is the Si5351 I2C transport boundary:
`si5351_device.cpp` directly depends on Linux `<linux/i2c-dev.h>` and ioctl
semantics. That work requires inspecting the existing device contract and the
CP2112 userspace interface before choosing an implementation; it was not
started here.
