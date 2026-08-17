# Issue 414 Slice 31: Guarded Intake Endpoint

Status: Implemented, tested, and production-wired on demand

## Outcome

WsprryPi now registers one guarded `GET /api/support-intake` endpoint. The
endpoint invokes `resolve_support_bundle_intake_production()` only after the
existing local-network, Host, and same-origin guard accepts an explicit
request. It performs no startup resolution, background refresh, HTTP-result
cache, or new persistence.

The response contract is deliberately narrow:

- `active` returns HTTP 200 with generation, expiry, minimum upload version,
  signing/bundle key IDs, the authenticated Dropbox request URL, and an
  optional signed user message;
- `disabled` returns HTTP 200 with generation, expiry, signing/bundle key IDs,
  and an optional message, but no request URL or upload version;
- `upgrade_required` returns HTTP 200 with only minimum upload version,
  official release URL, and an optional message; and
- every unavailable, malformed, inconsistent, exceptional, or non-disclosable
  result returns HTTP 503 with exactly `{"status":"unavailable"}`.

Every endpoint response removes permissive CORS and carries `Cache-Control:
no-store` and `X-Content-Type-Options: nosniff`. The exact OPTIONS route is
guarded and does not invoke the intake provider.

The HTTP translator independently validates the typed adapter result. A test
provider cannot place a request URL in disabled, upgrade, unavailable, or
malformed output. Guard rejection and provider exceptions disclose no
capability or diagnostic text.

## Validation

Passed on the macOS development host:

```text
make -C src support-bundle-http-test \
  support-bundle-web-server-wiring-test \
  support-bundle-intake-production-test \
  support-bundle-intake-runtime-test \
  support-bundle-intake-production-trust-test SUDO=

make -C src release BACKENDS=simulated ANCILLARY_GPIO=0 \
  COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' SUDO=
```

The HTTP test used only an ephemeral localhost listener and typed in-process
providers. It covered all four states, exact field sets, optional messages,
malformed-result erasure, exception containment, guard rejection, provider
call counts, restrictive headers, and OPTIONS behavior. No live intake HTTPS
retrieval or production rollback-state write occurred.

The hardware-free simulated/GPIO-free profile compiled and linked the complete
application, including the production endpoint wiring. This qualifies the
software integration on macOS; it does not qualify physical Si5351 I2C, GPIO,
Raspberry Pi installation, services, transmitter hardware, or RF.

## Remaining boundary

No UI calls this endpoint yet. No bundle encryption, upload execution, Dropbox
navigation, GitHub posting, installer/service operation, hardware access,
transmission, or RF activity was added.

The next slice may consume this endpoint from the support-bundle UI and render
the four states without opening Dropbox or uploading. Encryption and upload
consent remain separate later boundaries.
