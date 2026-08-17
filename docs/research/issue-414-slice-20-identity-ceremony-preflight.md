# Issue 414 Slice 20: Production Identity Ceremony Preflight

## Outcome

This slice adds a read-only maintainer preflight for a proposed production
bundle-encryption and intake-signing identity ceremony. It proves that the
explicit key IDs, tool paths, private storage roots, public metadata outputs,
and complete collision set satisfy the already qualified Slice 6 and Slice 13
provisioner boundaries before either provisioner may run.

The preflight reuses the exact provisioner key-ID, executable, private-directory,
repository-exclusion, ownership, and mode checks. Public outputs must have
distinct absolute paths in real owner-controlled directories outside both
private roots. It checks both future private finals, private partials, public
finals, and public partials for existing files, symlinks, or other collisions.

No executable is invoked and no file is created, opened for writing, modified,
renamed, or removed. Results contain only a typed status and the two public key
IDs; paths and exception text are not returned or printed. Unexpected local
inspection failures collapse to a non-disclosing `preflight_failed` status.

## Required ceremony evidence after later authorization

A future production ceremony is not complete until all of these are recorded:

1. Both generated public metadata files were independently reviewed for exact
   project, purpose, algorithm, key ID, public value, timestamp, and fingerprint.
2. Both distinct private identities were stored in the approved private root and
   backed up in the approved password vault.
3. Each vault item was independently restored—not copied from the live file—to
   a newly created disposable owner-only directory.
4. The restored Ed25519 identity successfully signed test bytes whose signature
   was verified with the reviewed public signing metadata.
5. A test archive encrypted to the reviewed age recipient was successfully
   decrypted byte-for-byte with the restored age identity.
6. All disposable restored identities, plaintext, ciphertext, and test
   signatures were removed through an explicitly authorized cleanup step.

The preflight does not collect or claim any of that evidence. Backup, restore,
recovery testing, and cleanup require their own bounded authorization.

## Validation

`make support-bundle-intake-identity-ceremony-preflight-test SUDO=` covers every
typed status, repeated ready evaluation with an unchanged filesystem snapshot,
unsafe executable/private/public paths and modes, symlinks, repository and
private-root overlap, identical outputs, every final/partial collision, hard-link
presence, and CLI non-disclosure. Tests use only fake executables and temporary
directories and run in Debian non-hardware CI.

## Remaining boundary

The maintainer must still choose and approve actual private roots, public-output
locations, production key IDs, and password-vault handling. Only then may this
preflight be run against those read-only inputs and the two production
provisioners be separately authorized.

No production key, metadata, vault item, repository, credential, network,
Dropbox, runtime, installer, HTTP, UI, service, Pi, hardware, or RF state changed.
