# Issue 414 Slice 26: Local Generation-1 Manifest

Status: Durably prepared and lifecycle-authenticated; unpublished

## Outcome

Production generation 1 is staged outside the repository in the owner-controlled
WsprryPi Support Intake manifest root. The root and generation directory are
mode `0700`; `intake.json` and `intake.json.sig` are owner-owned, regular,
single-link mode `0600` files.

The exact non-sensitive policy is:

- generation: `1`;
- status: `active`;
- published: `2026-08-17T15:33:43Z`;
- expires: `2026-11-15T15:33:43Z` (exactly 90 days);
- minimum client protocol: `1`;
- minimum upload version: `3.2.0`;
- signing key ID: `wsprrypi-intake-2026-01`;
- bundle key ID: `wsprrypi-bundle-2026-01`;
- release location: the official WsprryPi latest-release page; and
- user message: absent.

The manifest SHA-256 is:

```text
80902216b212ca1a8c2a9fd3e9693aac2c0aa17c7838d939bbebaa8887fb71e8
```

The request capability is present in the signed manifest because clients need
it after authentication, but it was never printed, placed in argv/environment,
or committed to WsprryPi. An independent in-memory comparison confirmed it
exactly matches the Slice 25 Keychain item.

## Keychain-backed controller

`prepare_support_bundle_intake_production_manifest.py` fixes the production
Keychain service/account and external identity/metadata/staging locations. Its
CLI accepts only approval and explicit publication/expiration timestamps.

It invokes exact root-owned `/usr/bin/security` with fixed shell-free arguments,
a minimal environment, discarded stderr, a hard read bound, a deadline, strict
single-line UTF-8 framing, and the existing Dropbox URL policy. The capability
exists only in process memory and is absent from results and generic errors.

macOS `/usr/bin/openssl` is LibreSSL 3.3.6 and failed closed during the first
read-only preflight because it cannot derive the Ed25519 public key. The
controller therefore fixes the qualified stable Homebrew OpenSSL 3 path. A
regression prevents fallback to macOS LibreSSL.

Before approval, the controller validates Keychain, both strict public metadata
records, the mode `0400` private signing identity, its exact public-key match,
timestamps, policy, and destination state, then reports only a digest proposal.
Approval delegates exact-byte construction, Ed25519 signing/self-verification,
atomic generation publication, and durability classification to Slice 14.

## Independent authentication and validation

The approved operation returned durable `committed`. The Slice 15 lifecycle
inspector independently verified the signature using only reviewed public
metadata, reconstructed the deterministic manifest exactly, and returned
`inspected` with the same generation, IDs, policy, timestamps, and SHA-256.

Focused validation passed:

- production controller: 7 tests;
- signing-key provisioning: 7 tests, one real-tool fixture skipped locally;
- manifest preparation: 12 tests, one real-tool fixture skipped locally;
- manifest lifecycle: 10 tests, one real-tool fixture skipped locally;
- Python syntax and final diff checks.

The skipped fixtures require their explicit real-tool environment variables;
the same underlying production OpenSSL 3 executable was exercised successfully
by the actual preflight, signing, self-verification, and lifecycle inspection.
Known macOS Make probes for Linux `/proc/meminfo` and `nproc` emitted warnings
without affecting the tests.

## Publication boundary and remaining work

The local bare publication repository and public GitHub repository remain at
the README-only initial commit
`770d63521cf23d1ccb5eb7c9911e040ab18032d7`. No candidate commit, push, public
retrieval, Dropbox modification, or application activation occurred.

Slice 27 subsequently authenticated this pair and recorded its exact bytes in a
verified local publication candidate commit. The next separately reviewed
slice is the Slice 17 live push boundary. Exact public-byte verification,
runtime activation, and encrypted upload orchestration remain later independent
boundaries.

No UI, installer, service, Raspberry Pi, GPIO, transmitter, or RF state changed.
