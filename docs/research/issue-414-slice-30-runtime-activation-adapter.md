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

The complete macOS application build was attempted. Its first attempt stopped
on the pre-existing GCC-only `-fmax-errors=10` flag under clang. With that flag
removed on the command line, the build advanced but stopped in the existing
Linux-specific `support_bundle_runtime.cpp` because macOS does not declare
`getrandom`. The new adapter's release object compiled successfully. Debian CI
remains the canonical ordinary non-hardware build environment.

Known macOS Make probes for Linux `/proc/meminfo` and `nproc` also emitted their
existing warnings without affecting focused tests.

## Remaining boundary

No network, production state, published manifest, Dropbox, Keychain, identity,
vault, installer, service, Raspberry Pi, GPIO, transmitter, or RF state changed.

The next separately reviewed slice may invoke this adapter from a guarded
backend support-intake endpoint and translate the same four states to a minimal
JSON contract. That work must decide the exact capability-disclosure moment,
retain same-origin/local-network protection, and remain separate from bundle
encryption and upload execution.
