# Issue 414 Slice 2: Local Artifact Primitives Prompt

## Objective

Implement and test the hardware-free local primitives needed to turn an already
validated readable support bundle into an immutable, encrypted support artifact
and a non-sensitive receipt. Keep these primitives behind typed C++ boundaries
so later HTTP and UI work cannot bypass their validation.

## Approved scope

- Generate and validate case IDs and artifact IDs using injected entropy.
- Validate the support context required before collection.
- Finalize an existing validated archive by confirming its exact size and
  SHA-256 and making it read-only to the service workflow.
- Invoke an `age`-compatible executable with argv, never a shell, using an
  exclusive private temporary file and atomic publication.
- Validate the encrypted output's type, owner, mode, size, and filename.
- Create a bounded UTF-8 JSON receipt without diagnostic or contact content.
- Add focused unit/integration tests and Makefile targets.

Do not integrate these primitives into HTTP routes, the job manager, collector,
installer, UI, Dropbox, GitHub, remote intake retrieval, or Ed25519 validation.
Do not modify the separate operator-documentation repository. Do not operate
hardware, services, GPIO, I2C, or RF.

## Required behavior

1. Case IDs use twelve Crockford Base32 symbols in `AAAA-BBBB-CCCC` form and
   encode 60 random bits. Artifact IDs use 128 random bits as 32 lowercase hex
   characters. Production entropy comes from the operating system; tests inject
   exact bytes and failures.
2. Support context accepts either a normalized WsprryPi GitHub issue URL, or a
   bounded meaningful description plus bounded user-approved contact value.
   Identity is not proven. New-issue and no-GitHub paths require both text fields.
3. Finalization opens the existing archive without following symlinks, verifies
   it is a private regular file owned by the service user, computes SHA-256 from
   the open descriptor, rejects mutation, and changes the mode to `0400`.
4. Encryption accepts only a finalized archive descriptor/record, fixed safe
   job directory, validated case/artifact IDs, key ID, recipient, and executable.
   Production construction uses `/usr/bin/age`; a typed executable override is
   available only to tests.
5. The child receives exactly `--encrypt --recipient RECIPIENT --output TEMP
   ARCHIVE`. It has no shell, redirects output to `/dev/null`, and must exit zero.
6. Temporary output is exclusive, mode `0600`, a direct child of the job
   directory, and removed after every failure. Existing final output is never
   overwritten. Publication is atomic.
7. Success requires a non-empty bounded regular ciphertext owned by the service
   user with mode `0600`. Return its exact size and lowercase SHA-256.
8. Receipt serialization is deterministic JSON and contains only protocol
   correlation, filenames, byte counts, hashes, key ID, optional normalized
   issue URL, and local upload state. It must reject unsafe names and invalid
   identifiers/digests before writing through an exclusive temporary file and
   atomic rename.
9. Failures preserve the finalized readable archive and never produce a
   plaintext fallback.

## Validation

- Deterministic identifier generation and malformed-input tests.
- Support-context kind, URL, blank-text, control-character, and size tests.
- Finalization tests for digest, size, mutation, symlink, ownership/mode, and
  read-only publication.
- Encryption tests for argv fidelity, exit failure, empty/oversized output,
  wrong mode, collision, partial cleanup, exact-byte input, retry, and digest.
- Receipt schema, deterministic serialization, unsafe value, collision, and
  cleanup tests.
- Run all new focused targets plus the existing support-bundle regression
  targets affected by shared build wiring.
- Run `git diff --check` and perform an adversarial safety review. Correct every
  actionable finding and repeat the relevant checks.

If the host lacks the real Debian-packaged `age`, use a deterministic test-only
helper to exercise the process and file-safety contract, report the missing real
round-trip gate explicitly, and do not claim encryption-format qualification.

## Exit criteria

Stop with local primitives and their tests. Do not begin job-state, HTTP, UI,
signed-intake, Dropbox, installer, or maintainer-tooling integration. Commit and
push only the attributable Slice 2 files after staged-diff review.
