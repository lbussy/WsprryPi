# Issue 414 Slice 7: Offline Signed-Intake Validation

Status: Offline trust and policy primitive implemented; retrieval and runtime wiring deferred

Depends on:

- [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)
- [Slice 6 key-provisioning tooling](issue-414-slice-6-key-provisioning.md)

## Outcome

WsprryPi now has a reusable offline C++ validator for exact signed-intake bytes.
It bounds both inputs before parsing, strictly parses the signature envelope,
selects only a caller-pinned Ed25519 key, decodes a canonical 64-byte unpadded
base64url signature, and verifies the signature over the exact manifest bytes
before parsing or returning manifest fields.

After authentication it strictly rejects duplicate or unknown JSON fields and
enforces schema, project, generation, rollback, same-generation immutability,
UTC validity, clock skew, status, client protocol, Dropbox request URL, GitHub
release URL, and recognized bundle-key policy. Successful results include the
exact-byte manifest SHA-256 for later persisted rollback state.

The validator is dependency-injected. This slice contains no production key,
endpoint, bundle recipient, request ID, network client, or persistent state.

## Validation

`make support-bundle-intake-validation-test SUDO=` exercises:

- valid active and disabled manifests;
- exact-byte mutation, corrupt signatures, unknown signing keys, and malformed
  or non-canonical signature envelopes;
- duplicate, unknown, missing, and wrong-type manifest fields;
- project, generation, time-window, status, protocol, URL, and bundle-key policy;
- rollback, same-generation mutation, and same-generation idempotency; and
- input bounds and failure-result non-disclosure.

The Debian non-hardware workflow runs this target with the existing OpenSSL 3
`libcrypto` dependency. Test signing keys are generated ephemerally in process;
no private key fixture is tracked.

## Remaining work

- Provision and safely retain the production intake-signing identity, then
  publish only its reviewed public metadata.
- Add bounded HTTPS retrieval with certificate/hostname validation, redirects
  disabled, explicit timeouts, and exact response-size enforcement.
- Persist the highest accepted generation and exact manifest hash under private
  WsprryPi state.
- Wire validated configuration into encryption and the later UI handoff.
- Add disabled, upgrade-required, rotation, recovery, and operator workflows.

No runtime, installer, HTTP retrieval, UI, Dropbox, GitHub posting, service,
hardware, GPIO, I2C, transmitter, or RF behavior changed in this slice.
