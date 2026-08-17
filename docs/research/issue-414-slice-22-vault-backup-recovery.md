# Issue 414 Slice 22: Dashlane Backup and Recovery Qualification

Status: Production private identities backed up and recovery qualified

## Outcome

The two Slice 21 production private identities are stored as exact file
attachments in separate, unshared Dashlane Secure Notes:

- `WsprryPi Manifest Signing Identity — wsprrypi-intake-2026-01`
- `WsprryPi Bundle Decryption Identity — wsprrypi-bundle-2026-01`

The signing attachment is named
`wsprrypi-intake-2026-01.ed25519-private.pem.txt` because Dashlane would not
accept the original `.pem` extension. Only the attachment name changed. Its
downloaded bytes matched the live `.pem` identity exactly. The bundle attachment
retained `wsprrypi-bundle-2026-01.age-identity.txt`.

Both notes identify their purpose, key ID, creation timestamp, public
fingerprint, original filename, attachment filename where different, warning
against editing/reformatting, and recovery-qualified state. Neither note is
shared or placed in a shared collection. The user confirmed both notes were
updated with `Recovery tested: 2026-08-17T13:18:35Z`.

Dashlane documents that macOS Secure Notes can attach and individually download
files, while attachments are excluded from DASH and CSV exports. Therefore this
qualification used individual attachment download rather than vault export:
[Dashlane Secure Note attachments](https://support.dashlane.com/hc/en-us/articles/202699381-Add-and-manage-Secure-Notes-in-Dashlane).

## Recovery evidence

Each attachment was independently downloaded from Dashlane into a newly created
owner-only `0700` disposable directory. Before use, both restored files were
owner-owned, regular, single-link mode `0400` files with the expected sizes.

The recovery tests established:

- each restored private identity matched its live identity byte for byte;
- the restored Ed25519 identity signed fixed test bytes;
- that signature verified against the reviewed Slice 21 public signing metadata;
- fixed plaintext encrypted to the reviewed age recipient;
- the restored age identity decrypted the ciphertext byte for byte; and
- no private bytes or private-file hashes were printed or committed.

After the user confirmed both Dashlane notes were updated, the user separately
confirmed permanent deletion of the exact disposable recovery directory. Its
absence was verified. The two live external identities remain owner-owned,
single-link mode `0400` files.

## Publication boundary

The identities are now backup-qualified and recovery-qualified. This does not
publish or activate them. Slice 23 subsequently versions the reviewed public
metadata and its deterministic production trust header. The next separately
reviewed work is to create and configure the public `WsprryPi/support-intake`
repository, prepare/sign/publish the first manifest, verify its public bytes,
and only then consider runtime activation.

At completion of Slice 22, no private identity, restored copy, private hash,
attachment, recipient, local absolute path, credential, or generated production
metadata had been committed. No GitHub/Dropbox administration, application
runtime, installer, HTTP, UI, service, Pi, hardware, or RF state changed.
