# Issue 411 Slice 1 implementation prompt

Implement only the common Ubuntu/GCC portability correction identified in
`compile-time-transmission-backend-selection.md`.

## Scope

1. Correct the ignored `write()` result in the async-shutdown notification path
   without weakening `-Werror`.
2. Preserve async-signal safety. The signal path must not log, allocate, throw,
   block, or retry.
3. Add a checked-in native Ubuntu 24.04/GCC 13 release-build regression that
   compiles the real parent executable with the normal release flags.
4. Run safe, non-hardware validation and report any environment limitations.

## Constraints

- Do not implement compile-time backend profiles in this slice.
- Do not alter backend selection, configuration, persistence, scheduling,
  transmitter lifecycle, UI, installation, services, GPIO, I2C, or RF behavior.
- Do not suppress `unused-result`, remove `-Werror`, or use a diagnostic-only
  compiler workaround as the production correction.
- Keep Issue 411 status unchanged. Use neutral `Related to #411` wording if the
  change is committed.

## Acceptance criteria

- The fortified `write()` result is consumed explicitly.
- The handler remains limited to async-signal-safe operations.
- The ordinary debug and release builds remain covered.
- Native Ubuntu 24.04 verifies GCC major version 13 and builds the release
  executable under the repository's normal `-Werror` policy.
- Existing hardware-free regressions remain green.
- No hardware, installation, service, GPIO, I2C, or RF activity occurs.
