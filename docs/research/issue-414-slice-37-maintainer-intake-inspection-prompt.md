# Issue #414 Slice 37 — maintainer intake inspection

## Objective

Add a maintainer-only, offline inspection boundary for a received encrypted
support bundle and its downloaded receipt. Authenticate the received ciphertext,
decrypt it with an explicitly selected project identity, validate the complete
archive and internal manifest without extraction, and report only safe correlation
metadata.

## Scope

1. Add `scripts/maintainer/inspect_received_support_bundle.py`.
2. Strictly validate the receipt schema, WsprryPi identifiers, filenames, sizes,
   hashes, upload state, issue URL, and bundle key ID before decryption.
3. Require the identity filename to match the receipt-selected key ID; require a
   private owner-only identity file and a fixed safe `age` executable.
4. Verify the ciphertext's exact size and SHA-256 before invoking `age` through a
   shell-free argv.
5. Stream decrypted bytes into an owner-only temporary workspace with an exact
   receipt-sized bound and timeout. Delete plaintext on every exit.
6. Verify the decrypted archive's exact size and SHA-256.
7. List and validate the gzip tar without extracting. Reject absolute/traversal
   paths, backslashes, duplicate paths, links, devices, FIFOs, sockets, permission
   escalation bits, excessive file count, excessive per-file size, and excessive
   total expansion.
8. Strictly validate `bundle/manifest.json`, correlate project/case ID and every
   declared file's path, size, and SHA-256 with the archive.
9. Print only a typed status plus case ID, artifact ID, key ID, and normalized
   issue reference when inspection succeeds. Never print private context,
   diagnostic bytes, paths, age stderr, identity contents, or exceptions.

## Tests and evidence

- Add focused Python tests for strict receipt parsing, ciphertext mutation,
  key-selection mismatch, decrypt failure/timeout/oversize, cleanup, every unsafe
  tar node/path/mode/limit, manifest mismatch, undeclared/duplicate content,
  success correlation, and CLI non-disclosure.
- Add a real Debian packaged-`age` success/tamper fixture with ephemeral test keys;
  no production private identity may enter tests or repository content.
- Wire a focused Make target and Debian non-hardware CI step.
- Run local Python syntax/unit checks, the real-age fixture where available,
  `git diff --check`, and the focused target on the authorized isolated `wspr4`
  snapshot.

## Non-goals

- Extracting diagnostic files.
- Moving Dropbox files from `Incoming` to `Processed`.
- Dropbox API access, retention deletion, maintainer notification, or GitHub API
  updates.
- Keychain/password-vault access or automatic identity selection.
- End-user UI, installer, service, hardware, GPIO, reboot, or RF changes.

## Exit criteria

Malformed, mutated, mismatched, malicious, oversized, or undecryptable input fails
closed without retained plaintext or sensitive output; a genuine ephemeral-age
fixture validates end to end; documentation distinguishes inspection from later
processing/retention; and the reviewed slice is committed and pushed only to the
current Issue #414 integration branch.
