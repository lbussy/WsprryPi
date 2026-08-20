# Issue #414 Slice 41 — maintainer retention eligibility audit

## Objective

Add a maintainer-only, read-only audit that determines which canonical Processed
support-bundle cases have reached their recorded retention review time. Establish
strict discovery and validation before any destructive retention operation.

## Scope and contract

Create a standalone Python tool that accepts an explicit absolute owner-only
`Processed` directory and an explicit canonical UTC evaluation time. Inspect only
direct canonical case directories created by Slice 38. Require owner-only
directory/file metadata, no links, the exact three-file case inventory, and a
strict valid processing record. Classify valid cases as `retained` or `due`;
`active_case` is retained until a later explicit lifecycle transition. Classify
unknown, malformed, unsafe, or unexpected entries as `unsafe` and fail the audit.

Output only typed summary counts and safe case/artifact identifiers. Do not print
paths, filenames received from Dropbox, issue URLs, hashes, diagnostics, private
context, key material, exception text, or file contents. Hold a shared lock on
Processed throughout discovery and validation so the audit cannot observe a
cooperating promotion transaction halfway through publication.

## Non-goals and safety

Do not delete, rename, move, extract, decrypt, open, upload, download, contact
Dropbox or GitHub, change retention metadata, or operate on production storage.
Do not install, restart services, reboot, access GPIO/I2C/transmitter hardware, or
perform RF activity. Deletion and lifecycle transitions require later separately
reviewed slices.

## Validation and exit criteria

Add deterministic tests for due/not-due boundaries, active cases, exact UTC,
directory and file safety, exact inventory, duplicate/unknown/malformed records,
canonical naming correlation, lock behavior, stable ordering, output
non-disclosure, and zero mutation. Wire a focused Make/CI target. Run Python
syntax, focused Slice 38/41 tests, diff checks, and an adversarial review; correct
all attributable findings. Record the implementation and update the roadmap,
then commit and push only Slice 41 files on the current Issue #414 branch.
