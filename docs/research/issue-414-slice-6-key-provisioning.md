# Issue 414 Slice 6: Bundle-Key Provisioning Tooling

Status: Maintainer tooling qualified; production identity not generated

Depends on:

- [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)
- [Slice 5 Debian age qualification](issue-414-slice-5-age-qualification.md)

## Outcome

The repository now contains maintainer-only tooling that can provision a
project-specific X25519 `age` identity and deterministic public metadata without
printing or copying private key content. It validates the fixed executable,
bundle key ID, owner-only private directory, output paths, generated identity,
and derived recipient; refuses symlinks and collisions; publishes the identity
as `0400`; publishes public JSON as `0600`; and removes partial output after
failure.

Public metadata version 1 records:

- schema version 1;
- project `wsprrypi` and purpose `support_bundle_encryption`;
- algorithm `age-x25519`;
- bundle key ID;
- public recipient;
- UTC creation time; and
- lowercase SHA-256 of the exact ASCII recipient bytes without a newline.

The fingerprint is a stable public-key identifier, not a replacement for
protecting or backing up the private identity.

## Validation

The Python suite covers successful publication and permissions, independently
computed fingerprint, public non-disclosure, existing-output collisions,
unsafe directory permissions, invalid key IDs, symlinked executables, invalid
recipients, generator failure, and partial cleanup.

A disposable Debian Trixie run with `/usr/bin/age-keygen` passed:

```text
Ran 7 tests
OK
real age-keygen provisioning fixture: PASS
```

The real fixture used only temporary directories and removed the ephemeral
identity at container exit. The canonical Debian non-hardware workflow now
runs `make support-bundle-key-provisioning-test` with packaged `age`.

## Production handoff

The real production operation is deliberately not executed yet. Before doing
so, the maintainer must choose:

1. the permanent owner-only private identity directory;
2. the password-vault record and secure attachment/recovery format;
3. the public metadata staging path for review; and
4. the initial key ID, expected to be `wsprrypi-bundle-2026-01`.

After those destinations are selected, the reviewed command shape is:

```text
python3 scripts/maintainer/provision_support_bundle_age_key.py \
  --age-keygen /usr/bin/age-keygen \
  --private-directory /ABSOLUTE/OWNER-ONLY/PRIVATE/DIRECTORY \
  --public-output /ABSOLUTE/PUBLIC/METADATA/STAGING/PATH.json \
  --key-id wsprrypi-bundle-2026-01
```

The private identity must be backed up and recovery-checked before the public
metadata is committed or used by an application release. Only the public JSON
may enter the repository. The bundle key must not be reused for intake-manifest
signing.

## Remaining work

- Select the production storage and password-vault backup destinations.
- Run the provisioning command as an explicit maintainer action.
- Verify the backup and independently review the public metadata.
- Commit only the public recipient metadata.
- Integrate recognized public key IDs into runtime encryption and later signed
  intake selection.

No production key, runtime, installer, HTTP, UI, Dropbox, service, hardware,
GPIO, I2C, transmitter, RF, or GitHub-posting behavior changed in this slice.
