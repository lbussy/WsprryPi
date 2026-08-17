# Issue 414 Slice 24 Execution Prompt: Publication Repository Establishment

## Objective

Create and validate the dedicated public `WsprryPi/support-intake` GitHub
repository and its owner-controlled local bare maintainer counterpart. Establish
the exact benign parent commit and repository policy required by Slices 16-18,
without preparing, signing, committing, or publishing an intake manifest.

## Verified context

- Slice 23 versioned and validated the production public trust metadata while
  leaving runtime inactive.
- GitHub authentication is valid for `lbussy` through the configured keyring.
- `WsprryPi/support-intake` does not currently exist.
- Slice 16 requires an owner-controlled mode `0700` bare repository, symbolic
  `HEAD` at `refs/heads/main`, exactly one branch ref, and exactly one remote:
  `https://github.com/WsprryPi/support-intake.git`.
- Slice 16 creates a single-parent publication commit, so the remote needs one
  reviewed non-manifest initial commit before the first intake publication.

## Scope and requirements

1. Create `WsprryPi/support-intake` as a public GitHub repository with:
   - default branch `main`;
   - one GitHub-generated minimal README as the initial and only commit;
   - description `Signed WsprryPi support-bundle intake configuration`;
   - Issues, Wiki, Projects, and Discussions disabled; and
   - no template, license, Actions workflow, Pages site, release, tag, secret,
     deploy key, webhook, environment, collaborator, or team grant.
2. Verify through GitHub metadata and Git that:
   - visibility is public and the exact owner/name and HTTPS URL are correct;
   - `main` is the default and only branch;
   - the initial tree contains only `README.md`;
   - no intake manifest or signature path exists; and
   - the disabled feature settings match the intended policy.
3. Create the local bare publication repository at the approved maintainer path
   under `WsprryPi Support Intake/publication/`, with owner-only `0700` directory
   permissions and the exact Slice 16 origin URL.
4. Validate the local repository using the production Slice 16 repository
   validator and independently confirm symbolic HEAD, sole ref, sole remote,
   initial commit identity, absence of alternates/grafts/replacements/shallow
   state, and no unsafe local transport configuration.
5. Record only non-sensitive repository identity, policy, initial commit, and
   validation evidence in the WsprryPi implementation record.

## Constraints and non-goals

- Do not prepare or sign generation 1, use either production private identity,
  commit `wsprrypi/intake.json` or `.sig`, run the Slice 17 push controller, or
  invoke Slice 18 public verification.
- Do not modify the production public metadata, private identities, vault
  backups, Dropbox File Request, application runtime, installer, HTTP, UI,
  service, Pi, GPIO, transmitter, or RF state.
- Do not enable branch protection or a ruleset that would prevent the reviewed
  exact-lease maintainer publication protocol; policy for direct publication is
  already enforced by Slices 16-18.
- Do not print, request, copy, or persist GitHub credentials.
- Preserve all unrelated repositories, worktrees, branches, and user changes.

## Validation and adversarial review

- Run the existing Slice 16-18 focused tests and `git diff --check`.
- Query GitHub again after creation and compare exact repository settings.
- Fetch remote refs independently and require exactly one `main` ref.
- Inspect the remote initial tree and commit parents; require a root commit with
  only `README.md` and no manifest content.
- Run the Slice 16 production repository validator against the local bare clone.
- Search both repository state and the WsprryPi diff for unexpected manifests,
  signatures, credentials, private-key markers, routing URLs, or activation.
- Correct every actionable finding and repeat review until clean.

## Exit criteria

Commit and push the WsprryPi documentation record only after the GitHub and
local repositories satisfy every exact invariant, all focused non-hardware
checks pass, no intake pair has been created or published, and the WsprryPi diff
contains only the Slice 24 prompt, outcome record, and plan/handoff updates.
