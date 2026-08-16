# Issue 414 Slice 3: Candidate Manifest and Lifecycle Prompt

## Objective

Connect the existing collector to the private-intake protocol at the readable
candidate boundary without changing the current HTTP or UI contract.

## Approved scope

- Add an opt-in collector interface for case ID and support context.
- Generate `bundle/manifest.json` before compression for private candidates.
- Record the case ID and manifest presence in the external result JSON.
- Extend result validation and job-manager snapshots with private-candidate
  metadata and an internal lifecycle state.
- Preserve existing collection behavior when private metadata is absent.
- Add focused shell and C++ regression coverage and durable implementation notes.

Do not add HTTP request fields, new routes, UI changes, encryption invocation,
remote intake, Ed25519, Dropbox, GitHub posting, installer dependencies, or
operator-documentation changes. Do not operate hardware, services, GPIO, I2C,
or RF.

## Private collector interface

Private mode requires a valid case ID plus exactly one context path:

1. `--github-issue URL`, normalized to the WsprryPi issue namespace; or
2. `--context-kind new_github_issue|no_github` together with absolute private
   `--problem-description-file` and `--contact-file` inputs.

Description and contact values must never be command-line arguments. Context
files must be regular, non-symlink, owned by the invoking user, not group/world
writable, bounded, non-empty, and free of control characters. Legacy mode
accepts none of these options and does not add a protocol manifest.

## Manifest requirements

- UTF-8 JSON schema and contract version 1.
- Project ID `wsprrypi`, display version, case ID, RFC 3339 UTC creation time,
  collection options, privacy categories, support context, warnings, and files.
- `files` contains every regular payload file except `manifest.json`, sorted
  bytewise by relative POSIX path with exact size and lowercase SHA-256.
- The collector rejects links or non-regular payload nodes before archiving.
- No context staging file is copied into the archive.
- Manifest generation failure leaves no successful archive or sidecar.

## Compatibility and lifecycle

- Existing result JSON without private fields remains valid.
- A private result requires both a valid `case_id` and
  `manifest_included: true`; partial or contradictory metadata is invalid.
- A validated private result gives the job snapshot its case ID and internal
  lifecycle `candidate_ready`.
- Legacy success keeps internal lifecycle `legacy_ready`.
- Failure, deletion, and expiry clear private candidate metadata.
- Current HTTP serialization remains unchanged and continues to report
  `succeeded`; the internal lifecycle is not yet a public API.

## Validation

- Extend collector tests for GitHub, non-GitHub, malformed/missing/conflicting
  context, file safety, manifest file order/hash/size, and no context leakage.
- Extend result-validator tests for legacy, valid private, malformed case ID,
  false/missing/partial manifest metadata, and incompatible combinations.
- Extend job-manager tests for case retention, private lifecycle, legacy
  compatibility, cleanup, and failure clearing.
- Run all directly affected tests plus `git diff --check`.
- Perform an adversarial review of path traversal, JSON escaping, command-line
  disclosure, archive node types, partial publication, and compatibility.
  Correct all actionable findings and repeat relevant checks.

## Exit criteria

Stop once private readable candidates are internally correlated and legacy HTTP
and UI behavior remains unchanged. Commit and push only the attributable Slice
3 changes after staged-diff review.
