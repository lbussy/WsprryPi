# Issue 414 Slice 28 Execution Prompt: Live Publication Push

## Objective

Publish the verified production generation-1 candidate to
`WsprryPi/support-intake` using the qualified Slice 17 authenticated,
lease-protected push boundary. Confirm the exact remote commit after the push,
then stop before public raw-byte retrieval or application activation.

## Verified context

- Slice 27 authenticated the production source and created local candidate
  `3e0b4017bfe7612bd39ccc6e2f29d743174664b5` with sole parent
  `770d63521cf23d1ccb5eb7c9911e040ab18032d7`.
- The candidate changes exactly `wsprrypi/intake.json` and
  `wsprrypi/intake.json.sig`; both blobs exactly match the staged generation.
- The bare repository is owner-controlled mode `0700`, has symbolic `HEAD` at
  `refs/heads/main`, one ref, one `origin`, and no unsafe object indirection.
- `origin` is exactly `https://github.com/WsprryPi/support-intake.git` for fetch
  and push.
- GitHub CLI reports the maintainer account authenticated from the system
  keyring with HTTPS Git operations. No credential value is to be displayed or
  handled manually.

## Scope and requirements

1. Recheck the WsprryPi worktrees, staged production generation, local
   publication repository, candidate parent/paths/bytes, authentication, and
   credential-helper availability before contacting the remote.
2. Run the focused Slice 15-17 tests and Python syntax checks before live use.
3. Invoke the Slice 17 controller without approval. It must:
   - re-authenticate the staged source and exact candidate;
   - query only `origin` `refs/heads/main` through controlled Git;
   - report the remote as exactly the candidate parent and return `proposed`;
   - perform no push; and
   - emit only approved non-sensitive metadata.
4. If the remote is absent, malformed, conflicting, already unexpected, or the
   query fails, stop without mutation and report the exact typed status.
5. Only after an eligible proposal, invoke the same controller once with
   explicit approval and the allowlisted `osxkeychain` helper.
6. Require one exact push using:
   - `--force-with-lease=refs/heads/main:PARENT`; and
   - `CANDIDATE:refs/heads/main`.
7. Require the controller's post-push query to return exactly the candidate and
   status `published` (or `already_published` if an independently identical
   publication won the race). Treat rejection and confirmation uncertainty as
   non-success and do not attempt rollback or an unqualified retry.
8. Independently query only the remote main ref after success and require exact
   candidate identity. Do not retrieve file contents in this slice.
9. Record only non-sensitive commit IDs, generation, status, public key IDs,
   manifest digest, controller status, and ref identity.

## Constraints and non-goals

- Never print, log, retrieve, or copy into WsprryPi documentation the manifest,
  signature, Dropbox request capability, signing public bytes, bundle recipient,
  credentials, tokens, cookies, or private-key material.
- Do not accept credentials through CLI, environment, files, prompts, or manual
  copying. Use only the fixed `osxkeychain` helper.
- Do not use a plain force push, change the lease, alter the refspec, push any
  other ref, add a remote, or modify Git configuration.
- Do not run Slice 18 public raw-file retrieval, curl raw endpoints, activate
  runtime trust, modify Dropbox/Keychain/identities/metadata, or change UI,
  installer, service, Pi, GPIO, transmitter, or RF state.
- Do not repeat a rejected or uncertain push automatically. Preserve evidence
  and stop for review.

## Validation and adversarial review

- Run manifest lifecycle, publication commit, and publication push focused
  tests.
- Confirm controlled Git disables hooks, prompting, proxies, system/global
  configuration, replacement objects, and non-allowlisted credential helpers.
- Compare candidate/source identity before live query, immediately before push,
  and after confirmation.
- Inspect command construction and output for secret disclosure or broader refs.
- Independently confirm remote `refs/heads/main` equals the candidate without
  retrieving blobs.
- Confirm the local candidate and staged source remain unchanged.
- Search the repository diff for credentials, request IDs, private material,
  public manifest bytes, or accidental Slice 18/runtime work.
- Run `git diff --check`; correct every actionable finding and repeat review
  until clean.

## Exit criteria

Commit and push the WsprryPi Slice 28 branch only after the controlled live push
is confirmed at the exact candidate, all local sources remain unchanged, all
focused checks pass, no sensitive bytes were disclosed, no public files were
retrieved, and the WsprryPi diff contains only this prompt, its non-sensitive
outcome record, the plan index, and the Slice 27 handoff update.
