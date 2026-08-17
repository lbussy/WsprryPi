# Issue 414 Slice 23: Production Public Trust Compilation

Status: Implemented, validated, and inactive

## Outcome

The reviewed Slice 21 production public metadata is now versioned under
`config/support-bundle-intake/`. The checked-in files exactly match the two
owner-controlled external staging files reviewed and recovery-qualified through
Slice 22:

- intake signing key `wsprrypi-intake-2026-01`, fingerprint
  `688b5769d2b763481bad938fe8a9963693950c5e80bcf6d47d71db75711843ac`;
- bundle encryption key `wsprrypi-bundle-2026-01`, fingerprint
  `61289289afbd0f7813eb59b54e60d514f3cd8dbdf05e9c6b2d405b101b5b0fc4`.

These are public values intended for application distribution. The signing JSON
contains the Ed25519 public key. The bundle JSON contains the age X25519 public
recipient needed by a later encryption slice. Neither contains private identity
material.

`src/support_bundle_intake_compiled_trust.hpp` is the exact deterministic output
of the Slice 19 compiler over those two repository inputs. It constructs the
Slice 12 trust type with only the signing key ID and exact 32 public bytes plus
the recognized bundle key ID. In accordance with the established trust-type
boundary, the header omits the age recipient, fingerprints, timestamps, source
paths, routing, credentials, and diagnostic fields.

## Validation

`support_bundle_intake_production_trust_test.py` independently verifies the
complete expected public records, decodes and hashes the Ed25519 key, hashes the
age recipient, regenerates the header and compares exact bytes, and compiles and
runs a C++ consumer that checks the resulting trust object. It also rejects
private-key markers, routing, credentials, maintainer paths, and unexpected
header disclosure, and proves there is still no production runtime call site.

The focused Slice 19 compiler test, new production trust test, Slice 12 runtime
test, Python syntax check, and final diff check pass. Debian non-hardware CI now
runs the production trust regression alongside the existing intake suite.

## Inactive boundary and remaining work

This slice versions public trust data but does not activate it. No production
source calls `support_bundle_intake_compiled_trust()` or
`resolve_support_bundle_intake_runtime()`. No manifest was prepared, signed,
retrieved, published, or disclosed, and no bundle was encrypted or uploaded.

Slice 24 subsequently established and validated the public publication
repository and its local bare maintainer counterpart. The next separately
reviewed work is generation-1 manifest preparation using the existing Dropbox
File Request URL. Candidate commit, live publication, exact public-byte
verification, runtime activation, and encryption/upload orchestration remain
later slices.

No private identity, private-file hash, credential, Dropbox/GitHub operation,
installer, HTTP, UI, service, Raspberry Pi, GPIO, transmitter, or RF state
changed.
