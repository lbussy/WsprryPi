# Issue 414 Slice 5: Debian `age` Qualification Prompt

## Objective

Close the real-ciphertext qualification gate for the support-bundle artifact
primitive against Debian 13's packaged `age`, without creating or publishing a
production WsprryPi key or exposing encryption in the application workflow.

## Verified context

- Slice 2 implemented descriptor-pinned finalization, shell-free `age`
  encryption, ciphertext validation, hashing, and receipt publication using a
  deterministic test helper.
- Slice 4 ends with immutable finalized readable bytes.
- The production executable contract is `/usr/bin/age`, but no production
  recipient/key ID has been provisioned in the repository.
- The canonical non-hardware CI environment is Debian Trixie.

## Scope

1. Add a focused C++ integration fixture that:
   - creates and finalizes a known private readable archive;
   - encrypts it through `encrypt_support_bundle()` and the supplied real `age`
     executable/ephemeral X25519 recipient;
   - validates ciphertext filename, ownership, `0600` mode, size, and digest;
   - decrypts with the corresponding ephemeral identity using argv, never a
     shell;
   - proves the decrypted bytes exactly equal the finalized readable bytes;
   - writes and validates the bounded non-sensitive receipt; and
   - confirms no partial files or private identity content enter repository
     output.
2. Add a safe shell driver that generates an ephemeral identity in a private
   temporary directory, derives its public recipient, runs the fixture, and
   removes the temporary directory on every exit.
3. Add a dedicated Make target that is not part of ordinary tests and fails
   clearly when packaged `age`/`age-keygen` is unavailable.
4. Execute the target in an ephemeral Debian Trixie container with the packaged
   `age` dependency, recording exact version and evidence.
5. Record the qualification outcome and update the private-intake plan.

## Constraints and non-goals

- Use only an ephemeral test identity. Do not create, request, read, store,
  publish, back up, or rotate a production private key.
- Do not invent a production recipient, key ID, or fingerprint.
- Do not change installer dependencies, runtime construction, job manager,
  HTTP routes, UI, Dropbox, signed-manifest, GitHub, services, hardware, GPIO,
  I2C, transmitter, or RF behavior.
- Do not claim the application can encrypt until a separately reviewed public
  recipient is provisioned and lifecycle integration is implemented.
- Keep the fixture hardware-free, unprivileged, deterministic apart from the
  intentionally ephemeral age identity, and safe to repeat.

## Validation and evidence

- `bash -n` and ShellCheck for the new driver.
- Focused Make target with Debian packaged `age` and an exact-byte decrypt
  comparison.
- Existing `support-bundle-private-artifact-test` remains green.
- Confirm the generated identity exists only under the temporary directory and
  no key material appears in tracked/untracked repository files or command
  output.
- Run `git diff --check` and an independent adversarial review; correct all
  actionable findings before commit.

## Exit criteria

Stop after real Debian packaged-`age` round-trip qualification and durable
evidence. Commit and push only attributable Slice 5 files. The next slice must
provision the public production recipient/key metadata before enabling runtime
encryption.
