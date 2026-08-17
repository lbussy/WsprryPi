# Issue 414 Slice 13 Execution Prompt: Intake-Signing Key Provisioning

## Objective

Add a maintainer-only, hardware-free tool that can provision the project-specific
Ed25519 private key used to sign exact-byte support-intake manifests and publish
deterministic public metadata suitable for later application packaging. Keep the
bundle-encryption key and intake-signing key strictly separate.

## Verified context

- Slice 6 provides hardened `age` X25519 bundle-key provisioning, but there is
  no equivalent intake-manifest signing-key provisioning tool.
- Slice 7 verifies raw 32-byte Ed25519 public keys through OpenSSL 3.
- Slice 12 accepts explicitly supplied pinned signing-public keys, but contains
  no production trust values and is not activated.
- Intake signing key IDs are exactly `wsprrypi-intake-YYYY-NN`.
- Private key material must never enter the repository, logs, command output,
  public metadata, CI artifacts, or application binaries.

## Scope

1. Add `scripts/maintainer/provision_support_bundle_intake_signing_key.py`.
2. Require an absolute, regular, non-symlink, executable OpenSSL path that is
   not group- or world-writable.
3. Require an absolute owner-only `0700` private directory outside the
   repository, an absolute public-output path, and collision-free final and
   partial output names.
4. Invoke OpenSSL without a shell and with stdin, stdout, and stderr arranged so
   private-key bytes cannot be printed. Generate an Ed25519 PKCS#8 PEM private
   key into a private partial file, validate it, set it to `0400`, and derive the
   public SubjectPublicKeyInfo DER from that file.
5. Strictly parse the Ed25519 SPKI DER and accept only the canonical 44-byte
   structure containing exactly one 32-byte raw public key. Do not accept a
   generic or differently encoded key.
6. Publish bounded UTF-8 JSON with schema 1, project `wsprrypi`, purpose
   `support_intake_manifest_signing`, algorithm `Ed25519`, key ID, unpadded
   canonical base64url raw public key, UTC creation time, and a lowercase SHA-256
   fingerprint of the exact raw 32 public-key bytes.
7. Publish the private key as `0400` and public JSON as `0600` without
   overwriting existing files. On any failure, attempt independent best-effort
   rollback of every partial and every final published by this invocation.
8. Add deterministic fake-tool tests and a disposable real-OpenSSL fixture for
   success, permissions, exact metadata, independent public-key/fingerprint
   verification, secret non-disclosure, malformed DER, wrong algorithm,
   generator/derivation failure, unsafe paths, collisions, and publication-step
   rollback including cleanup-error continuation.
9. Wire the focused test into the Makefile and Debian non-hardware CI, and add a
   truthful implementation record and roadmap link.

## Constraints

- Use the existing repository conventions from Slice 6 where they remain
  applicable; preserve its files and behavior.
- Use `subprocess.run` with fixed argument vectors and no shell.
- Do not pass private key content in argv or capture/print private subprocess
  output.
- Do not overwrite, rotate, delete, back up, or import any existing key.
- Do not generate a production key or choose a production private directory,
  password-vault record, or public metadata destination.
- Do not add production public keys, bundle recipients, manifests, signatures,
  Dropbox request IDs, URLs, credentials, or secrets.
- Do not activate Slice 12 from main, HTTP, WebServer, installer, service, or UI.
- Do not perform network, installation, service, Raspberry Pi, GPIO, I2C,
  transmitter, or RF activity.

## Validation and evidence

- Run the focused fake-tool suite locally.
- Run the focused real-OpenSSL fixture with an explicit trusted executable.
- Run the suite in a clean Debian container using packaged OpenSSL when
  practical.
- Independently derive the real fixture's public DER/raw key from the retained
  disposable private key and compare it with metadata.
- Run Python syntax compilation, `git diff --check`, and review the complete
  staged diff.
- Perform an adversarial review of secret handling, subprocess arguments,
  output framing, DER/base64 canonicality, filesystem ownership/modes,
  collision refusal, atomic publication, rollback coverage, and documentation
  truthfulness. Correct every actionable finding and repeat until clean.

## Exit criteria

- The repository can safely create a disposable Ed25519 intake-signing private
  key and deterministic public metadata without exposing the private key.
- Focused fake and real tests pass and CI wiring is present.
- No production key or application activation is introduced.
- The implementation record states the exact separate maintainer action still
  required before any public metadata may be packaged into WsprryPi.
