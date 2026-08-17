# Issue 414 Slice 30: Production Runtime Activation Adapter

Status: Compiled, tested, and intentionally uninvoked

## Outcome

WsprryPi now has one backend production adapter that binds the reviewed compiled
public trust to the qualified Slice 12 runtime. It is the sole production source
that calls both:

```text
support_bundle_intake_compiled_trust()
resolve_support_bundle_intake_runtime(...)
```

No HTTP, UI, startup, finalization, encryption, or upload path invokes the
adapter. This slice therefore compiles the production activation boundary
without performing runtime HTTPS retrieval or changing rollback state.

## Minimal result contract

`SupportBundleIntakeProductionResult` has four states:

- `active`: requires a completed runtime, ready controller, durability-confirmed
  state, complete active manifest identity, and a request URL. It carries only
  generation, expiration, minimum upload version, signing/bundle key IDs, the
  authenticated request URL, and optional signed user message.
- `disabled`: requires the same authenticated and durable boundary, status
  `disabled`, and no request URL. It carries only generation, expiration,
  signing/bundle key IDs, and optional signed user message.
- `upgrade_required`: requires the controller's authenticated, durably recorded
  upgrade path and an otherwise empty manifest. It carries only minimum version,
  official release URL, and optional signed user message.
- `unavailable`: represents every runtime, retrieval, validation, state,
  durability, malformed, inconsistent, or exceptional result and contains no
  other fields.

Only `active` can contain the Dropbox request capability. Controller diagnostic
enums, fetched bytes, signatures, public-key bytes, manifest digest, and string
errors are absent from the result type.

The test seam accepts one typed in-process runtime provider. It is not exposed
through CLI, INI, environment, HTTP, or UI configuration. A missing or throwing
provider returns field-empty `unavailable`.

## Validation

Focused validation passed:

- production activation adapter test;
- Slice 12 intake runtime test;
- Slice 23 production trust regression;
- release compilation of `support_bundle_intake_production.cpp` with the
  repository flags adapted only to remove clang's unsupported GCC diagnostic
  option; and
- final diff and repository-wide call-site checks.

The adapter tests cover exact active, disabled, and upgrade mapping; provider
call count; every runtime/controller failure class; durability uncertainty;
missing identity and policy fields; inconsistent status/diagnostic combinations;
capability leakage attempts; missing providers; and exception containment.

The production-trust regression now permits exactly the adapter to consume the
compiled trust/runtime pair and continues to reject every other production call
site. A repository-wide source scan confirms nothing calls
`resolve_support_bundle_intake_production()`.

The complete macOS application build attempt originally stopped on the
pre-existing GCC-only `-fmax-errors=10` flag under clang and then on the
Linux-only `getrandom` runtime dependency. The subsequent
[integration rebase and macOS portability qualification](issue-414-integration-rebase-macos-portability.md)
resolved and qualified both limitations after rebasing this work onto current
`devel`. The complete application build then reached a separate existing clang
warning in the reusable LCBLog component; that component issue remains outside
this slice. Debian remains the canonical ordinary non-hardware build
environment.

## Remaining boundary

No network, production state, published manifest, Dropbox, Keychain, identity,
vault, installer, service, Raspberry Pi, GPIO, transmitter, or RF state changed.

[Slice 31](issue-414-slice-31-intake-endpoint.md) subsequently invoked this
adapter only from an explicit guarded backend request and translated the same
four states to a minimal non-cacheable JSON contract. UI consumption,
encryption, and upload execution remain separate later boundaries.
