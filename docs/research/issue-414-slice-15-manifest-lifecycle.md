# Issue 414 Slice 15: Local Intake Manifest Lifecycle

Status: Local lifecycle administration qualified; remote publication deferred

Depends on:

- [Slice 13 signing-key provisioning](issue-414-slice-13-signing-key-provisioning.md)
- [Slice 14 signed manifest preparation](issue-414-slice-14-manifest-preparation.md)

## Outcome

The repository now contains a maintainer-only local lifecycle tool with
`inspect`, `rotate`, `disable`, and `renew` operations. It treats the staging
root as authenticated append-only history: every entry must be a contiguous
canonical `generation-N` directory, and each directory must contain only the
two owner-controlled `0600` Slice 14 files.

The tool holds a shared directory lock for inspection and an exclusive lock for
mutation. It selects the highest complete generation, strictly validates the
signature envelope and selected Slice 13 public metadata, verifies Ed25519 over
the exact manifest bytes before parsing, then reconstructs the deterministic
Slice 14 document and requires byte-for-byte equality. Directory generation,
manifest generation, schema, project, status shape, policy fields, URLs,
timestamps, and bundle key ID must all agree.

`inspect` requires no private key and changes nothing. Its output is restricted
to lifecycle status, generation, intake status, timestamps, minimum protocol and
version, public key IDs, and exact manifest SHA-256. It does not print routing
URLs, messages, signatures, or key bytes.

## Mutating operations

Without `--approve`, mutating commands authenticate current state, validate the
complete proposed successor, and report only a non-sensitive summary and digest.
They create no files. Approved operations require the matching `0400` signing
private key and matching bundle public metadata, derive `current + 1`, and
delegate all signing, self-verification, atomic generation-directory publication,
and durability classification to Slice 14.

- `rotate` requires a replacement Dropbox request URL, sets the successor
  active even when restoring a disabled intake, and may explicitly replace the
  minimum upload version or user message.
- `disable` removes the request URL, sets disabled, and requires a nonempty
  bounded user message.
- `renew` changes only publication and expiration timestamps.

All successors require a later publication time and an expiration later than
both publication and the current expiration. Existing generations are never
edited, deleted, renamed, or overwritten. Slice 14
`committed_sync_uncertain` is preserved as a distinct lifecycle result.

## Validation

`make support-bundle-intake-manifest-lifecycle-test SUDO=` covers authenticated
inspection, dry-run immutability, approval gating, active rotation, restoration
from disabled, disablement, renewal field preservation, monotonic timestamps,
unsafe and unexpected inventory, gaps and partial directories, exact-byte
mutation, verification failure, predecessor preservation, CLI non-disclosure,
and delegated durability uncertainty.

The real OpenSSL fixture generates a disposable Ed25519 key, stages generation
1, authenticates it using only public metadata, rotates to generation 2, proves
the predecessor bytes unchanged, and independently authenticates the successor.
Debian non-hardware CI runs the same fixture with packaged `/usr/bin/openssl`.

## Remaining work

- Provision, back up, recovery-test, and approve the two production identities
  through explicit maintainer actions.
- Define signing-key and bundle-key rotation transitions separately; Slice 15
  intentionally requires the existing selected identities.
- Use Slice 16 to create a verified local publication candidate commit; remote
  push and exact-byte public retrieval remain separate boundaries.
- Add expiration monitoring/reminders and documented maintainer recovery.
- Package approved public trust and activate the application only afterward.

No production key, metadata, manifest, signature, Dropbox request ID, routing
URL, remote repository, runtime, HTTP, UI, installer, service, hardware, GPIO,
I2C, transmitter, or RF behavior changed in this slice.
