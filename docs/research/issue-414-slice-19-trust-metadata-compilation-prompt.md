# Issue 414 Slice 19 Execution Prompt: Public Trust Metadata Compilation

## Objective

Add an inactive maintainer compiler that converts reviewed public outputs from
the Slice 6 bundle-key and Slice 13 intake-signing-key provisioners into exact,
deterministic C++ data for `SupportBundleIntakeRuntimeTrust`. Use test-only
metadata; do not select or generate production identities or activate runtime.

## Scope and requirements

1. Accept one or more intake-signing public metadata files and one or more
   bundle-encryption public metadata files through explicit typed arguments.
2. Strictly bound and parse every input, rejecting duplicate JSON keys, unknown
   fields, wrong types, unsafe files, malformed project/key identifiers,
   algorithms, encodings, timestamps, fingerprints, Ed25519 values, age X25519
   recipients, and duplicate IDs.
3. Recompute and compare the SHA-256 fingerprint of every public value. Never
   accept or read a private-key/identity input.
4. Sort each key class by key ID and emit byte-for-byte deterministic C++ with
   no timestamps, source paths, recipients, fingerprints, comments containing
   metadata, or private/routing material. The output SHALL contain only signing
   IDs and 32 public bytes plus recognized bundle IDs.
5. Publish through an owner-controlled partial file and atomic replacement;
   preserve a prior output on validation/write failure and leave no partial.
6. Add adversarial tests for deterministic ordering, exact values, compilable
   output, duplicate keys at every JSON depth, schema/type/field errors,
   malformed/noncanonical public encodings, fingerprint mismatch, invalid age
   recipients, duplicate IDs, unsafe inputs, output failure preservation, and
   non-disclosure.
7. Wire the focused test into Make and Debian non-hardware CI. Document the
   inactive boundary and remaining production approval/activation work.

## Non-goals

- Do not generate, select, import, back up, or publish production keys.
- Do not add production metadata or a generated production header to Git.
- Do not contact GitHub or Dropbox, modify the installer, activate runtime,
  change HTTP/UI, operate services, access a Pi, or exercise hardware/RF.

## Exit criteria

The compiler produces the same minimal, compilable trust header from the same
reviewed public metadata regardless of input order, fails closed without
damaging prior output, passes local and Debian tests, and remains unactivated.
