# Issue 414 Slice 23 Execution Prompt: Production Public Trust Compilation

## Objective

Version the reviewed Slice 21 production public metadata and compile it into the
minimal deterministic C++ trust data defined by Slice 19. Prove that repository
metadata and compiled output agree exactly while keeping the runtime inactive.

## Verified context

- Slice 22 recovery-qualified the separate production private identities.
- The reviewed public key IDs are `wsprrypi-intake-2026-01` and
  `wsprrypi-bundle-2026-01`.
- The reviewed public fingerprints are
  `688b5769d2b763481bad938fe8a9963693950c5e80bcf6d47d71db75711843ac`
  and `61289289afbd0f7813eb59b54e60d514f3cd8dbdf05e9c6b2d405b101b5b0fc4`.
- Slice 19 emits only the signing key ID and 32 public Ed25519 bytes plus the
  recognized bundle key ID. It intentionally omits the age recipient from the
  runtime trust type.
- Slice 12 runtime construction remains deliberately uncalled by production.

## Scope and requirements

1. Validate the two external public metadata files with the Slice 19 compiler
   before copying any bytes into the repository.
2. Check in exact, human-reviewable public JSON for both production key IDs.
3. Check in the deterministic compiler output as
   `src/support_bundle_intake_compiled_trust.hpp`.
4. Add a source regression that:
   - validates the exact production IDs, algorithms, timestamps, public values,
     and fingerprints;
   - independently recomputes both fingerprints;
   - regenerates the header from repository metadata and compares exact bytes;
   - compiles and runs a small C++ consumer proving the generated trust object
     contains exactly one correct signing key and one recognized bundle ID; and
   - rejects private material, routing, credentials, local paths, and unexpected
     fields in the versioned artifacts.
5. Wire the focused regression into the Makefile and Debian non-hardware CI.
6. Update the implementation record and plan index truthfully.

## Constraints and non-goals

- Public keys, recipients, IDs, timestamps, and fingerprints are intentionally
  public. No private identity bytes or private-file hashes may enter Git.
- Do not call the production runtime, retrieve a manifest, encrypt a bundle,
  create or contact GitHub/Dropbox resources, publish a manifest, or alter an
  installer, service, UI, Pi, GPIO, transmitter, or RF state.
- Do not add the age recipient to `SupportBundleIntakeRuntimeTrust`; encryption
  orchestration is a later separately reviewed slice.
- Do not modify or delete the external production identity or staging files.
- Preserve all unrelated worktrees and branches.

## Validation and adversarial review

- Run the Slice 19 synthetic compiler test, the new production-source
  regression, and the Slice 12 runtime test.
- Run Python syntax checks and `git diff --check`.
- Inspect the complete diff and scan changed files for private-key markers,
  credentials, absolute maintainer paths, request URLs, and accidental runtime
  activation.
- Confirm deterministic regeneration from only checked-in public inputs.
- Confirm the application still has no production call site for intake runtime
  resolution or the compiled-trust factory.
- Correct every actionable finding and repeat review until clean.

## Exit criteria

Commit and push only when the checked-in public sources, compiled header, tests,
CI, and documentation agree exactly; all focused checks pass; no private or
routing material is present; runtime remains inactive; and the final staged diff
contains only this slice.
