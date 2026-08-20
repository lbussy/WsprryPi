# Issue #414 Slice 45 — Signed-out Encrypted Upload Qualification

Date: 20 August 2026

## Outcome

One fresh Issue #414 support candidate was collected on `wspr4` without the
optional I2C probe. Case `KPXV-ZKYQ-8P7J` produced a readable archive containing
safe relative member paths, 120 manifest-declared diagnostic files, project
`wsprrypi`, canonical project version `3.2.1-qualification.2`, and the expected
existing-issue correlation. The downloaded 108431-byte archive had SHA-256
`86834b1bdd700d45d86f8c3788232deaaea6cfee66ff61ef764ebc6bd55e0ce3`.
A focused assignment scan found no unredacted callsign, locator, email,
password, token, secret, or API-key assignment.

The exact reviewed bytes were finalized and encrypted locally under public key
ID `wsprrypi-bundle-2026-01`. The 108647-byte ciphertext had SHA-256
`400c97f7384312792aab1526353d970e6524f60372684a52541f09e2fcd09b3a`;
the downloaded receipt recorded the same archive identity, ciphertext identity,
case, artifact ID, key ID, and Issue #414 correlation.

A fresh Chrome incognito session showed the Dropbox File Request signed out.
Only the `.age` ciphertext was selected. After the approved uploader name and
contact were entered, Dropbox displayed `Finished uploading`. The readable
archive and receipt were not transmitted. The Pi workflow subsequently recorded
`upload_reported_complete`, which remains an end-user report rather than
maintainer confirmation.

The ciphertext and receipt are retained with directory mode `0700` and file
mode `0600` under `~/Downloads/Issue-414-KPXV-ZKYQ-8P7J` for the next
maintainer-processing slice. Their retained SHA-256 values match the qualified
downloads. The Dropbox request ID and URL are intentionally absent from this
record.

## Findings closed

The first fresh candidate revealed that the collector wrote project version
`unknown`. Commit `09e397e8e20030d75e7f6ca64f961a651c2ffcf8` now passes the
canonical runtime build version through the private collector boundary and
rejects missing or noncanonical values. The corrected candidate proved the
manifest carries `3.2.1-qualification.2`.

Live route qualification then found that Apache proxied the bundle API but not
the signed intake endpoint, leaving the UI dependent on its direct-port
fallback. Commit `b86a26d88d1fc98b511900f8044a92792bedda86` adds the exact
`/wsprrypi/api/support-intake` forward and reverse mappings to both the
canonical vhost and installer-managed block. It retains the narrow endpoint
boundary: no broad `/api` proxy, CORS relaxation, or Host/Origin rewriting.
Focused shell and C++ regressions enforce both mappings.

A 390-pixel headless-Chrome image initially appeared to clip the refresh
dialog. Comparison proved Chrome had rendered its 500-pixel minimum layout and
cropped the bitmap; a 500-pixel responsive render showed both actions, correct
wrapping, and no horizontal overflow. No UI source change was warranted.
Impeccable detection reported only pre-existing advisory design-token drift.

## Installation and runtime evidence

Clean isolated clones used local-only exact tags
`v3.2.1-qualification.2` and `v3.2.1-qualification.3` on branch `main`.
The guarded installers completed and created `~/finished`. The final installed
binary reports `3.2.1-qualification.3`; `wsprrypi.service` is active and
enabled, Apache configuration syntax is valid, and `Transmit = false` remains
configured. Both the same-origin Apache intake route and the direct port 31415
fallback return the same authenticated active generation, minimum upload
version, signing key ID, and bundle key ID.

Restart startup cleanup removed the completed Pi-side job directory according
to the existing startup-cleanup policy. This does not affect the completed
Dropbox upload or the privately retained ciphertext and receipt, but it means
the next slice must use those retained files rather than the Pi job endpoint.

## Validation

Local macOS validation passed collector, runtime, manager, HTTP, server-wiring,
contract-reconciliation, UI source/integration, Apache proxy, shell syntax,
JavaScript, whitespace, archive structure, manifest, checksum, and receipt
checks. Linux validation on `wspr4` passed the collector and focused support
bundle tests, Apache proxy regression, UI source regression, release builds,
exact-version checks, installer, Apache syntax, service checks, and both intake
routes. Dropbox's signed-out success page supplied the external upload evidence.

Desktop and narrow responsive maintenance-page renders were inspected. No UI
source was changed in this slice. No reboot, I2C probe, GPIO operation,
transmitter operation, RF output, production-key change, manifest publication,
Dropbox deletion, or support-bundle decryption occurred.

## Documentation Impact

This prompt and implementation record document the developer qualification.
The separate operator-documentation repository was considered but not changed
because cross-repository documentation work was not authorized. Operator and
maintainer instructions for collection, signed-out upload, receipt retention,
and processing remain required there.

## Remaining boundary

The next slice is the maintainer-processing qualification using the retained
ciphertext and receipt: independently acquire the Dropbox copy, verify receipt
and ciphertext identity, decrypt with the production private identity, inspect
the readable bundle, and exercise the bounded processing/promotion transaction.
That slice must not delete the Dropbox source, retained local handoff, backups,
or unrelated user data without separate explicit authorization.
