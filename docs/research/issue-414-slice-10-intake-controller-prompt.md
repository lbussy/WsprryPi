# Issue 414 Slice 10: Signed-Intake Controller Prompt

## Objective

Implement and qualify one typed controller that composes the existing bounded
HTTPS retrieval, exact-byte signed-manifest validation, and private rollback
state primitives in a fail-closed order. Stop before application/runtime
construction, production trust material, encryption/upload orchestration, or UI.

## Verified context

- Slice 9 retrieves the exact paired manifest and signature-envelope bytes and
  returns neither document when either fetch fails.
- Slice 7 verifies the detached Ed25519 signature before exposing manifest
  fields and enforces project, generation, time, status, protocol, URL, and
  recognized bundle-key policy.
- Slice 8 loads and atomically commits only the accepted generation and exact
  manifest SHA-256, serializes writers, rejects rollback or same-generation
  mutation, and distinguishes published-but-not-confirmed directory-sync state.
- No production signing public key, bundle recipient, Dropbox request ID, or
  runtime-owned state directory has been selected.

## Scope and required behavior

1. Add a typed controller request containing the private state root, retrieval
   request, pinned signing public keys, recognized bundle-key IDs, current UTC
   time, and supported client protocol.
2. Execute exactly this order: load prior state; retrieve both exact documents;
   validate/authenticate them using the loaded prior state; then commit the
   validated generation and exact-byte digest.
3. Treat absent prior state as first use. Fail closed before retrieval when the
   state root or existing state is unsafe, unreadable, or invalid.
4. Never pass unverified manifest fields to state commit and never return a
   manifest on retrieval, validation, commit, or durability-confirmation
   failure. Preserve the underlying stage status in typed diagnostic fields,
   but never include fetched bytes, URLs, messages, signatures, or key material
   in an error object or log.
5. Treat `committed` and `unchanged` as successful, durable outcomes. Treat
   `committed_sync_uncertain` as an unsuccessful controller result with no
   manifest disclosure; a retry of the identical signed generation may confirm
   durability through Slice 8's unchanged path.
6. Preserve Slice 8 as the final race authority: if another writer advances
   state after this controller's load/validation, surface commit rollback or
   mutation as failure and disclose no manifest.
7. Provide a typed in-process dependency seam for deterministic tests only.
   Production must call the existing concrete Slice 7-9 primitives directly;
   add no CLI, INI, environment, HTTP, or UI injection mechanism.
8. Add focused tests proving call order, input propagation, absent/loaded state,
   success and idempotency, every stage failure, no later calls after failure,
   failure non-disclosure, concurrent-state advancement, and uncertain-sync
   retry. Include at least one offline end-to-end path using real signature
   validation and real state persistence with only retrieval injected.
9. Add the focused target to Debian non-hardware CI and document the exact
   implemented boundary and remaining work.

## Constraints and non-goals

- Do not add or publish production manifests, signatures, public/private keys,
  bundle recipients, Dropbox request URLs, or state-directory configuration.
- Do not change Slice 7 validation policy, Slice 8 persistence schema, or Slice
  9 transport policy merely to simplify controller composition.
- Do not cache or persist fetched documents, signatures, URLs, messages, release
  links, recipients, or key IDs beyond the existing minimal rollback state.
- Do not add retry/backoff, background refresh, application startup/runtime
  wiring, archive encryption, Dropbox upload, GitHub issue posting, UI, or
  operator-documentation screenshots.
- Do not perform network access in tests or any installer, service, hardware,
  GPIO, I2C, transmitter, or RF activity.

## Validation and evidence

- Run the Slice 7, 8, 9, and new Slice 10 focused tests locally.
- Run the new controller target in a clean Debian container without network
  access by using a compiled/in-process fixture rather than the live endpoint.
- Verify the CI target, final `git diff --check`, complete staged diff, and an
  independent adversarial review. Correct every actionable finding and repeat
  review until no blockers remain.

## Exit criteria

Stop with a reusable, tested controller whose only successful output is a
cryptographically validated manifest backed by durably confirmed rollback
state. Commit and push only attributable Slice 10 files. Runtime construction,
production trust/publication material, encryption/upload composition, and UI
remain separate later slices.
