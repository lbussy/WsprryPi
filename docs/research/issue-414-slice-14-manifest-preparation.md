# Issue 414 Slice 14: Signed Intake Manifest Preparation

Status: Local maintainer preparation qualified; remote publication deferred

Depends on:

- [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)
- [Slice 6 bundle-key provisioning](issue-414-slice-6-key-provisioning.md)
- [Slice 7 offline signed-intake validation](issue-414-slice-7-intake-validation.md)
- [Slice 13 signing-key provisioning](issue-414-slice-13-signing-key-provisioning.md)

## Outcome

The repository now contains a maintainer-only tool that constructs one strict
version-1 support-intake manifest, signs its exact bytes with Ed25519, verifies
the signature, and stages the complete pair locally for review.

Before signing, the tool strictly validates the complete Slice 13 signing
metadata and Slice 6 bundle metadata schemas, canonical public encodings, key-ID
syntax, UTC creation times, and independently recomputed fingerprints. It
derives the public key from the owner-controlled `0400` private key and requires
an exact match with the selected signing metadata.

Manifest construction enforces:

- project and schema version 1;
- positive generation and client protocol;
- strict bounded SemVer 2 upload version;
- real UTC timestamps from 1970 onward with publication before expiration;
- active versus disabled request-URL presence;
- exact Dropbox File Request host and opaque-ID syntax;
- a WsprryPi GitHub release URL without credentials, query, fragment, or path
  traversal;
- bounded control-free optional user copy; and
- the validated bundle-encryption key ID.

The deterministic UTF-8/LF manifest is written and fsynced before a fixed,
shell-free OpenSSL Ed25519 signing call. The tool requires exactly 64 signature
bytes, emits canonical unpadded base64url in the strict envelope, and asks
OpenSSL to verify the exact staged manifest before publication. Tool output does
not print manifest contents, URLs, user messages, signatures, or key bytes.

## Publication semantics

The caller supplies a pre-existing absolute owner-controlled `0700` staging
root and an explicit generation. Both `0600` files are built inside a new
`.generation-N.partial` directory. The files and partial directory are fsynced,
then one directory rename makes the complete pair visible as:

```text
generation-N/intake.json
generation-N/intake.json.sig
```

Existing partial or final generation paths are refused. A pre-rename failure
attempts independent cleanup of both files and the unpublished directory. After
rename, the staging root is fsynced. Root-sync failure leaves the published pair
intact and returns `committed_sync_uncertain`, because deleting possibly
published data would be less truthful than requiring inspection or retry.

This is local staging, not remote publication.

## Validation

`make support-bundle-intake-manifest-preparation-test SUDO=` covers deterministic
active and disabled manifests, exact signature encoding, hashes and modes,
version/time/URL/message policy, metadata fingerprint and private/public mismatch,
signing and self-verification failures, collision preservation, pre-rename
rollback, cleanup-error continuation, and post-rename sync uncertainty.

The suite supports a disposable real OpenSSL fixture through
`WSPRRYPI_REAL_OPENSSL=/absolute/non-symlink/path/to/openssl`. It generates a
temporary Ed25519 key, prepares the pair, independently decodes the envelope,
and verifies the signature over the exact staged manifest. Debian non-hardware
CI runs the fixture with packaged `/usr/bin/openssl`. Temporary private keys are
removed by test cleanup.

## Remaining work

- Provision, back up, recovery-test, and approve the two production identities
  through explicit maintainer actions.
- Add lifecycle-aware `inspect`, `rotate`, `disable`, and `renew` operations that
  derive a new generation from verified current state rather than accepting a
  generation in isolation.
- Add an authenticated remote publication boundary that preserves pair identity,
  then retrieves and verifies the published exact bytes.
- Package only reviewed public trust metadata and activate Slice 12 later.

No production key, public metadata, manifest, signature, Dropbox request ID,
request URL, remote repository, application runtime, HTTP, UI, installer,
service, hardware, GPIO, I2C, transmitter, or RF behavior changed in this slice.
