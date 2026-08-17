# Issue 414 Slice 11: Minimum Upload Version Gate Prompt

## Objective

Implement and qualify the signed `minimum_upload_version` application-version
gate required by the current support-intake contract. Provide only the narrowly
scoped, authenticated upgrade information needed by a later UI. Stop before
runtime construction, production trust material, upload, or UI.

## Verified context

- The current contract requires every upload attempt to compare the installed
  application version with signed `minimum_upload_version`, block older clients
  without a bypass, display the minimum, and offer the signed official release
  location without revealing the replacement Dropbox URL.
- Slice 7 currently validates the minimum string but enforces only the integer
  client-protocol gate. Its failure helper intentionally returns no manifest.
- Slice 10 returns a manifest only after validation and durable rollback-state
  commit and otherwise exposes stage enums only.
- Retrieval failures must remain availability/network failures and must never be
  relabeled as upgrade requirements.

## Scope and required behavior

1. Add the installed application version to the typed Slice 7 validation
   request and require it for every validation attempt.
2. Parse versions without locale or integer overflow. Require canonical SemVer
   2 for both the signed minimum and installed version, including optional
   prerelease and build metadata so development/release builds can be compared
   truthfully. Reject empty, malformed, non-canonical, or oversized installed
   versions as `invalid_client_version`, not `upgrade_required`.
3. Compare major, minor, patch, and SemVer prerelease precedence; ignore build
   metadata for precedence. Numeric prerelease identifiers sort numerically and
   below nonnumeric identifiers. A prerelease sorts below its corresponding
   release.
4. Add a distinct `upgrade_required` validation failure when the installed
   version is lower than the signed minimum on an `active` manifest. A valid
   `disabled` manifest remains authoritative and is durably accepted without
   upgrade guidance because no upload route exists. Do not reuse protocol
   incompatibility or retrieval failure categories.
5. Authenticate the exact manifest bytes before parsing or returning any policy
   field. Before constructing upgrade information, validate project/schema,
   generation/rollback, time window, status/request-URL shape, release URL, and
   recognized bundle key as already required.
6. On `upgrade_required` only, return a dedicated limited structure containing
   the signed minimum version, validated official release URL, and optional
   signed user message. The ordinary manifest result MUST remain empty, and the
   limited structure MUST NOT contain or expose the Dropbox request URL,
   signing/bundle key IDs, timestamps, signature, or fetched bytes.
7. An authenticated `upgrade_required` result SHALL also carry a separate
   internal state candidate containing only generation and exact manifest
   SHA-256. Slice 10 must durably commit that candidate before propagating the
   limited upgrade structure, preventing replay of an older signed generation.
   Commit failure or mutation returns neither manifest nor upgrade guidance;
   uncertain durability requires the existing identical retry confirmation.
   Do not convert any other validation/retrieval/state failure into an upgrade
   result.
8. Add focused adversarial tests for equality, newer/older major-minor-patch,
   prerelease ordering, build metadata, very large numeric identifiers,
   malformed/non-canonical/oversized installed versions, invalid signatures,
   invalid/expired/wrong-project manifests, invalid release URLs, unknown bundle
   keys, rollback, and request-URL non-disclosure.
9. Add controller tests proving durable upgrade-state commit before guidance,
   higher-generation upgrade followed by lower-generation replay rejection,
   competing-writer authority, uncertain-sync retry, no manifest or request
   URL, and unchanged categorization of retrieval/protocol/state failures. Keep
   all tests offline with ephemeral signing keys.
10. Update focused CI and implementation documentation, including correction of
    the earlier Slice 1 note that described minimum version as display-only.

## Constraints and non-goals

- Do not weaken exact-byte signature verification, strict JSON parsing, clock
  policy, URL allowlists, key recognition, rollback checks, or durable commit.
- Do not reveal a request URL to an outdated or otherwise rejected client.
- Do not expose upgrade guidance until its authenticated generation/hash state
  is durably committed; never persist URLs or guidance in rollback state.
- Do not add a bypass, endpoint override, version override, CLI, INI,
  environment, HTTP route, UI, retry loop, or background refresh.
- Do not add production signing keys, bundle recipients, manifests, signatures,
  Dropbox IDs, runtime state paths, installer changes, services, hardware,
  GPIO, I2C, transmitter, or RF activity.

## Validation and evidence

- Run Slice 7 validation and Slice 10 controller tests plus the new focused
  version-gate coverage locally.
- Run the relevant targets in a clean Debian container without network access.
- Run `git diff --check`, review the complete staged diff, and obtain an
  independent adversarial review. Correct every actionable finding and repeat
  review until no blockers remain.

## Exit criteria

Stop with a strict signed application-version gate and a minimal authenticated
upgrade result suitable for later runtime/UI consumption. Commit and push only
attributable Slice 11 files. Runtime construction, production trust material,
encryption/upload composition, and UI remain later slices.
