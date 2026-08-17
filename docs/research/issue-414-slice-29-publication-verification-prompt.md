# Issue 414 Slice 29 Execution Prompt: Production Public Verification

## Objective

Retrieve the two published production intake files through the qualified Slice
18 HTTPS controller, compare both bodies byte-for-byte with the authenticated
local generation-1 pair, independently authenticate the retrieved pair, and
confirm the exact generation, digest, and candidate commit. Stop before trust
activation, application integration, or upload orchestration.

## Verified context

- Slice 28 published and ref-confirmed `WsprryPi/support-intake` main at
  `3e0b4017bfe7612bd39ccc6e2f29d743174664b5`.
- The candidate's sole parent is
  `770d63521cf23d1ccb5eb7c9911e040ab18032d7` and its diff contains exactly the
  two fixed intake paths.
- The authenticated local source is active generation 1, signed by
  `wsprrypi-intake-2026-01`, uses bundle key `wsprrypi-bundle-2026-01`, and has
  manifest SHA-256
  `80902216b212ca1a8c2a9fd3e9693aac2c0aa17c7838d939bbebaa8887fb71e8`.
- Slice 18 fixes the two public `raw.githubusercontent.com` endpoints and uses
  root-owned `/usr/bin/curl` with HTTPS-only transport, ordinary CA/hostname
  validation, no redirects, no proxy, bounded bodies, deadlines, and HTTP 200.

## Scope and requirements

1. Recheck both WsprryPi worktrees, the owner-controlled staging and publication
   roots, lifecycle authentication, exact local candidate, and candidate/source
   byte equality before network retrieval.
2. Run the focused Slice 15, 16, and 18 tests plus Python syntax checks before
   production use.
3. Invoke only the Slice 18 production verification controller with:
   - expected commit
     `3e0b4017bfe7612bd39ccc6e2f29d743174664b5`;
   - exact production Git, OpenSSL, bare repository, staging root, and public
     signing metadata paths; and
   - no injectable URL, transport, proxy, credential, or output path.
4. Require the controller to re-authenticate local generation 1 and validate the
   exact local candidate before either fetch.
5. Retrieve both fixed endpoints. Require HTTP 200, body bounds, no redirect,
   and exact byte equality for both the manifest and signature envelope.
6. Independently authenticate the retrieved pair in an owner-only temporary
   directory using the reviewed public signing metadata. Require exact
   generation and manifest digest equality with the authenticated source.
7. Require final typed status `verified`, generation `1`, the expected manifest
   SHA-256, and the expected candidate commit.
8. Confirm after retrieval that the staged source, local candidate, and remote
   main ref remain unchanged. A ref-only query may be used; do not render either
   public body.
9. Record only status, generation, manifest digest, commit IDs, public key IDs,
   transport policy, and validation results.

## Constraints and non-goals

- Never print, log, persist outside the controller's owner-only temporary
  directory, or copy into WsprryPi documentation the public manifest body,
  signature envelope, Dropbox request capability, release/user message fields,
  signing public bytes, bundle recipient, or any private material.
- Do not use a browser, follow redirects, weaken TLS validation, enable proxy
  inheritance, accept alternate endpoints, or fetch any other path/ref.
- Do not modify or push `support-intake`, staging files, Keychain, Dropbox,
  identities, metadata, or vault backups.
- Do not compile or activate production trust, wire runtime resolution, expose
  the request URL to an application, encrypt/upload a bundle, or modify UI,
  installer, service, Pi, GPIO, transmitter, or RF state.
- Retrieval failure, content mismatch, authentication failure, or local
  validation failure is non-success. Do not normalize or repair public/local
  content automatically.

## Validation and adversarial review

- Run manifest lifecycle, publication commit, and publication verification
  focused tests.
- Inspect fixed curl argv/environment and confirm bounds and status-trailer
  handling remain fail-closed and binary-preserving.
- Confirm fetched bytes are never placed in result objects, argv, environment,
  logs, or repository files.
- Re-authenticate the local source and recompare candidate blobs after the live
  verification.
- Independently query only remote `refs/heads/main` and require the expected
  candidate.
- Search the complete WsprryPi diff for request IDs, signatures, recipients,
  credentials, private material, manifest contents, or activation wiring.
- Run `git diff --check`; correct every actionable finding and repeat review
  until clean.

## Exit criteria

Commit and push the WsprryPi Slice 29 branch only after production verification
returns `verified`, all local and remote identities remain exact, no sensitive
body is disclosed or retained, all focused checks pass, and the WsprryPi diff
contains only this prompt, its non-sensitive outcome record, the plan index, and
the Slice 28 handoff update.
