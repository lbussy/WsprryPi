# Issue #414 Slice 38 — maintainer processing and promotion

## Outcome

A maintainer-only processor now promotes a Slice 37-inspected ciphertext and
receipt into a canonical private case directory. It binds the exact incoming
files with no-follow descriptors, revalidates their identity after inspection,
parses and copies the same receipt descriptor, verifies exact ciphertext and
receipt bytes, and publishes a synchronized case transaction without overwrite.

The processing record contains only safe correlation and operational metadata.
Its optional received-filename field is null because a Dropbox-assigned name
cannot be proven free of uploader identity.
It records the retention classification and review/expiration time but does not
delete retained cases. Output is limited to a typed status, case ID, and artifact
ID.

## Transaction states

- `processed` means the case was published and synchronized, then both exact
  Incoming objects were removed and Incoming was synchronized.
- `unchanged` means an independently revalidated existing case matched and a
  retry completed or confirmed cleanup.
- `processed_cleanup_pending` means the Processed case is valid but at least one
  exact Incoming object could not be removed or synchronized.
- `committed_sync_uncertain` means atomic rename completed but Processed directory
  synchronization failed; Incoming is deliberately retained for a later retry.
- Pre-commit inspection, validation, copy, collision, or publication failures do
  not remove Incoming.

Maintainer confirmation remains a separate human action. Diagnostic extraction,
opening individual diagnostics, GitHub updates, Dropbox automation, and automatic
retention deletion remain future work.

## Safety boundary

Incoming and Processed must be distinct owner-only `0700` directories outside the
repository. Files must be direct owner-owned regular single-link children of
Incoming. The processor never derives a path from the Dropbox filename, follows
a symlink, executes a shell, exposes an identity, or prints exceptions, paths,
diagnostics, private support context, or subprocess output. A Processed-directory
lock serializes cooperating writers through transition validation, publication,
synchronization, and the cleanup decision.

## Validation

The twelve-test focused suite covers successful canonical promotion, directory/input and
repository-boundary checks, inspection failure, post-inspection replacement,
collisions, strict records, same-case mutation, idempotent and cleanup-pending
retry, retention classes, injected write/fsync/close/rename/directory-sync
failures, preservation of Incoming, partial cleanup, and output non-disclosure.
The Slice 37 suite remains the real decryption and archive-validation authority.

Python syntax checks, the Slice 37 inspection regression, the focused processor
suite, and the focused Make target passed on macOS. The isolated `wspr4` snapshot
passed all twelve processor tests and all seven Slice 37 tests, including the real
Debian packaged-age fixture. The snapshot was removed and
`/home/pi/WsprryPi` remained clean on `devel`. The expected build-metadata warning
occurred because the isolated source snapshot intentionally had no `.git`
directory. No production bundle, installation, service, reboot, hardware, GPIO,
or RF operation was performed.

## Documentation impact

The private-intake implementation plan points to Slice 38 and this maintainer
record distinguishes inspection, publication, cleanup-pending, confirmation,
retention metadata, and future deletion/extraction. The separate operator-docs
repository is reviewed but unchanged because cross-repository modification was
not authorized and this workflow remains maintainer-only.
