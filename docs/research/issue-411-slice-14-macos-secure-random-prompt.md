# Issue 411 Slice 14 execution prompt: portable secure support-bundle job IDs

## Objective

Replace the Linux-only support-bundle job-ID entropy call with the smallest
already-linked cryptographically secure API that works on Linux and macOS,
preserving the existing 128-bit lowercase-hex identifier and fail-closed
behavior. Continue the strict macOS build only through the next unrelated
diagnostic.

## Verified starting point

- Slice 13 is pushed at `9fa85ea33c9237856b478df0eb8f11a0b680462d`.
- The strict `BACKENDS=si5351 ANCILLARY_GPIO=0` macOS build now stops at
  `support_bundle_runtime.cpp:20:36: error: use of undeclared identifier
  'getrandom'`.
- Production currently fills 16 bytes through Linux `getrandom`, retries
  `EINTR`, rejects zero or errors, and encodes exactly 32 lowercase hex
  characters.
- The application already requires and links OpenSSL `libcrypto`.
- The separate Issue 414 integration branch independently qualified replacing
  this exact loop with checked OpenSSL `RAND_bytes` on Linux and macOS. No other
  Issue 414 commit or feature belongs in this slice.
- OpenSSL documents `RAND_bytes` as a CSPRNG operation that returns `1` on
  success and refuses output when secure entropy initialization fails; callers
  must check that return value.

## Required behavior

1. Use `RAND_bytes` to fill the existing 16-byte buffer and require an exact
   success result of `1`.
2. Preserve the existing `secure randomness unavailable` exception for every
   failure result. Never fall back to timestamps, process IDs, standard-library
   engines, weak randomness, partial bytes, or predictable identifiers.
3. Preserve the 32-character lowercase hexadecimal format and production/test
   dependency contracts.
4. Remove the Linux-only random header, `getrandom`, `errno`, and retry loop.
5. Make no support-bundle endpoint, intake, collector, storage, permission,
   installer, UI, or Issue 414 feature change.
6. Adjust only the runtime test's temporary-directory construction if required
   to run the existing security assertions truthfully on macOS.

## Qualification

- Run the support-bundle runtime test on macOS with warnings as errors and
  macOS-compatible linker flags; confirm 128 unique 32-character lowercase-hex
  production IDs.
- Inspect source to confirm the `RAND_bytes(...) != 1` fail-closed check and
  absence of `getrandom` and `<sys/random.h>`.
- Run the exact strict macOS Si5351 build used by Slice 13. Stop after success
  or the next exact unrelated compiler/linker diagnostic.
- Run the focused runtime test and strict Si5351 release build in a clean,
  unprivileged disposable Linux/GCC environment.
- Run `git diff --check`, inspect the entire diff, and adversarially check that
  the security, format, and Issue 414 boundaries remain intact.

## Safety and non-goals

Do not access support-bundle production storage or collectors; perform network
or Dropbox activity; install software; manage services; access GPIO, I2C, or
transmitter hardware; transmit; produce RF; merge the Issue 414 branch; weaken
warnings globally; or fix the next unrelated portability diagnostic.

## Exit criteria

Persist this prompt and an evidence-backed result, commit only attributable
files, push only `codex/issue-411-macos-secure-random`, and report the branch,
commit, validation, working tree, documentation impact, and next diagnostic.
