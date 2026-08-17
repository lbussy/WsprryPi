# Issue 414 Slice 6: Bundle-Key Provisioning Tooling Prompt

## Objective

Create and qualify maintainer-only tooling for provisioning the project-specific
WsprryPi `age` X25519 bundle-encryption key pair and deterministic public
metadata, while keeping all real private material outside the repository and
leaving production provisioning as an explicit maintainer action.

## Verified context

- Bundle key IDs are `wsprrypi-bundle-YYYY-NN`.
- Slice 5 qualified Debian packaged `age` 1.2.1 with ephemeral X25519 keys.
- The application does not yet contain a production recipient or key ID.
- Bundle encryption and future intake-manifest signing require separate keys.
- The private identity must be retained by the maintainer and backed up in the
  password vault; Codex has no authorized password-vault destination in this
  slice.

## Scope

1. Add a maintainer-only provisioning command that:
   - requires an absolute fixed `age-keygen` executable;
   - validates a bundle key ID and an absolute owner-only private directory;
   - refuses symlinks, unsafe permissions, collisions, and existing outputs;
   - creates the identity through `age-keygen` without exposing secret content
     in argv, stdout, stderr, logs, or public metadata;
   - derives the X25519 public recipient with `age-keygen -y`;
   - publishes the private identity as owner-read-only `0400` without overwrite;
   - publishes bounded deterministic public JSON without overwrite; and
   - removes partial outputs on every failure.
2. Define public metadata version 1 with project, purpose, algorithm, key ID,
   recipient, creation time, and fingerprint. The fingerprint is lowercase
   SHA-256 of the exact ASCII recipient bytes, without a trailing newline.
3. Add tests for success, permissions, fingerprint, private non-disclosure,
   collision refusal, unsafe private directories, invalid IDs, invalid
   recipients, tool failure, and partial cleanup.
4. Execute an ephemeral real-`age-keygen` provisioning test in Debian Trixie.
5. Record the boundary and a precise manual handoff for production provisioning.

## Constraints and non-goals

- Do not generate the actual production WsprryPi identity in this slice.
- Do not place any private identity, recovery copy, password-vault export, or
  secret content in the repository, application, CI artifact, command output,
  logs, Dropbox, or documentation.
- Do not automate macOS Keychain or a password vault without a separately
  selected storage design and destination.
- Do not commit public metadata produced from an ephemeral test key.
- Do not change runtime encryption, installer dependencies, HTTP, UI, signed
  intake, Dropbox, services, hardware, GPIO, I2C, transmitter, or RF behavior.
- Public metadata alone does not authorize or activate encryption.

## Validation

- Python tests and static syntax validation.
- Debian Trixie real-`age-keygen` success with private `0400` identity, exact
  public recipient, independently recomputed fingerprint, and no secret in JSON
  or captured output.
- Adversarial collision/failure cleanup checks.
- Confirm no `AGE-SECRET-KEY-` value exists in any repository output.
- `git diff --check` and independent adversarial review, correcting all
  actionable findings.

## Exit criteria

Stop with qualified tooling and documentation. Commit and push only attributable
Slice 6 files. Report that the next action requires the maintainer to select the
final private-storage and password-vault backup destinations before running the
tool for `wsprrypi-bundle-2026-01`.
