# Issue 414 Slice 14 Execution Prompt: Signed Intake Manifest Preparation

## Objective

Add a maintainer-only, hardware-free tool that prepares and exact-byte signs one
strict WsprryPi support-intake manifest into a new local staging directory. The
result is a reviewable `intake.json` and `intake.json.sig` pair suitable for a
later, separately authorized publication workflow.

## Verified context

- Slice 1 fixes the version-1 manifest and detached-signature formats.
- Slice 7 verifies Ed25519 over the exact manifest bytes before disclosure.
- Slice 13 provisions separate Ed25519 signing-key public metadata without
  generating a production key.
- No maintainer tool yet constructs, signs, self-verifies, or stages a manifest.
- Remote GitHub publication, Dropbox request administration, and application
  activation remain unimplemented and unauthorized in this slice.

## Scope

1. Add `scripts/maintainer/prepare_support_bundle_intake_manifest.py`.
2. Require absolute, safe, non-symlink OpenSSL and input paths; require the
   private key to be owner-controlled, regular, single-link, and exactly `0400`.
3. Strictly parse Slice 13 signing metadata and Slice 6 bundle metadata. Require
   exact schemas, project/purpose/algorithm/key-ID syntax, canonical public-key
   or recipient encoding, and independently recomputed fingerprints.
4. Derive the public Ed25519 key from the private key and require exact equality
   with the selected signing metadata before signing.
5. Construct deterministic UTF-8 JSON with LF ending and the exact Slice 1
   top-level fields. Validate positive generation/protocol, strict SemVer,
   strict UTC ordering, bounded optional user message, exact active/disabled
   request-URL rules, fixed WsprryPi GitHub release URL policy, and the selected
   recognized bundle key ID.
6. Sign the exact `intake.json` bytes with Ed25519 through fixed, shell-free
   OpenSSL argv. Require exactly 64 signature bytes, encode canonical unpadded
   base64url, construct the strict signature envelope, and self-verify the exact
   bytes before publication.
7. Publish only beneath a pre-existing absolute owner-controlled `0700` staging
   root. Build both bounded `0600` files inside a new owner-only partial
   generation directory, fsync both files and that directory, then make the pair
   visible together by renaming the directory to `generation-N`. Refuse final or
   partial collisions, fsync the staging root, and clean the unpublished partial
   directory after pre-rename failures. Report post-rename root-sync failure
   truthfully as `committed_sync_uncertain` without deleting published files.
8. Add deterministic fake-tool tests and a disposable real-OpenSSL fixture for
   exact-byte signing/verification, metadata/private-key mismatch, schema and
   fingerprint rejection, active/disabled policy, bounds, collisions, every
   publication step, cleanup-error continuation, and sync uncertainty.
9. Wire the focused test into the Makefile and Debian non-hardware CI, then add
   a truthful implementation record and roadmap link.

## Constraints

- Do not generate, select, import, back up, rotate, or use any production key.
- Do not include any private key, credential, token, Dropbox request ID, real
  request URL, manifest, signature, or production public metadata in the repo.
- Do not overwrite or update an existing staging pair; each preparation uses a
  new generation directory and generation is an explicit reviewed input.
- Do not implement `inspect`, `rotate`, `disable`, or `renew` lifecycle state,
  remote GitHub publication, post-publication retrieval, Dropbox API/browser
  administration, reminders, application runtime activation, HTTP, UI, service,
  installer, Raspberry Pi, GPIO, I2C, transmitter, or RF behavior.
- Do not print manifest contents, URLs, messages, signatures, public-key bytes,
  or private-key content. Success output may identify only output paths,
  generation, signing key ID, bundle key ID, and exact manifest SHA-256.

## Validation and evidence

- Run fake-tool and real disposable OpenSSL tests locally.
- Run a clean Debian packaged-OpenSSL fixture.
- Independently verify the real signature over the exact staged manifest bytes.
- Run Python syntax compilation and `git diff --check`.
- Perform an adversarial review of schema strictness, canonical encoding,
  private-key handling, argv/output disclosure, exact-byte identity, URL/time/
  version rules, no-overwrite publication, fsync truthfulness, rollback, tests,
  and documentation. Correct every actionable finding and repeat until clean.

## Exit criteria

- A disposable, strictly validated manifest can be exact-byte signed,
  self-verified, and atomically staged as a new pair.
- Tests pass with fake and packaged OpenSSL and CI wiring is present.
- No production material, remote publication, or application activation occurs.
- The record identifies lifecycle administration and remote publication as the
  next separate boundary.
