# Issue #414 Slice 37 — maintainer intake inspection

## Outcome

A maintainer-only offline tool now validates a downloaded receipt and received
ciphertext before decrypting, selects the identity by the receipt's public key ID,
streams decryption into a bounded private temporary workspace, verifies the exact
plaintext archive size and SHA-256, and validates the complete tar and internal
manifest without extracting diagnostics.

Successful output contains only the case ID, artifact ID, bundle key ID, and an
existing GitHub issue URL or `private-context-only`. Failure output is a typed
status. Identity contents, private support context, diagnostics, filesystem paths,
`age` stderr, and exception text are not printed.

## Safety boundary

The inspector rejects malformed or duplicate receipt/manifest JSON; unsafe files
or executables; key-ID mismatch; ciphertext mutation; decryption failure, timeout,
or expansion beyond the receipt size; archive size/hash mismatch; unsafe or
duplicate tar paths; links and special nodes; permission-escalation bits; and
file-count, individual-file, or total-expansion limit violations. It hashes every
declared diagnostic member and requires the manifest inventory to match the
archive exactly. Receipt JSON is bounded independently to 16 KiB; the bundle
manifest is bounded to 256 KiB so its inventory can represent the permitted file
count. Plaintext exists only under an owner-only temporary directory
and is removed on success and every failure.

## Operational use

The maintainer supplies explicit absolute paths to the ciphertext, receipt,
matching `<bundle-key-id>.age-identity.txt` with exact owner-only mode `0400` or
`0600`, and an owner-only `0700` work directory. Production defaults to the
fixed Debian `/usr/bin/age`; a maintainer on another platform may explicitly
select an absolute owner- or root-owned regular executable with no symlink or
group/world write permissions. The tool does not search a
Keychain, password vault, Dropbox directory, or repository for identities.

## Validation

The focused Python suite covers valid correlation, strict receipt parsing,
ciphertext/archive mismatch, identity mismatch, decrypt failure/timeout/oversize,
unsafe archive paths/nodes/modes/duplicates, manifest mismatch, undeclared files,
cleanup, and CLI non-disclosure. On Debian with packaged age, it also generates an
ephemeral test identity, encrypts and decrypts a genuine fixture, then proves that
authenticated decryption rejects tampering.

- macOS Python syntax and focused tests passed with resource warnings promoted to
  errors. The real fixture passed with Homebrew `age`/`age-keygen` 1.3.1.
- The focused Make target passed locally; its default real-age case is skipped on
  macOS because production intentionally fixes the CLI to `/usr/bin/age`.
- The authorized isolated `wspr4` snapshot ran all seven tests, including the
  genuine Debian packaged-`age` 1.2.1 round trip and tamper rejection. The source
  snapshot lacked `.git`, so the included build-metadata helper emitted its
  expected non-repository warning before the focused Python target passed.
- The isolated snapshot was removed, and `/home/pi/WsprryPi` remained clean on
  `devel`. No installation, service, reboot, hardware, GPIO, or RF action occurred.

## Documentation impact

The implementation plan now points to this slice. The separate operator
documentation repository remains unchanged because this is a maintainer-only
workflow and cross-repository work was not authorized. A future maintainer runbook
must document identity restoration, receipt pairing, inspection output, and
retention decisions.

## Remaining boundary

Inspection deliberately does not extract diagnostics, update GitHub, access
Dropbox, move artifacts from `Incoming` to `Processed`, or apply retention. A
later slice may add an explicit processing/promotion transaction after this
inspection result is independently reviewed.
