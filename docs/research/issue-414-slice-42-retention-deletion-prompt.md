# Issue #414 Slice 42 — explicit retention deletion transaction

## Objective

Add a maintainer-only, one-case deletion transaction for a canonical Processed
support bundle that Slice 41 independently validates as due. Make deletion
explicit, exclusive, fail closed, resumable after partial cleanup, and truthful
about synchronization uncertainty.

## Scope and contract

Require an absolute owner-only Processed directory, canonical case ID, artifact
ID, and exact confirmation text containing both identifiers. Production obtains
the evaluation time from the system UTC clock; only the typed test seam may
inject time.
Under an exclusive Processed lock, revalidate the complete case through Slice 41
and reject active/not-yet-due, unsafe, mutated, mismatched, or ambiguous cases.

Atomically rename the selected case without overwrite to a canonical hidden
retirement tombstone and synchronize Processed before deleting content. Delete
only the three canonical owner-only files, with the processing record last. Make
an exact tombstone retry safely resume partial deletion; reject simultaneous
source/tombstone presence, unexpected inventory, unsafe nodes, or missing record
before payload cleanup. Synchronize the tombstone after file deletion and
Processed after directory removal. Distinguish deleted, resumed, cleanup-pending,
and committed-sync-uncertain outcomes.

Output only typed status and safe case/artifact IDs. Never print paths, Dropbox
names, URLs, hashes, diagnostics, private context, keys, exceptions, or contents.

## Safety and non-goals

Exercise deletion only in temporary test fixtures. Do not operate on production
Processed or Dropbox storage, backups, Incoming, repositories, services,
hardware, or RF. Do not automate lifecycle transitions or bulk deletion.

## Validation and exit criteria

Cover confirmation, due boundaries, active/retained rejection, exact selection,
exclusive locking, no-overwrite rename, directory synchronization, every partial
deletion restart boundary, file/path substitution, unexpected inventory,
mutation, idempotent absent result, typed failures, non-disclosure, and unrelated
case preservation. Run Slice 41/42 and Slice 38 tests locally and in an isolated
Debian snapshot, adversarially reassess until clean, document the outcome, update
the roadmap, and commit/push only attributable files.
