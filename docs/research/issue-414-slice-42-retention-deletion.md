# Issue #414 Slice 42 — explicit retention deletion transaction

## Outcome

A maintainer-only tool can now delete one explicitly selected canonical
Processed case after independently revalidating it as retention-due. Production
uses the system UTC clock and requires exact confirmation text containing the
case and artifact IDs; time injection exists only in the typed test seam.

Under an exclusive Processed lock, the transaction invokes Slice 41's complete
case validator, rejects retained, active, unsafe, mutated, mismatched, ambiguous,
or colliding cases, and renames the selected directory without overwrite to a
canonical hidden tombstone. It synchronizes Processed before cleanup, deletes
only descriptor-revalidated canonical files with the processing record last,
synchronizes the tombstone, removes it, and synchronizes Processed again.

Retries resume exact partial tombstones while the retained processing record
continues to prove selection, due policy, and safe correlation. An empty
tombstone can complete directory cleanup. Missing source and tombstone is a
truthful idempotent `absent` result after a confirming directory sync. Statuses
distinguish cleanup pending from a committed deletion whose final directory sync
is uncertain.

Output contains only typed status and safe case/artifact identifiers. It does not
print paths, Dropbox names, URLs, hashes, diagnostics, private context, keys,
exceptions, or file contents.

## Validation

The focused fifteen-test fixture suite covers success, idempotent absence, unrelated-case
preservation, exact confirmation, invalid requests, due boundaries, active-case
rejection, exclusive locking, rename failure, post-rename sync uncertainty,
payload/record/rmdir restart boundaries, final-sync uncertainty, mutation,
unexpected inventory, descriptor/path substitution, unsafe resumed metadata,
and CLI non-disclosure. Slice 41 audit and Slice 38 processing coverage remain
authoritative for complete-case validation and promotion transactions.

Python syntax, focused Make targets, and diff checks passed on macOS. An
isolated `wspr4` snapshot passed all fifteen deletion, ten audit, twelve
processing, and seven inspection tests. Its expected build-metadata warning
reflected the intentionally omitted `.git` directory. The snapshot was removed,
the primary `devel` checkout remained clean, and the installed service remained
active.

All deletion tests operate only inside automatically removed temporary
directories. No production Processed, Incoming, Dropbox, backup, repository,
service, hardware, or RF state is modified.

## Documentation impact

The private-intake roadmap now records bounded destructive retention enforcement.
The separate operator-documentation repository remains unchanged because
cross-repository modification was not authorized. A maintainer runbook remains
required before routine operational use, including backups and Dropbox-retention
limitations that this local tool cannot enforce.

## Remaining boundary

This transaction deletes only the local canonical Processed case. It cannot erase
Dropbox history, synchronized replicas, downloaded copies, backups, or user-held
artifacts. It does not perform bulk deletion or automatically transition active
cases to resolved. Signed-out provider qualification, final contract
reconciliation, and operator/maintainer documentation remain.
