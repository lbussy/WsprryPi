# Issue 414 Slice 4: Review and Finalization Workflow Prompt

## Objective

Expose the private readable-candidate workflow through guarded HTTP and the
Maintenance interface, ending at explicit user approval and immutable
finalization. Preserve legacy API callers and stop before encryption or upload.

## Backend scope

- Generate the case ID in the application with injected production/test entropy.
- Accept a strict optional `support_context` object on support-bundle creation.
- Validate existing-issue and description/contact paths with the Slice 2 rules.
- Stage description/contact as private files for the collector; never place them
  in process arguments.
- Require the collector result case ID to match the application-generated ID.
- Expose non-sensitive `case_id` and `workflow_state` in private snapshots.
- Mark a candidate downloaded only after the HTTP content provider completes.
- Add a guarded finalization endpoint that requires a completed candidate
  download, verifies exact size/SHA-256, changes the archive to `0400`, retains
  its descriptor, and is idempotent.
- Retain the readable candidate after download. Existing explicit deletion and
  24-hour expiry continue to remove the whole job directory.

Legacy creation with only `probe_i2c` remains valid and keeps the established
download behavior. No encryption, receipt, signed intake, Dropbox, GitHub
posting, installer dependency, or remote network work is in scope.

## HTTP contract

Creation accepts:

```json
{
  "probe_i2c": false,
  "support_context": {
    "kind": "existing_github_issue",
    "issue_url": "https://github.com/WsprryPi/WsprryPi/issues/352"
  }
}
```

or `kind: new_github_issue|no_github` with bounded `problem_description` and
`contact`. Reject unknown, partial, contradictory, oversized, or mistyped
fields. Identity is not verified.

Private snapshots add `case_id` and one of `collecting`, `candidate_ready`,
`candidate_downloaded`, or `finalized`. Legacy snapshots omit those fields.

`POST /api/support-bundles/<id>/finalize` has an empty body, uses the existing
same-origin guard, and returns the finalized snapshot. It fails closed for
unknown, legacy, active, undownloaded, corrupt, deleted, or expired jobs.

## UI contract

Replace the create modal with an inline progressive workflow in the existing
Support Bundle panel:

1. Choose an existing GitHub issue or describe the problem and provide contact.
2. Optionally consent to the active I²C probe.
3. Create and display the case ID while collecting.
4. Download the readable `.tar.gz`; do not delete it automatically.
5. Explain the browser save boundary and require an explicit review checkbox.
6. Finalize only after that checkbox; clearly state that the exact reviewed
   bytes become immutable and remain on the Pi for the later encryption step.
7. Keep **Delete from Pi** available throughout retained states.

Use established Bootstrap, light/dark tokens, direct language, inline status,
keyboard-visible focus, `aria-live`, field-specific errors, and mobile stacking.
Do not use a modal, nested card grid, decorative state colors, or imply that
opening/downloading/finalizing is an upload.

## Validation

- Unit tests for case generation failure, context validation, collector argv and
  private staging, result-case mismatch, lifecycle transitions, idempotent
  finalization, mutation/digest failure, deletion, and expiry.
- HTTP tests for strict request parsing, guarded finalize, download completion,
  legacy compatibility, and non-disclosure of description/contact.
- UI/source tests for both context paths, conditional fields, validation,
  retained download, explicit consent, finalize, deletion, focus, truthful copy,
  and no private values in status output.
- Render and inspect desktop and mobile in one Impeccable screenshot round, fix
  material findings in one batch, and confirm once.
- Run the Impeccable detector once after UI edits and complete the required finish
  review. Run relevant backend, collector, UI, ShellCheck, and diff checks.

## Exit criteria

Stop at immutable finalized readable bytes. Do not begin encryption, receipt,
signed-intake, Dropbox, or issue-posting implementation. Commit and push only
the attributable Slice 4 files after staged review.
