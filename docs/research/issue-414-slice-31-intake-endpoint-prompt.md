# Issue 414 Slice 31 Execution Prompt: Guarded Intake Endpoint

## Objective

Activate the compiled production intake adapter only behind one guarded,
on-demand backend JSON endpoint. Translate its four typed states without
exposing diagnostics or disclosing the Dropbox request capability before an
authenticated `active` result.

## Verified context

- Slice 30 compiled and tested `resolve_support_bundle_intake_production()` but
  intentionally left it uninvoked.
- The adapter already enforces authenticated, durability-confirmed
  `active`, `disabled`, and `upgrade_required` results and field-empty
  `unavailable` failure.
- Existing support-bundle HTTP routes use `SupportRequestGuard` to require a
  recognized local client, valid Host, and same-origin Origin when supplied.
- Current `devel` provides a hardware-free simulated build profile for macOS;
  physical Si5351 Linux-I2C portability is deferred under Issue 411.

## Scope and requirements

1. Register exactly one `GET /api/support-intake` production route with the
   existing support-bundle route family.
2. Run the existing local-network/Host/Origin guard before invoking any intake
   provider. Foreign, null, malformed, or unrecognized requests must return a
   non-disclosing forbidden response and invoke the provider zero times.
3. Invoke the provider exactly once per allowed GET. Do not resolve at server
   startup, cache a result, refresh in the background, or persist HTTP output.
4. Return `Cache-Control: no-store`, `X-Content-Type-Options: nosniff`, and no
   permissive CORS headers.
5. Use this minimal JSON contract:
   - `active` (HTTP 200): `status`, `generation`, `expires_at`,
     `minimum_upload_version`, `signing_key_id`, `bundle_key_id`, and
     `request_url`, plus optional `user_message`;
   - `disabled` (HTTP 200): `status`, `generation`, `expires_at`,
     `signing_key_id`, and `bundle_key_id`, plus optional `user_message`;
   - `upgrade_required` (HTTP 200): `status`, `minimum_upload_version`, and
     `release_url`, plus optional `user_message`; and
   - `unavailable` (HTTP 503): exactly `{"status":"unavailable"}`.
6. Independently reject malformed/inconsistent typed provider results as
   field-empty `unavailable`; a test provider must not be able to leak a request
   URL through disabled, upgrade, unavailable, or malformed output.
7. Catch provider and serialization exceptions and return only the unavailable
   contract.
8. Register a guarded OPTIONS response for the exact endpoint without enabling
   cross-origin access.
9. Wire production explicitly to `resolve_support_bundle_intake_production` and
   update source regressions so this is the sole production invocation.
10. Add focused HTTP tests for all states, exact fields, optional-message
    behavior, malformed-result erasure, exception containment, provider call
    counts, guard rejection, headers, and repeated on-demand resolution.

## Constraints and non-goals

- Tests must use the typed in-process provider and must not retrieve live HTTPS
  endpoints or modify production rollback state.
- Do not add UI, startup calls, caches, timers, CLI/INI/environment overrides,
  logging of response fields, encryption, upload, Dropbox navigation, GitHub
  posting, installer/service changes, hardware access, transmission, or RF.
- Do not expose fetched bytes, signatures, digests, public keys, controller
  enums, filesystem paths, arbitrary errors, or private material.

## Validation and exit criteria

- Run the HTTP, web-server wiring, production adapter, runtime, and production
  trust tests.
- Build the application with the macOS simulated/GPIO-free profile and the
  established Apple-clang validation flags.
- Prove repository-wide that the endpoint is the only production caller and
  that no UI calls it yet.
- Run `git diff --check`, inspect the complete diff, correct every actionable
  adversarial finding, then commit and push only this branch.
