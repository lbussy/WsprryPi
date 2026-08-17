# Issue 414 Integration Rebase and macOS Portability

## Outcome

The Issue 414 integration branch was rebased onto `devel` at `bb4311d2`. The
Makefile conflict was resolved by retaining devel's host-aware build sizing and
backend-profile targets, retaining every Issue 414 target, and replacing the
slice-by-slice cumulative `.PHONY` declarations with functional groups.

The two Slice 30 macOS limitations are resolved:

- compiler detection keeps `-Wall -Werror` for every compiler while applying
  GCC's `-fmax-errors=10` only when the selected compiler is not clang; and
- production support-bundle job IDs now use OpenSSL `RAND_bytes` for 128 bits
  of cryptographically secure randomness instead of Linux-only `getrandom`.

The Make reconciliation also makes `-latomic` Linux-only and resolves the real
installed `age-keygen` path before the provisioning qualification. The runtime
test now constructs its private temporary root from a canonical host temporary
directory, preserving the production canonical-path safety rule on macOS.

## Qualification

On the macOS development host:

- the release build advanced past the former unused `-fmax-errors=10` failure;
- `support_bundle_runtime.cpp` compiled with `RAND_bytes`;
- `support-bundle-runtime-test` linked without Linux `libatomic` and passed,
  exercising 128 unique, lowercase, 32-character production job IDs; and
- all focused Issue 414 intake tests passed. The real age round-trip and real
  age-key provisioning checks ran against the installed Homebrew executables.

Some signing/manifest Python suites retained their intentional environment-
dependent OpenSSL skips when a separately requested real OpenSSL executable
was not supplied.

The complete application release build remains unqualified on macOS. After
passing both resolved Slice 30 failures, clang stopped on an existing
`-Wpessimizing-move` warning in `src/LCBLog/src/lcblog.tpp`, promoted to an
error by the repository's warnings-as-errors policy. This integration work did
not modify that reusable component or suppress the warning. Debian remains the
canonical complete non-hardware build environment until that separate
component portability issue is authorized and corrected.

## Boundary

No endpoint slice, network request, production state, Dropbox, Keychain,
installer, service, Raspberry Pi, GPIO, transmitter, or RF operation was
performed.
