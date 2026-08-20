# Issue #414 Slice 38 — maintainer processing and promotion transaction

## Objective

Add a maintainer-only, fail-closed transaction that promotes an independently
inspected encrypted support bundle from `Incoming` to `Processed` and records
durable, non-sensitive processing metadata.

## Required boundary

The tool accepts explicit absolute paths for owner-only `Incoming`, `Processed`,
and private-work directories, the ciphertext, receipt, and matching age identity.
It uses the Slice 37 production inspector as the sole bundle-validation authority.
Inputs must be direct, owner-owned, regular, single-link children of `Incoming`;
directories must be distinct, mode `0700`, non-symlinks, and outside the repository.

Publish a private `case-<case-id>-<artifact-id>` directory containing canonical
ciphertext, receipt, and strict processing-record filenames. The record contains
only validated correlation, ciphertext integrity, safe received-name advisory,
UTC processing time, lifecycle, and retention metadata. It excludes private
context, diagnostics, identity paths, Dropbox identity, request URLs, and keys.

Publication must use descriptor-bound copying, exact size/hash verification,
file and directory synchronization, temporary-identity revalidation, serialized
writers, no overwrite, atomic case-directory rename, and truthful uncertain-sync
reporting. Preserve `Incoming` until durable publication. Cleanup removes only
the exact validated input objects; failures produce a retryable cleanup-pending
state. Exact existing processed cases are idempotent, while mutation conflicts
fail closed.

Retention classes are `uncorrelated` (14 days), `active_case` (review required,
no short expiration), and `resolved_case` (30–90 days). This slice records the
decision only and performs no automatic retention deletion.

## Validation and exclusions

Add deterministic success, validation, race/substitution, collision, copy,
synchronization, publication, retry, retention, strict-record, cleanup, and
non-disclosure tests; a focused Make target; Debian non-hardware CI wiring; and
isolated `wspr4` validation. Do not access Dropbox or GitHub APIs, extract
diagnostics, process a real support bundle, install, operate services, reboot,
access GPIO or transmitter hardware, or transmit. Commit and push only this
slice's attributable files on the current Issue #414 branch after adversarial
review finds no actionable blocker.
