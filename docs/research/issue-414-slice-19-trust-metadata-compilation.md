# Issue 414 Slice 19: Public Trust Metadata Compilation

## Outcome

This slice adds an inactive maintainer compiler for the public JSON emitted by
the Slice 6 age-key and Slice 13 intake-signing-key provisioners. It validates
the complete public metadata contract, independently recomputes each public
value fingerprint, sorts both key classes, and emits deterministic C++ suitable
for constructing `SupportBundleIntakeRuntimeTrust`.

The generated data is deliberately minimal: each intake signing key contributes
only its syntactically valid key ID and exact 32 public bytes; each bundle key
contributes only its recognized key ID. Recipients, fingerprints, creation
timestamps, source paths, private identities, routing, and diagnostic fields are
not emitted.

Input parsing is bounded, UTF-8 strict, duplicate-key rejecting at every object
depth, exact-field, and exact-type. It enforces project/purpose/algorithm/key-ID
policy, calendar-valid UTC timestamps, canonical Ed25519 base64url, valid age
X25519 Bech32 recipients, matching SHA-256 fingerprints, uniqueness, the runtime
maximum of 16 keys per class, and owner-controlled regular input files. Output
uses an owner-only exclusive partial, file fsync, atomic replacement, and
directory fsync.

## Validation

`make support-bundle-intake-trust-compilation-test SUDO=` uses only reserved
2099 test IDs and synthetic public values. It proves deterministic input-order
independence, exact/minimal output, C++ compilation, nested and top-level
duplicate rejection, malformed policy/encoding/fingerprint/recipient/timestamp
rejection, key-count bounds, unsafe-input rejection, prior-output preservation,
partial cleanup, and non-disclosure. Debian non-hardware CI runs the same test.

## Inactive boundary and remaining work

No production public metadata or generated production header is committed, and
the application does not call the generated function. Production identities
still require separately approved storage, generation, password-vault backup,
recovery testing, and public-value review. Only then may an approved generated
header be versioned and Slice 12 runtime activation be considered.

No network, GitHub/Dropbox administration, credential, installer, HTTP, UI,
service, Raspberry Pi, GPIO, transmitter, or RF behavior changed.
