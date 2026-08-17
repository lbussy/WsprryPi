# Issue 414 Slice 30 Execution Prompt: Production Runtime Activation Adapter

## Objective

Bind the already-versioned production public trust to the qualified Slice 12
runtime through one narrow backend adapter, and translate authenticated runtime
outcomes into a minimal support-intake access contract for later workflow use.
Compile the adapter into the application, but do not call it from HTTP, UI,
startup, finalization, encryption, or upload paths in this slice.

## Verified context

- Slice 23 versioned the reviewed production public metadata and deterministic
  `support_bundle_intake_compiled_trust()` header.
- Slice 29 retrieved and independently authenticated production generation 1 at
  candidate `3e0b4017bfe7612bd39ccc6e2f29d743174664b5` with manifest SHA-256
  `80902216b212ca1a8c2a9fd3e9693aac2c0aa17c7838d939bbebaa8887fb71e8`.
- Slice 12 fixes production state root, version, clock, protocol, retrieval, and
  controller dependencies and already enforces signature-before-disclosure,
  rollback, expiry, disabled, protocol, bundle-key, and upgrade policy.
- No production source currently calls either the compiled trust constructor or
  the runtime resolver.

## Scope and requirements

1. Add a backend-only production adapter that is the sole production consumer
   of `support_bundle_intake_compiled_trust()` and calls
   `resolve_support_bundle_intake_runtime()` with that exact trust.
2. Define a minimal typed result with exactly four public states:
   `active`, `disabled`, `upgrade_required`, and `unavailable`.
3. Translate only authenticated and durability-confirmed runtime outcomes:
   - `active`: require completed runtime, ready controller, manifest status
     `active`, a request URL, generation, expiration, minimum upload version,
     signing key ID, and recognized bundle key ID; only this state may carry the
     request URL, and it may preserve the optional signed user message;
   - `disabled`: require completed runtime, ready controller, manifest status
     `disabled`, and no request URL; preserve only generation, expiration,
     signing/bundle IDs, and optional signed user message;
   - `upgrade_required`: require completed runtime, controller validation status
     `upgrade_required`, and durability-approved limited upgrade guidance;
     expose only minimum version, official release URL, and optional signed user
     message; and
   - every malformed, inconsistent, failed, uncertain, or unexpected outcome:
     return a field-empty `unavailable` result.
4. Do not expose controller/retrieval/state failure details, fetched bytes,
   signature, public-key bytes, manifest digest, or arbitrary diagnostic text in
   the adapter result.
5. Provide one typed in-process provider seam for deterministic unit tests. It
   must not become CLI, configuration, environment, HTTP, or UI injection.
6. Catch provider/runtime exceptions and fail closed to field-empty
   `unavailable`.
7. Update the production-trust regression so exactly this adapter is permitted
   to reference the compiled trust/runtime pair, while every other production
   call site remains forbidden.
8. Add focused tests for every accepted state, every invariant violation,
   runtime/controller failures, uncertainty, exception containment, provider
   call count, and strict field non-disclosure.
9. Compile the new source into the ordinary application and add focused Make/CI
   coverage.

## Constraints and non-goals

- Do not invoke the production adapter during tests or execution; use only the
  typed test provider. Do not perform live HTTPS retrieval or mutate production
  rollback state in this slice.
- Do not add an HTTP route, JSON serializer, UI control, startup call, background
  refresh, cache, persistence, CLI, INI field, environment override, or logging.
- Do not expose the request URL from disabled, upgrade, unavailable, or any
  failure/uncertain result.
- Do not encrypt or upload a bundle, open Dropbox/GitHub, modify the published
  manifest, rotate keys, or change installer, service, Pi, GPIO, transmitter, or
  RF state.
- Preserve the existing compiled public trust bytes exactly.

## Validation and adversarial review

- Run the new adapter test plus Slice 12 runtime and Slice 23 production-trust
  tests.
- Build the ordinary application without installing or running it.
- Prove repository-wide that only the adapter production source references both
  compiled trust and runtime resolution, and that nothing invokes the adapter.
- Exercise malformed active/disabled/upgrade combinations and require complete
  field erasure on failure.
- Inspect result types for capability or diagnostic leakage and inspect the full
  diff for private material or activation beyond the adapter.
- Run `git diff --check`; correct every actionable finding and repeat review
  until clean.

## Exit criteria

Commit and push only after the production adapter is deterministic, fail-closed,
non-disclosing, compiled but uninvoked, all focused/build checks pass, no live
network or production-state operation occurred, and documentation accurately
separates this adapter from later HTTP/UI activation and upload orchestration.
