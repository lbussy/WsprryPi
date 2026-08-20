# Issue #414 Slice 43 — Contract Reconciliation

Date: 20 August 2026

## Outcome

The normative private-intake contract now matches the implemented version-1
readable manifest, receipt, signed-intake manifest, encryption filename,
review/finalization sequence, upload-reporting boundary, and maintainer
provider-filename privacy rule. Its implementation index covers Slices 1–43 and
separates completed implementation and non-RF qualification from the remaining
release/provider exercise.

Historical slice records remain evidence of their individual boundaries. Their
then-current future-work statements are not rewritten as present-day status.

## Reconciled boundaries

- The readable manifest lists the fields produced by the collector and excludes
  installation identity, routing, recipient, and key-fingerprint metadata.
- The operator chooses collection scope and support context before collection;
  changing either after review requires recollection under a new candidate.
- The downloaded receipt lists its exact version-1 fields and remains immutable
  if a GitHub issue is created later.
- The ciphertext filename includes both case and artifact identifiers.
- The signed-intake manifest lists its exact version-1 fields; signing key ID,
  algorithm, and signature remain in the detached signature envelope.
- Dropbox success is explicitly user-reported until confirmed by the maintainer.
- A provider-modified filename is untrusted personal metadata and is not copied
  into the canonical processing record.
- Local one-case retention deletion is implemented, but deletion from Dropbox,
  synchronized replicas, backups, downloads, and user-held copies is outside
  that transaction.

## Remaining qualification and documentation

The full provider exercise still requires a release build satisfying the signed
`minimum_upload_version`, a fresh signed-out Dropbox upload, and maintainer
receipt/inspection/promotion of that same artifact. No production upload was
performed in this slice.

Operator and maintainer documentation in the separate `Wsprry_Pi_Docs`
repository remains a separately authorized follow-up.

## Validation

The repository regression binds the normative document to the production
collector, receipt writer, and signed-intake validator. It also preserves the
filename privacy, receipt immutability, truthful upload state, and remaining
qualification statements.

Local validation:

```text
python3 -m py_compile scripts/tests/support_bundle_contract_reconciliation_test.py
make -C src support-bundle-contract-reconciliation-test \
  support-bundle-private-artifact-test \
  support-bundle-intake-validation-test SUDO=
git diff --check
```

All passed. The same three focused tests passed from an isolated source snapshot
under `/tmp` on `wspr4`. The snapshot intentionally excluded `.git`, so build
metadata generation reported that no Git repository was present; this did not
affect the focused tests. The production checkout remained on synchronized
`devel`, and `wsprrypi.service` remained active. The snapshot was then removed.

## Safety boundary

This slice changed documentation, a source/document regression, Make wiring,
and CI wiring only. It did not change runtime behavior, UI, schemas, production
manifests, keys, Dropbox, GitHub, installation, service state, hardware, GPIO,
or RF behavior.
