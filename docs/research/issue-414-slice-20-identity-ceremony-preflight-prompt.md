# Issue 414 Slice 20 Execution Prompt: Production Identity Ceremony Preflight

## Objective

Add a read-only maintainer preflight that proves a proposed WsprryPi production
identity ceremony is safe to begin before either private identity is generated.
Define the associated manual backup/recovery evidence gate. Exercise only
disposable test paths and fake executables.

## Requirements

1. Accept explicit absolute paths for the age-keygen and OpenSSL executables,
   the bundle and signing private directories, and both public metadata outputs,
   plus exact bundle/signing key IDs.
2. Reuse the Slice 6 and Slice 13 executable, private-directory, and key-ID
   validation contracts. Require owner-only `0700` real private directories
   outside the repository and safe non-symlink executables.
3. Require each public-output parent to be a real owner-controlled directory
   outside either private root. Reject identical outputs and every existing,
   symlink, hard-link, private-final, or known partial collision that either
   provisioner could encounter.
4. Make no filesystem changes, invoke no executable, generate no key, inspect no
   password vault, and contact no service. Repeated successful preflight SHALL
   be idempotent and leave an exact filesystem snapshot unchanged.
5. Return a typed, non-disclosing result limited to status and the two key IDs.
   CLI failure output SHALL not echo private paths, public paths, executable
   paths, credentials, or exception text.
6. Define the later ceremony evidence gate: both public metadata files reviewed,
   both private identities backed up in the approved vault, independent restore
   copies recovered to a disposable owner-only directory, signing verified with
   the restored Ed25519 key, encryption/decryption round-trip verified with the
   restored age identity, and disposable recovery material removed. This slice
   records the contract only and does not claim any evidence.
7. Add adversarial tests for every status, unsafe paths/modes/symlinks/tools,
   repository/private-root overlap, all collision classes, idempotency,
   zero-mutation snapshots, and output non-disclosure. Wire Make and Debian CI.

## Non-goals

- Do not choose actual storage or vault locations for the maintainer.
- Do not generate, import, back up, restore, or delete any production key.
- Do not create/contact the public repository or Dropbox, use credentials,
  publish metadata, activate runtime/UI, modify services, access a Pi, or use RF.

## Exit criteria

Unsafe or incomplete ceremony layouts fail closed without mutation; a ready
layout yields only a typed `ready` result and the documented evidence checklist.
