# Issue #414 Slice 41 — maintainer retention eligibility audit

## Outcome

A maintainer-only read-only tool now audits canonical Processed cases and reports
which have reached their recorded retention review time. It requires an explicit
owner-only Processed directory and exact UTC evaluation time, holds a shared
transaction lock, and produces stable safe-identifier output.

The audit requires canonical case-directory correlation, exact three-file
inventory, owner-only regular single-link files, a strict processing record, an
exact matching receipt, and ciphertext size/SHA-256 agreement. It independently
revalidates that uncorrelated review is exactly 14 days after processing,
resolved-case review is an integral 30–90 days after processing, and active cases
have no review timestamp. Enumeration is bounded to 4096 cases.

Invalid time, unsafe storage, unexpected entries, malformed or mismatched
metadata, and integrity failures fail closed without partial results. Output is
limited to typed status/counts and case/artifact IDs; it excludes paths, Dropbox
names, issue URLs, hashes, diagnostics, private context, key material, file
contents, and exceptions.

## Validation

The focused ten-test suite covers the due boundary, retained and active cases,
stable order, canonical UTC, unsafe root/case metadata, unexpected inventory,
bad names, duplicate records, policy/date tampering, ciphertext mutation,
receipt correlation, shared-lock writer exclusion, zero mutation, and CLI
non-disclosure. Slice 38's twelve processing tests remain green. Python syntax,
the focused Make targets, and final diff checks passed on macOS. An isolated
`wspr4` snapshot passed all ten audit, twelve processing, and seven inspection
tests; its expected build-metadata warning reflected the intentionally omitted
`.git` directory. The snapshot was removed and the primary `devel` checkout
remained clean. No installation, service, hardware, or RF operation occurred.

## Documentation impact

The implementation plan now distinguishes implemented retention eligibility
auditing from destructive enforcement. The separate operator documentation
repository remains unchanged because this is maintainer-only tooling and
cross-repository modification was not authorized. A future maintainer runbook
must cover audit invocation, interpretation, lifecycle transition, and deletion.

## Remaining boundary

This slice never deletes, renames, moves, extracts, decrypts, or changes a case.
A later destructive slice must define explicit selection, confirmation,
exclusive locking, crash recovery, synchronization truthfulness, and backup/
provider limitations before deleting any eligible Processed case.
