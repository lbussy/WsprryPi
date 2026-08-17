# Issue 414 Slice 22 Execution Prompt: Dashlane Backup and Recovery Qualification

## Objective

Back up the two Slice 21 production private identities as exact file attachments
in two dedicated, unshared Dashlane Secure Notes; independently download each
attachment into a disposable owner-only recovery directory; prove restored-key
signing and encryption round trips; remove disposable recovery material; and
record only non-secret evidence.

## Preconditions and confirmation

- Dashlane is the sole installed password vault and is already authenticated.
- Current Dashlane macOS functionality supports Secure Note attachments and
  downloading/exporting those attachments.
- Because attaching private identities transmits sensitive files to Dashlane,
  obtain action-time user confirmation naming both files and Dashlane before the
  first attachment upload.

## Requirements

1. Create two unshared Secure Notes with exact titles:
   - `WsprryPi Manifest Signing Identity — wsprrypi-intake-2026-01`
   - `WsprryPi Bundle Decryption Identity — wsprrypi-bundle-2026-01`
2. Each note SHALL state purpose, key ID, creation timestamp, public SHA-256
   fingerprint, original filename, `Backup status: pending recovery test`, and a
   warning not to edit or reformat the attachment. Do not paste private bytes.
3. Attach only the matching private file to each note. Do not share either note,
   add it to a shared collection, or attach public metadata unnecessarily.
4. Confirm Dashlane shows each attachment and saved note before proceeding.
5. Download/export each attachment from Dashlane—not from the live private-key
   path—to a new disposable owner-only recovery directory. Preserve exact
   filenames and require regular owner-owned single-link `0400` files before use.
6. With the restored signing identity, sign fixed test bytes and verify the
   signature with the Slice 21 reviewed public signing metadata.
7. Encrypt fixed test bytes to the reviewed age recipient and decrypt them with
   the restored bundle identity; require byte-for-byte equality.
8. Independently compare restored files byte-for-byte and by SHA-256 with the
   live identities. Do not print private bytes or private-file hashes.
9. Update both Secure Notes to `Backup status: recovery qualified` with the UTC
   test timestamp. Do not attach test artifacts.
10. Remove the exact disposable recovery directory and verify its absence.
11. Commit only this prompt and a non-secret record containing key IDs, public
    fingerprints, test timestamp, typed results, and Dashlane note titles.

## Failure and cleanup policy

- Stop on any unexpected prompt, sharing state, attachment mismatch, download
  ambiguity, permission error, cryptographic failure, or UI uncertainty.
- Do not delete or replace a live production identity.
- Do not delete a Dashlane note or attachment without separate confirmation;
  Dashlane documents attachment deletion as irreversible.
- If recovery material exists after a failure, report its exact directory and
  request cleanup authorization rather than broadening deletion.

## Non-goals

- Do not export the Dashlane vault; attachments are not included in DASH/CSV
  exports and are qualified only through individual download.
- Do not compile/commit production trust, create/contact the public repository,
  publish metadata, prepare a manifest, activate runtime/UI/services, use
  Dropbox, access a Pi, or exercise hardware/RF.

## Exit criteria

Both exact private identities are attached to separate unshared Dashlane Secure
Notes, independently restored, cryptographically qualified, marked recovery
qualified, and all disposable recovery material is removed.
