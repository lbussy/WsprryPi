# Issue #414 Slice 44 — release-like active-intake qualification

## Objective

Prepare and install a truthful local-only qualification build that satisfies the
signed `minimum_upload_version` without moving or publishing the existing
`v3.2.0` tag.

## Scope

Reconcile the clean Issue #414 integration branch with current `devel`. In an
isolated clone, use a release-like `main` context and exact local-only tag
`v3.2.1-qualification.1`; prove the binary reports that version. Run focused
local and target non-hardware tests, install it on `wspr4` through the guarded
installer workflow, and verify active signed intake with transmission disabled.

## Constraints and non-goals

Preserve all ordinary worktrees and production data. Do not move or push a
release tag, modify the signed manifest, disclose its request URL, upload a
bundle, touch Dropbox, process or delete support data, reboot, enable
transmission, or exercise GPIO, I2C, transmitter hardware, or RF.

## Acceptance

Require exact version provenance, successful completion marker, active/enabled
service, disabled transmission, safe tool/state ownership, HTTP 200 `active`
intake, clean checkouts, removal of isolated clones and the local-only tag, and a
clean adversarial review. Commit and push only the integration merge and
attributable Slice 44 documentation.
