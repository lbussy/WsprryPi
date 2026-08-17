# Issue 414 Slice 16 Execution Prompt: Publication Candidate Commit

## Objective

Add a maintainer-only tool that authenticates the highest approved local intake
generation and atomically records its exact two files as the only content change
in a dedicated bare `WsprryPi/support-intake` Git repository. Do not push or
contact GitHub in this slice.

## Verified context

- Slice 15 authenticates append-only local lifecycle history and the highest
  exact manifest/signature pair.
- The reserved public endpoints use repository path `wsprrypi/intake.json` and
  `wsprrypi/intake.json.sig` on branch `main`.
- A live repository has not been created or authorized, and remote publication
  plus public HTTPS retrieval remain separate work.

## Scope

1. Add `scripts/maintainer/commit_support_bundle_intake_publication.py`.
2. Reuse Slice 15 authentication before reading or committing manifest data.
3. Require an absolute safe Git executable and an absolute owner-controlled
   bare repository with `core.bare=true`, current symbolic branch `main`, a
   single expected `origin` URL exactly
   `https://github.com/WsprryPi/support-intake.git`, and a valid current commit.
4. Without `--approve`, validate all inputs and report only generation, status,
   key IDs, manifest digest, current commit, and the two fixed target paths.
   Create no Git objects, refs, files, commits, or index state.
5. With `--approve`, use a private temporary index and fixed shell-free Git argv
   to read the current tree, write exactly two blobs, replace exactly the two
   fixed target paths, write a tree, and create one commit with a deterministic
   bounded subject/body containing only generation, status, key IDs, and digest.
6. Atomically advance `refs/heads/main` with `git update-ref NEW OLD`. A competing
   update must fail without overwriting it.
7. Verify after the update that the commit has exactly one parent equal to OLD,
   its only changed paths are the two fixed targets, and both committed blobs
   exactly equal the authenticated local bytes. If post-update verification
   fails, compare-and-swap the ref back from NEW to OLD; report rollback failure
   distinctly and never claim success.
8. Output no request/release URL, user message, signature value, public-key byte,
   private-key content, credentials, token, or remote response.
9. Add deterministic disposable bare-repository tests for dry-run immutability,
   exact successful commit, predecessor preservation, fixed tree scope, dirty or
   malformed repository policy, wrong remote/branch/non-bare inputs, competing
   ref updates, pre-update failures, post-update rollback, rollback failure, and
   CLI non-disclosure. Tests may use reserved fake signed pairs only.
10. Wire the test into Make and Debian non-hardware CI, add an implementation
    record, and update the roadmap.

## Constraints

- Do not create, clone, fetch, push, or modify a real GitHub repository.
- Do not use GitHub credentials, `gh`, SSH, network access, hooks, GPG signing,
  Git LFS, submodules, a worktree repository, or user-global Git configuration.
- Disable hooks and optional filters for the controlled commit plumbing.
- Do not overwrite or delete the source generation or any existing remote ref.
- Do not implement remote publication, post-publication HTTPS retrieval,
  production keys/metadata/manifests, application activation, HTTP, UI,
  installer, service, Raspberry Pi, GPIO, I2C, transmitter, or RF behavior.

## Validation and evidence

- Run the complete disposable bare-repository suite locally and in clean Debian.
- Prove dry-run object/ref immutability, exact committed blob equality, exact
  two-path diff scope, parent identity, and compare-and-swap behavior.
- Run Python syntax compilation and `git diff --check`.
- Adversarially review Git configuration isolation, hooks/filters, argv/output
  disclosure, source authentication, path scope, temporary-index cleanup,
  ref races, post-commit verification, rollback truthfulness, tests, and docs.
  Correct all actionable findings and repeat.

## Exit criteria

- One authenticated local generation can become one verified local publication
  candidate commit through an atomic ref update.
- Dry-run and all failures leave the branch ref unchanged unless a rollback
  failure is reported explicitly.
- No network, real remote mutation, or production material is used.
