# Issue #414 Slice 46 — maintainer processing qualification

## Objective

Qualify the existing maintainer inspection and promotion transaction with the
real encrypted candidate retained by Slice 45. Independently acquire the
Dropbox-synchronized ciphertext, bind it to the retained receipt, decrypt and
inspect it with the production private identity, then promote private staged
copies as an active Issue #414 case.

## Scope and safety boundary

1. Verify the Dropbox ciphertext's exact size and SHA-256 against the receipt
   before decryption.
2. Use the receipt-selected production identity without printing or copying its
   contents. Plaintext may exist only in the inspector's owner-only temporary
   workspace and must be removed on every exit.
3. Validate the complete tar and manifest without extracting diagnostics or
   printing private context.
4. Stage separate owner-only copies in private Incoming, Processed, and work
   directories outside the repository. Run the Slice 38 transaction with
   retention class `active_case`.
5. Verify canonical names, modes, hashes, safe processing metadata, Incoming
   cleanup, and empty plaintext workspace.
6. Preserve the Dropbox source, the retained Downloads ciphertext and receipt,
   backups, unrelated user data, and all production keys.
7. Correct only portability or contract defects directly exposed by this real
   qualification. Add focused regressions and update maintainer research docs.

## Validation

Run Python syntax, focused inspection and processing suites, the real-age
fixture, repository Make targets, whitespace checks, and an isolated non-RF
Linux regression on `wspr4` if needed. Perform no installation, service change,
reboot, hardware access, GPIO operation, transmission, RF action, Dropbox move
or deletion, GitHub mutation, or diagnostic extraction.

## Exit criteria

The independently acquired ciphertext matches the receipt; production-key
inspection succeeds without retained plaintext; the bounded transaction
publishes one canonical `active_case`; all retained sources remain unchanged;
actionable adversarial findings are closed; and attributable changes are
committed and pushed only to the Issue #414 integration branch.
