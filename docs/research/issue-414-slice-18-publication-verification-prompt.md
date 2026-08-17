# Issue 414 Slice 18 Execution Prompt: Public Publication Verification

## Objective

Add a maintainer-only controller that retrieves the two fixed public raw files
after a Slice 17 push and declares publication verified only when both exact
bytes, Ed25519 signature, generation, and manifest digest match the authenticated
local candidate. Exercise retrieval only through a fake transport.

## Requirements

1. Add `verify_support_bundle_intake_publication.py` with fixed production URLs
   for `wsprrypi/intake.json` and `.sig` on raw.githubusercontent.com.
2. Reauthenticate the highest Slice 15 generation and verify the Slice 16 local
   candidate while holding the staging lock.
3. Require expected candidate commit identity from the caller and exact equality
   with local `main`; never infer success merely from HTTP 200.
4. Fetch both files independently with fixed curl policy: HTTPS-only, normal CA
   and hostname verification, no redirects, no proxy environment, curl config
   disabled, status 200, deadlines, and independent protocol size bounds.
5. Compare both remote bodies byte-for-byte with local candidate blobs before
   parsing. Then independently run the strict Slice 15 signature and
   deterministic-manifest authentication on an owner-only temporary generation
   root and require generation and SHA-256 equality.
6. Return typed `verified`, `retrieval_failed`, `content_mismatch`,
   `authentication_failed`, or `local_validation_failed`. Return no fetched
   bytes, URLs, messages, signatures, or key material.
7. Production transport is fixed; injection exists only in a separately named
   test entry point. Add adversarial fake tests for every status, independent
   endpoint failure/oversize, swapped/partial/mutated bodies, signature failure,
   candidate mismatch, lock retention, idempotency, and non-disclosure.
8. Wire Make/CI and document the inactive boundary.

## Constraints

- Do not contact GitHub, create a repository, push, use credentials, or publish
  production material.
- Do not activate application, HTTP, UI, installer, service, hardware, or RF.

## Exit criteria

- Only exact publicly retrieved and independently authenticated candidate bytes
  are `verified`; all other outcomes fail closed and tests pass offline.
