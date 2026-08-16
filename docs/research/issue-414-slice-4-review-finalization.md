# Issue 414 Slice 4: Readable Review and Finalization

Status: Implemented through immutable readable bytes; encryption and upload deferred

Depends on:

- [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)
- [Slice 2 local artifact primitives](issue-414-slice-2-local-artifacts.md)
- [Slice 3 candidate manifest and lifecycle](issue-414-slice-3-candidate-lifecycle.md)

## Outcome

The guarded local HTTP API and Maintenance interface now expose the private
candidate workflow. The application generates the case ID, requires useful
support context, passes private description/contact values to the collector in
owner-only staging files rather than process arguments, and verifies that the
collector returns the same case ID with an internal manifest.

The user downloads and retains a readable `.tar.gz` candidate, reviews it
locally, and explicitly approves its exact bytes. Finalization reopens and
rehashes the candidate, rejects mutation or unsafe metadata, changes it from
`0600` to `0400`, and retains an open descriptor. Repeated finalization is
idempotent. Explicit deletion and the existing 24-hour expiry remove the whole
job directory.

## HTTP and UI boundary

Private creation accepts exactly one of an existing WsprryPi issue URL or a
bounded problem description and contact value. Private snapshots expose only
the case ID and lifecycle state; they do not echo issue URLs, descriptions,
contacts, paths, filenames, or digests.

The Maintenance workflow is inline rather than modal. It makes active I2C
probing opt-in, retains the candidate after download, explains the browser save
boundary, and requires a review checkbox before finalization. Legacy creation
with only `probe_i2c` remains compatible.

## Validation

Passed on the macOS development host:

- support-bundle job-manager, collector-executor, HTTP, and download-file tests;
- WsprryPi-UI unit suite and UI source-regression test;
- JavaScript syntax validation and `git diff --check`;
- Impeccable detector plus desktop and 390-pixel mobile render inspection.

The runtime target remains Linux-specific because its existing secure job-ID
generator uses `getrandom`; that target was not represented as macOS-qualified.
No hardware, GPIO, I2C, transmitter, RF, installation, service, or remote
network activity was performed.

## Remaining work

- Encrypt the finalized bytes with the project bundle-encryption public key.
- Produce the encrypted-artifact receipt and verify a packaged `age` round trip.
- Retrieve and verify the signed intake manifest with version gating and URL
  rotation.
- Upload only the encrypted artifact through Dropbox and implement bounded
  retry/rate-limit behavior.
- Implement the optional GitHub issue handoff and operator documentation.

No encryption, Dropbox, GitHub posting, signed-intake retrieval, installer, or
remote-network behavior is implemented by this slice.
