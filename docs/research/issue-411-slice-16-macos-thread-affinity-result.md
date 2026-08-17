# Issue 411 Slice 16 result: macOS transmit-thread CPU affinity

## Outcome

The shared WSPR transmit-thread path no longer requires Linux CPU-affinity
types when compiled on macOS. Linux retains the same `cpu_set_t`, `CPU_ZERO`,
`CPU_SET`, `pthread_self()`, and `pthread_setaffinity_np()` operation. A failed
Linux affinity request retains its existing debug diagnostic and remains
non-fatal.

macOS now returns an explicit `Unsupported` result, emits a debug callback, and
continues without CPU pinning. It does not call Mach thread-policy APIs or
change scheduling priority, QoS, timing, or other host state. Unclassified
operating systems still fail at compile time rather than silently selecting a
fallback.

The separate Raspberry Pi backend watchdog-affinity implementation was not
changed.

## Implementation

- `src/WSPR-Transmitter/src/thread_affinity.hpp` defines the typed
  `Applied`/`Unsupported`/`Failed` result contract.
- `src/WSPR-Transmitter/src/thread_affinity.cpp` contains the Linux and macOS
  implementations behind compile-time platform selection.
- `src/WSPR-Transmitter/src/wspr_transmit.cpp` consumes the typed result while
  preserving Linux diagnostics and continuation behavior.
- The component and parent Makefiles exclude the focused test entry point from
  production source discovery.
- `thread_affinity_unavailable_test.cpp` and
  `thread_affinity_source_test.py` cover the unavailable result and source
  boundary without exercising a transmission.

## Validation

All validation was hardware-free. No transmission, test tone, RF, GPIO, I2C,
installation, service, power-control, or privileged scheduling operation was
performed.

### macOS host

- `make thread-affinity-test` in `src/WSPR-Transmitter/src`: passed.
- Strict parent build:
  `make -C src release BACKENDS=si5351 ANCILLARY_GPIO=0 COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' SUDO=`:
  compiled the macOS unavailable implementation and advanced past the former
  `cpu_set_t` error. It then stopped at the next unrelated warnings-as-errors:
  `busy_wait_until` and `add_ns` are unused on macOS.

The complete macOS executable is therefore not yet qualified by this slice.

### Unprivileged Debian/GCC container

After cleaning copied host build products:

- WSPR-Transmitter `make thread-affinity-test`: passed.
- WSPR-Transmitter `make transmission-controller-contract-test`: passed.
- Parent strict Si5351/GPIO-free release with
  `COMMON_FLAGS='-Wall -Werror -MMD -MP'`: passed.
- Parent `make selector-shutdown-cleanup-test SUDO=`: passed. Expected
  non-Raspberry-Pi device-tree and chrony diagnostics were present.

The Linux parent build compiled the real Linux affinity implementation; the
test-only forced-unavailable path did not replace it in production objects.

## Qualification boundary

This evidence covers compilation and software contracts only. It does not
qualify thread pinning under load, Raspberry Pi timing, GPIO, I2C/CP2112,
Si5351 electrical behavior or output, installation, services, RF, or a physical
transmitter chain.

## Documentation impact

The execution prompt and this result record were added to the repository.
Operator documentation was considered but is unchanged: this slice introduces
no operator setting or workflow and macOS remains an incomplete development
build target. Operator installation and CP2112 validation instructions remain
deferred until the candidate completes portability and physical-adapter
qualification.

## Next observed diagnostic

The next bounded portability slice is to classify the shared
`busy_wait_until()` and `add_ns()` helpers, which are compiled but unused in the
strict macOS Si5351-only profile. That work was not started here.
