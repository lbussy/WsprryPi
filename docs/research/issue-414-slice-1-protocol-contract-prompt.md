# Issue 414 Slice 1: Private Intake Protocol Contract Prompt

## Objective

Freeze an implementable first-version protocol for WsprryPi private support-bundle intake before changing runtime or UI code.

This slice converts `docs/plans/support-bundle-private-intake.md` into concrete schemas, state transitions, security boundaries, retention rules, and implementation seams that later slices can test independently.

## Repository boundary

Work only in the WsprryPi repository. Treat `WsprryPi-UI` and `src/` as ordinary parent-repository component paths with their own design and test boundaries.

Do not modify runtime, installer, UI, collector, configuration, workflow, or operator-documentation behavior in this slice. Do not operate Raspberry Pi hardware, services, GPIO, I2C, or RF.

Preserve unrelated working-tree changes. Use an isolated worktree based on the current `origin/devel` when another issue owns the main checkout.

## Required evidence

Inspect at least:

- `docs/plans/support-bundle-private-intake.md`;
- Issue #414 and its relationship to Issue #352;
- the existing collector output, result JSON, SHA-256, and archive naming;
- support-bundle job state, 24-hour retention, deletion, download verification, guarded HTTP routes, and UI lifecycle;
- WsprryPi version representation and available HTTPS and OpenSSL facilities;
- Debian 13 availability and stable behavior of the proposed encryption tool;
- official Dropbox File Request behavior relevant to signed-out upload and request closure; and
- existing WsprryPi product and design guidance for proposed operator states and terminology.

Use primary sources for external technical claims.

## Decisions that must be closed

1. Case ID and artifact ID formats.
2. Candidate, finalized, encrypted, downloaded, upload-reported, and deleted state transitions.
3. Exact readable archive and internal manifest relationship.
4. Support-context placement and GitHub/non-GitHub correlation timing.
5. Review and exclusion behavior compatible with a browser-downloaded `.tar.gz`.
6. Exact-byte finalization and the point after which recollection is forbidden.
7. Encryption format, recipient type, dependency, invocation, output validation, and retry behavior.
8. Bundle-encryption and intake-manifest-signing key IDs and rotation compatibility.
9. Intake manifest and detached-signature byte formats.
10. Signature verification, endpoint restrictions, expiry, clock handling, generation rollback protection, disable behavior, and upgrade gating.
11. Dropbox handoff behavior and the boundary between opening the request and confirmed receipt.
12. Local and Pi-side artifact retention, deletion, restart, and failure behavior.
13. Maintainer intake validation and archive extraction safety.
14. Test fixtures and acceptance gates needed before UI integration.

## Required invariants

- The user can obtain and inspect a readable archive before upload consent.
- The encrypted artifact contains the exact finalized readable archive bytes.
- No diagnostic recollection occurs after the user approves a candidate.
- Plaintext is never uploaded as a fallback.
- Bundle encryption and intake-manifest signing use separate project-specific keys.
- Missing, invalid, expired, disabled, rolled-back, or incompatible intake configuration blocks upload without blocking local collection and saving.
- No maintainer Dropbox or GitHub credential is embedded in the application.
- GitHub posting remains explicit and authenticated by the user.
- Non-GitHub submissions contain a useful description and user-approved contact method.
- Opening Dropbox is not reported as successful upload.
- Existing CLI collection remains available.

## Deliverables

1. A decision record at `docs/research/issue-414-slice-1-protocol-contract.md`.
2. Concrete JSON examples and validation rules.
3. A state model and failure matrix.
4. Explicit implemented-baseline, future-work, and non-goal sections.
5. A bounded handoff for Issue #414 Slice 2.
6. A small update to the parent plan linking this decision record without claiming runtime implementation.

## Validation

- Confirm the decision record resolves every required decision or labels it as an explicit later-slice implementation choice that does not alter the protocol.
- Check all named paths and baseline statements against the current source.
- Run `git diff --check` and `git diff --cached --check` as applicable.
- No source build, UI render, or hardware validation is required because this slice changes documentation only.
- Use Impeccable to review proposed operator terminology and error-state hierarchy; do not modify or render the UI.

## Completion boundary

Stop after the protocol decision record, prompt, plan cross-reference, and documentation validation. Do not begin encryption, manifest retrieval, maintainer tooling, or UI implementation.
