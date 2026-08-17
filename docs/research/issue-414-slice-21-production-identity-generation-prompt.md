# Issue 414 Slice 21 Execution Prompt: Production Identity Generation

## Objective

Perform the explicitly approved production identity-generation ceremony using
the Slice 6 and Slice 13 provisioners, the Slice 20-ready layout, and the fixed
production IDs. Verify the resulting private/public correspondence and stop
before backup, recovery testing, publication, compilation, or activation.

## Approved production inputs

- Signing key ID: `wsprrypi-intake-2026-01`
- Bundle key ID: `wsprrypi-bundle-2026-01`
- Separate owner-only signing and encryption directories under the approved
  external Application Support root
- Separate owner-only public-staging directory under that root
- Homebrew age 1.3.1 real keg executable
- Homebrew OpenSSL 3.6.3 real keg executable

## Requirements

1. Rerun Slice 20 preflight immediately before generation and proceed only from
   exact `ready`.
2. Capture one explicit UTC ceremony timestamp and use it for both metadata
   files.
3. Invoke each qualified provisioner shell-free with fixed explicit paths and
   IDs. Never print, copy, parse through a shell, or place private bytes in argv,
   environment, repository, logs, or public metadata.
4. If either provisioner fails, stop. Preserve any successfully published first
   identity and report the incomplete pair; do not retry destructively, delete,
   overwrite, or improvise rollback.
5. Verify both private outputs are regular owner-owned single-link `0400` files;
   both public outputs are regular owner-owned single-link `0600` files; no
   partials exist; and no generated file is inside any Git worktree.
6. Independently derive the age recipient from the private identity and the
   Ed25519 public DER from the private PEM, then compare them to the strictly
   parsed metadata. Recompute both SHA-256 fingerprints and require exact match.
   Report only IDs, algorithms, timestamps, fingerprints, modes, and typed
   success—not private bytes.
7. Commit only this prompt and a non-secret implementation record. Do not commit
   generated public metadata, local absolute paths, private material, commands
   containing local paths, or recovery artifacts.

## Stop boundary

- Do not access or modify the password vault.
- Do not make recovery copies or perform restore tests.
- Do not compile or commit production trust metadata.
- Do not create/contact `WsprryPi/support-intake`, GitHub, or Dropbox.
- Do not prepare/sign/push a manifest, activate runtime/UI/services, access a
  Raspberry Pi, or exercise hardware/RF.

## Exit criteria

Both production identities and public metadata exist only in their approved
external locations and independently correspond exactly. The ceremony remains
explicitly incomplete pending vault backup and recovery testing.
