# Issue 411 Slice 15: macOS synchronous signal waiting result

## Result

The reusable Signal-Handler component now normalizes Linux and macOS
synchronous signal waiting behind one typed internal result:

- Linux compiles the existing `sigwaitinfo()` operation and classifies its
  `-1` result through `errno`;
- macOS compiles `sigwait()` and classifies its returned error number directly,
  without reading `errno` or requiring a `siginfo_t` payload; and
- other operating systems fail compilation rather than being silently treated
  as Linux or selecting a runtime fallback.

Both paths produce `Received`, `Interrupted`, or `Failed` with an explicit
signal or error number. The worker preserves the existing handled signal set,
calling-thread mask setup, worker mask reapplication, targeted `pthread_kill`
`SIGUSR1` wake, inline callback, and terminal restoration behavior.

An unexpected persistent wait failure now emits one diagnostic, marks the
worker failed, and exits instead of repeatedly invoking the wait operation.
`stop()` and destruction still join that failed worker safely without sending a
wake signal to a thread that has already exited. Repeated stop remains safe.

## Focused and lifecycle tests

The bounded component test now proves:

- Linux-style success and `EINTR` normalization;
- macOS-style success and a direct `EINVAL` failure while ambient `errno` is a
  different value;
- controlled `SIGTERM` delivery reaches the existing non-immediate callback;
- `stop()` wakes and joins the synchronous waiter;
- a second `stop()` returns safely;
- a second start/stop cycle on the same object remains safe;
- destruction stops and joins a blocked worker; and
- an injected persistent `EIO` failure invokes the waiter exactly once, exits,
  joins, and does not busy-loop.

A source-level platform-selection test confirms the Linux branch contains
`sigwaitinfo()` and `errno`, the Apple branch contains `sigwait()` and no
`sigwaitinfo` or `errno`, and unclassified hosts encounter a compile-time
error. It also confirms there is no environment-controlled selection.

## macOS validation

The standalone component passed with validation-only overrides for its
pre-existing Apple-clang limitations:

```sh
make clean
make portability-test \
  CXXFLAGS='-std=c++20 -Wall -Werror -MMD -MP' \
  LDFLAGS='-lpthread'
```

The overrides exclude the standalone Makefile's existing GCC-only
`-fmax-errors=10` and obsolete `-lstdc++fs` compile flags and Linux-only
`-latomic` link flag. Those unrelated component-build limitations were not
changed.

The exact parent strict build was run:

```sh
make -C src release \
  BACKENDS=si5351 \
  ANCILLARY_GPIO=0 \
  COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' \
  SUDO=
```

It compiled and linked the new Apple `sigwait()` seam into the parent object
graph, then stopped at the next unrelated portability diagnostic:

```text
WSPR-Transmitter/src/wspr_transmit.cpp:2112:9: error: unknown type name 'cpu_set_t'
```

The complete macOS application build therefore remains unqualified. CPU
affinity portability was not changed in this slice.

## Linux/GCC and parent validation

In a clean, unprivileged disposable Debian/GCC 14 environment:

- the standalone `make portability-test` passed using the compiled Linux
  `sigwaitinfo()` path;
- the strict warnings-as-errors Si5351/GPIO-free parent release compiled and
  linked successfully; and
- the normal-profile `selector-shutdown-cleanup-test` passed.

The test output includes one intentional `EIO` diagnostic from the injected
failure-path case. No destructive or terminal-affecting signal was sent; the
only test delivery was the established blocked `SIGTERM` to the test process.

## Safety and remaining work

No reboot, power-off, installation, service action, GPIO, I2C, transmitter,
test tone, or RF operation occurred. Machine-power and Issue 414 support-bundle
behavior were unchanged. The next macOS portability slice begins at the
`cpu_set_t` diagnostic above.
