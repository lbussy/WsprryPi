# Issue 414 Slice 7: Offline Signed-Intake Validation Prompt

## Objective

Implement and qualify the offline trust boundary for WsprryPi's signed support
intake configuration. Accept bounded exact manifest and signature-envelope bytes,
verify a test-injected Ed25519 public key, parse both documents strictly, and
return only policy-validated configuration. Stop before network retrieval,
persistence, runtime wiring, or UI.

## Verified context

- Slice 1 fixes the version-1 manifest and detached-signature contracts.
- Slice 6 provides bundle-key provisioning tooling, but no production bundle or
  intake-signing key has been generated.
- OpenSSL 3 `libcrypto` is already a build dependency and supports raw Ed25519
  public keys through EVP.
- Production endpoints and keys must not be invented or embedded in this slice.

## Scope

1. Add a typed C++ offline validator that receives exact manifest bytes, exact
   signature-envelope bytes, pinned test-injected signing keys, recognized
   bundle-key IDs, current UTC time, client protocol, and previously accepted
   generation/hash state.
2. Bound the manifest to 16 KiB and the signature envelope to a smaller explicit
   limit before parsing.
3. Strictly reject malformed JSON, duplicate keys at any object depth, unknown
   top-level fields, wrong types, missing fields, and trailing content.
4. Require signature-envelope schema 1, algorithm `Ed25519`, a pinned key ID,
   and canonical unpadded base64url decoding to exactly 64 signature bytes.
5. Verify Ed25519 over the exact manifest bytes before parsing or exposing any
   manifest field.
6. Enforce the Slice 1 project, schema, generation, UTC-window, status, protocol,
   URL, recognized bundle-key, and rollback/same-generation rules.
7. Return the manifest SHA-256 and validated fields without persisting state.
8. Add focused valid and adversarial tests using ephemeral test-only Ed25519
   keys, including exact-byte mutation, wrong key, corrupt signature, strict
   JSON failures, time boundaries, disabled/active variants, URL restrictions,
   protocol gating, unknown bundle keys, rollback, and same-generation mutation.

## Constraints and non-goals

- Do not generate or publish a production signing or bundle-encryption key.
- Do not add production endpoint URLs, public keys, recipients, or fingerprints.
- Do not fetch any network resource or add HTTP, redirect, TLS, timeout, cache,
  filesystem persistence, runtime, installer, service, or UI behavior.
- Do not reveal an unverified request URL or other manifest field in a failure.
- Do not change encryption, Dropbox handoff, GitHub workflow, hardware, GPIO,
  I2C, transmitter, or RF behavior.
- Test keys are ephemeral process memory only and must not be tracked as private
  key fixtures.

## Validation

- Focused C++ unit tests linked to OpenSSL `libcrypto`.
- Tests must prove signature verification covers exact bytes and precedes
  manifest-policy output.
- Exercise every failure category and both accepted status variants.
- `git diff --check`, staged-diff review, and independent adversarial review;
  correct all actionable findings before commit.

## Exit criteria

Stop with a reusable, offline-only validation primitive and documented outcome.
Commit and push only attributable Slice 7 files. Retrieval, persisted rollback
state, production key provisioning, runtime wiring, and UI remain later slices.
