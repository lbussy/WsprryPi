# Issue 414 Slice 26 Execution Prompt: Local Generation-1 Manifest

## Objective

Retrieve the production Dropbox File Request capability from macOS Keychain
without exposing it in argv or output, prepare and exact-byte sign the local
generation-1 intake pair with the recovery-qualified production signing
identity, authenticate it through the Slice 15 lifecycle boundary, and stop
before any publication candidate commit or network operation.

## Verified context

- Slice 25 established and signed-out-qualified the open production WsprryPi
  File Request and stored its URL in Keychain service
  `org.wsprrypi.support-intake`, account `wsprrypi-file-request`.
- Production signing identity `wsprrypi-intake-2026-01` and bundle public
  metadata `wsprrypi-bundle-2026-01` are owner-controlled and recovery-qualified.
- The latest released WsprryPi tag is `v3.2.0`; stale protocol examples using
  `1.3.0` must not govern the production minimum.
- The public intake repository remains at its README-only root commit.
- No local manifest staging root currently exists.

## Production generation-1 policy

- generation: `1`;
- status: `active`;
- minimum client protocol: `1`;
- minimum upload version: `3.2.0`;
- release URL: `https://github.com/WsprryPi/WsprryPi/releases/latest`;
- user message: `null`;
- publication timestamp: the explicit current UTC second selected at execution;
- expiration: exactly 90 days after publication; and
- bundle key ID: derived from the reviewed production bundle metadata.

## Scope and requirements

1. Add a narrow maintainer controller that:
   - fixes production Keychain service/account identifiers;
   - invokes only exact root-owned `/usr/bin/security` with fixed shell-free
     arguments and a minimal environment;
   - bounds stdout, discards stderr, rejects missing/multiple/newline/non-UTF-8
     values, and applies the existing Dropbox URL policy;
   - never accepts or emits the request URL through CLI arguments, environment,
     logs, exceptions, result objects, or repository files; and
   - passes the in-memory URL directly to Slice 14 `prepare()`.
2. Keep production external paths explicit and owner-controlled. Create one
   empty mode `0700` staging root under `WsprryPi Support Intake/manifests/`
   only after preflight succeeds.
3. Add focused tests proving fixed Keychain argv/environment, strict output
   bounds/framing/policy, no disclosure, failure-before-preparation, exact
   policy propagation, and no production publication/network call.
4. Run a dry preflight that validates Keychain, metadata, identity, OpenSSL,
   timestamps, and empty destination without signing.
5. Prepare generation 1 once with explicit approval, require a durable
   `committed` result, then authenticate it independently through Slice 15.
6. Verify owner/mode/link/type, exact deterministic reconstruction, signature,
   public IDs, policy fields, exact SHA-256, and absence of unexpected inventory.
7. Record only non-sensitive timestamps, IDs, policy, digest, modes, and status.

## Constraints and non-goals

- Do not print, commit, log, hash for reporting, or place the request capability
  in argv, environment, exception text, or documentation.
- Do not expose private identity bytes or private-file hashes.
- Do not create a Slice 16 candidate commit, contact/push/fetch the public
  repository, run Slice 18 retrieval, or activate application runtime.
- Do not modify Dropbox, Keychain, production public metadata, identities, vault
  backups, UI, installer, service, Pi, GPIO, transmitter, or RF state.
- Preserve every predecessor and unrelated worktree/branch.

## Validation and adversarial review

- Run the new controller tests plus Slice 13-15 provisioning, preparation, and
  lifecycle tests.
- Independently authenticate the production pair using only public metadata and
  verify deterministic manifest reconstruction without displaying sensitive
  fields.
- Inspect process interfaces and the complete diff for capability/private-data
  disclosure or accidental publication/runtime wiring.
- Confirm the public repository ref/tree remain unchanged.
- Run Python syntax checks and `git diff --check`.
- Correct every actionable finding and repeat review until clean.

## Exit criteria

Commit and push only when the Keychain-backed controller is fail-closed and
non-disclosing, generation 1 is durably staged and lifecycle-authenticated,
the public repository is unchanged, all focused checks pass, and the WsprryPi
diff contains only this slice's controller, tests, CI/Make wiring, prompt,
outcome record, plan index, and prior handoff.
