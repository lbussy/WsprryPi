# Issue 414 Slice 13: Intake-Signing Key Provisioning

Status: Maintainer tooling qualified; production signing key not generated

Depends on:

- [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)
- [Slice 7 offline signed-intake validation](issue-414-slice-7-intake-validation.md)
- [Slice 12 runtime construction](issue-414-slice-12-intake-runtime.md)

## Outcome

The repository now contains a maintainer-only tool that can provision a
project-specific Ed25519 intake-manifest signing key and deterministic public
metadata without printing or copying private key content. It uses shell-free
fixed OpenSSL argument vectors, discards generation output, applies an owner-only
umask before generation, validates the private output, and derives only the
public SubjectPublicKeyInfo bytes.

The derivation boundary accepts only the canonical 44-byte Ed25519 SPKI DER
prefix followed by exactly 32 raw public-key bytes. Public metadata version 1
records:

- schema version 1;
- project `wsprrypi` and purpose `support_intake_manifest_signing`;
- algorithm `Ed25519`;
- an exact `wsprrypi-intake-YYYY-NN` key ID;
- the raw 32-byte public key as canonical unpadded base64url;
- UTC creation time; and
- lowercase SHA-256 of the exact raw public-key bytes.

The tool requires an absolute safe OpenSSL executable and an owner-controlled
`0700` private directory outside the repository. It refuses collisions,
publishes the private PKCS#8 PEM as `0400` and public JSON as `0600`, and
independently attempts rollback of partial and final outputs after failure.

## Validation

`make support-bundle-signing-key-provisioning-test SUDO=` exercises a fake
OpenSSL executable and covers exact metadata, permissions, independent decoding
and fingerprint verification, private non-disclosure, generation and derivation
failures, wrong-algorithm and non-canonical DER, unsafe paths, collisions, each
publication step, and cleanup-error continuation.

The same suite supports a disposable real-tool fixture through
`WSPRRYPI_REAL_OPENSSL=/absolute/non-symlink/path/to/openssl`. Debian
non-hardware CI runs that fixture with packaged `/usr/bin/openssl`. The fixture
independently re-derives the public DER from the retained temporary private key
and compares its raw value and digest with the published metadata. All fixture
keys remain under temporary test directories and are deleted by test cleanup.

## Production handoff

The real production operation is deliberately not executed. Before doing so,
the maintainer must select:

1. the permanent owner-only private signing-key directory;
2. the password-vault record and secure attachment/recovery format;
3. the public metadata staging path; and
4. the initial key ID, expected to be `wsprrypi-intake-2026-01`.

The reviewed command shape is:

```text
python3 scripts/maintainer/provision_support_bundle_intake_signing_key.py \
  --openssl /usr/bin/openssl \
  --private-directory /ABSOLUTE/OWNER-ONLY/PRIVATE/DIRECTORY \
  --public-output /ABSOLUTE/PUBLIC/METADATA/STAGING/PATH.json \
  --key-id wsprrypi-intake-2026-01
```

The private key must be backed up and recovery-tested before its public metadata
is committed, packaged, or used to sign a production manifest. Only public JSON
may enter the repository. This key must never be reused for bundle encryption.

## Remaining work

- Select the two production private-key storage and password-vault backup
  destinations for the separate Slice 6 and Slice 13 keys.
- Provision, independently review, back up, and recovery-test both identities as
  explicit maintainer operations.
- Commit and package only approved public metadata.
- Use Slice 14 to construct, exact-byte sign, self-verify, and atomically stage a
  new local manifest pair; lifecycle administration and remote publication
  remain separate work.
- Activate Slice 12 only after production public trust is reviewed.

No production key, public trust value, manifest, signature, recipient, Dropbox
request ID, URL, runtime activation, installer, UI, service, hardware, GPIO,
I2C, transmitter, or RF behavior changed in this slice.
