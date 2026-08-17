# Issue 414 Slice 27 Execution Prompt: Production Publication Candidate

## Objective

Authenticate the locally staged production generation-1 manifest/signature pair
and record those exact bytes as one local commit in the owner-controlled bare
`support-intake` publication repository. Stop before every publication-repository
network operation, including fetch, push, or public retrieval.

## Verified context

- Slice 26 durably staged and independently authenticated generation 1 under
  the production manifest root.
- The authenticated manifest uses signing key `wsprrypi-intake-2026-01`, bundle
  key `wsprrypi-bundle-2026-01`, and SHA-256
  `80902216b212ca1a8c2a9fd3e9693aac2c0aa17c7838d939bbebaa8887fb71e8`.
- Slice 24 established the owner-controlled mode `0700` bare publication
  repository with only `origin`, fixed to
  `https://github.com/WsprryPi/support-intake.git`.
- Its `main` ref remains at the README-only initial commit
  `770d63521cf23d1ccb5eb7c9911e040ab18032d7`.
- Slice 16 qualified the local compare-and-swap publication commit boundary.

## Scope and requirements

1. Recheck both WsprryPi worktrees and preserve all unrelated branches and
   files.
2. Validate the production Git executable, OpenSSL executable, staging root,
   public signing metadata, and bare-repository ownership, mode, symbolic HEAD,
   sole ref, sole remote, remote URL, and unsafe-object-indirection exclusions.
3. Run the Slice 16 controller without approval and prove that it:
   - authenticates generation 1 through the Slice 15 lifecycle boundary;
   - reports the expected non-sensitive generation, status, IDs, digest, prior
     commit, and fixed target paths; and
   - creates no Git objects and changes no ref.
4. Run the controller once with explicit approval. Require status `committed`
   and a single-parent commit whose parent is the initial README commit.
5. Independently verify the resulting local commit:
   - changes exactly `wsprrypi/intake.json` and
     `wsprrypi/intake.json.sig`;
   - contains bytes exactly equal to the authenticated local pair;
   - preserves `README.md` unchanged;
   - has the expected non-sensitive commit metadata; and
   - is the only advancement of local `refs/heads/main`.
6. Confirm the source generation remains unchanged and re-authenticates after
   the commit.
7. Record only non-sensitive object IDs, policy fields, digest, modes, statuses,
   and inventory. Do not display manifest or signature contents.

## Constraints and non-goals

- Do not invoke the Slice 17 push controller or use the publication repository
  for network access. Do not fetch or push `support-intake`, query its remote,
  or retrieve public raw URLs.
- Do not print, log, copy into documentation, or otherwise disclose the Dropbox
  request capability, signature value, signing public bytes, bundle recipient,
  or any private-key material.
- Do not modify the staged generation, Dropbox, Keychain, identities, metadata,
  vault backups, application runtime, trust compilation, UI, installer,
  services, Raspberry Pi, GPIO, transmitter, or RF state.
- Do not modify the public GitHub repository. Local unreachable Git objects are
  acceptable only if an approved attempt fails as documented by Slice 16.
- Keep live push, exact public-byte verification, and runtime activation as
  separately approved later slices.

## Validation and adversarial review

- Run the Slice 15 lifecycle and Slice 16 publication-commit focused tests.
- Capture the dry-run ref and object inventory before and after and require exact
  equality.
- Inspect the approved commit through controlled local Git plumbing, comparing
  exact blob bytes by digest/byte comparison without rendering them.
- Re-run lifecycle authentication after the commit and verify the source files'
  ownership, mode, type, link count, size, and digest remain unchanged.
- Search the WsprryPi diff for credentials, request IDs, private material, or
  accidental Slice 17/runtime wiring.
- Run Python syntax checks and `git diff --check`.
- Correct every actionable finding and repeat review until clean.

## Exit criteria

Commit and push the WsprryPi Slice 27 branch only after the local publication
candidate is verified, the external bare repository has advanced exactly once,
no publication-repository network operation occurred, all focused checks pass,
and the repository diff contains only this prompt, its non-sensitive outcome
record, the plan index, and the Slice 26 handoff update.
