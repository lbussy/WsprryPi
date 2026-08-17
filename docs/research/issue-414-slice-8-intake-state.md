# Issue 414 Slice 8: Signed-Intake Rollback State

Status: Private rollback-state primitive implemented; retrieval and runtime wiring deferred

Depends on:

- [Slice 7 offline signed-intake validation](issue-414-slice-7-intake-validation.md)

## Outcome

WsprryPi now has an offline C++ primitive that loads and atomically commits the
highest accepted signed-intake manifest generation and its exact-byte SHA-256.
The fixed version-1 state contains only:

- schema version;
- project ID;
- positive generation; and
- lowercase 64-character manifest SHA-256.

The primitive requires an injected absolute, owner-only `0700` storage root and
uses directory-relative operations after opening and verifying that root without
following a symlink. Existing state must be a single-link, owner-only `0600`
regular file. Parsing rejects malformed, duplicate, unknown, missing, wrong-type,
empty, trailing, and oversized content.

Commits take an exclusive directory lock before loading transition state, reject
rollback and same-generation mutation, treat identical state as idempotent, and
permit only a higher generation. A higher generation is written
to an exclusive same-directory `0600` temporary file, synced, atomically renamed,
and followed by a directory sync. Immediately before rename, the temporary
pathname is checked against the still-open descriptor for device, inode, owner,
mode, link count, and exact size. A directory-sync failure is reported as
published with uncertain durability rather than falsely claiming no change. An
identical retry re-syncs the directory before reporting confirmed durability.

No URL, message, signature, key, recipient, version string, contact value, or
diagnostic data is persisted.

## Validation

`make support-bundle-intake-state-test SUDO=` covers:

- absent state, first commit, strict load, idempotency, and higher generation;
- rollback, same-generation mutation, and invalid input with prior preservation;
- malformed schema/project/generation/digest, duplicates, unknown fields,
  trailing content, empty files, and size bounds;
- unsafe root and state modes, root/state symlinks, hard links; and
- exclusive temporary collisions, write and file-sync failures, pathname
  substitution, rename failure, and directory-sync uncertainty;
- prior-state preservation and temporary cleanup before publication; and
- deterministic competing writers proving a stale lower generation cannot
  overwrite a newly committed higher generation.

The focused target is included in Debian non-hardware CI.

## Remaining work

- Add bounded HTTPS retrieval with normal certificate and hostname validation,
  redirects disabled, explicit timeouts, and exact response-size enforcement.
- Combine retrieval, Slice 7 validation, and Slice 8 state commit behind a typed
  controller without exposing unverified manifest fields.
- Provision production signing and bundle identities only after their permanent
  private-storage and recovery destinations are selected.
- Add runtime construction and later UI/operator workflows in separate slices.

No HTTPS, runtime wiring, installer, UI, Dropbox, GitHub posting, service,
hardware, GPIO, I2C, transmitter, or RF behavior changed in this slice.
