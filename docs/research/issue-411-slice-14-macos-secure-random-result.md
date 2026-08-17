# Issue 411 Slice 14: portable secure support-bundle job IDs

## Result

Support-bundle production job IDs now obtain their existing 16 random bytes
through OpenSSL `RAND_bytes` rather than Linux-only `getrandom`. The application
already requires and links OpenSSL `libcrypto`, so this removes an operating-
system-specific dependency without adding a library or runtime configuration.

The security and data contract is unchanged:

- exactly 128 random bits are requested;
- only an exact `RAND_bytes` return value of `1` is accepted;
- every other result throws `secure randomness unavailable` with no fallback;
- the output remains exactly 32 lowercase hexadecimal characters; and
- production paths, storage, collection, and test-dependency injection are
  unchanged.

This follows the [OpenSSL `RAND_bytes` contract](https://docs.openssl.org/3.4/man3/RAND_bytes/),
which defines it as a cryptographically secure generator, requires callers to
check the return value, and reports `1` only on success. It also matches the
narrow implementation independently qualified on the separate Issue 414
integration branch; no other Issue 414 work was imported.

The existing runtime test now constructs its private temporary directory from
the host's canonical temporary-directory path. This preserves its path-safety
assertions on macOS, where `/tmp` resolves to `/private/tmp`, without changing
production behavior.

## Qualification

On macOS, `support-bundle-runtime-test` passed with warnings as errors and
macOS-compatible linker flags. It exercised 128 production-generated IDs and
confirmed that each was unique, lowercase hexadecimal, and 32 characters long.
Source review confirmed that `support_bundle_runtime.cpp` contains the checked
`RAND_bytes(...) != 1` failure boundary and no `getrandom`, `<sys/random.h>`,
`errno`, weak fallback, or partial-output path.

The exact strict application build was then run:

```sh
make release \
  BACKENDS=si5351 \
  ANCILLARY_GPIO=0 \
  COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' \
  SUDO=
```

It passed compilation of the corrected support-bundle runtime and stopped at
the next unrelated platform diagnostic:

```text
Signal-Handler/src/signal_handler.cpp:337:19: error: use of undeclared identifier 'sigwaitinfo'
```

The complete macOS application build remains unqualified. The signal-handler
portability defect was not changed in this slice.

In a clean, unprivileged disposable Debian/GCC 14 environment:

- `support-bundle-runtime-test BACKENDS=si5351 ANCILLARY_GPIO=0 SUDO=` passed;
  and
- the strict warnings-as-errors `BACKENDS=si5351 ANCILLARY_GPIO=0` release build
  compiled and linked successfully.

## Safety and scope boundary

No production support-bundle path, collector, storage, endpoint, intake,
Dropbox, installer, service, GPIO, I2C, transmitter, test tone, or RF operation
was exercised. CP2112 and physical Si5351 qualification remain pending adapter
availability. The next macOS portability slice begins at `sigwaitinfo`.
