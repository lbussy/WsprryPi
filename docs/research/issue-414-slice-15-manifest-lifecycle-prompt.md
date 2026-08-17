# Issue 414 Slice 15 Execution Prompt: Local Intake Manifest Lifecycle

## Objective

Add a maintainer-only local lifecycle tool with `inspect`, `rotate`, `disable`,
and `renew` operations. It must authenticate the exact current staged manifest
pair, require that it is the highest complete local generation, derive the next
generation rather than accepting one from the operator, and delegate exact-byte
signing and atomic staging to Slice 14.

## Verified context

- Slice 14 can construct and sign one explicitly numbered generation but does
  not authenticate or derive lifecycle state.
- The contract requires ordinary URL rotation, temporary disablement, periodic
  renewal, monotonic generations, and review before later remote publication.
- Remote publication and post-publication retrieval remain separate work.

## Scope

1. Add `scripts/maintainer/manage_support_bundle_intake_manifest.py`.
2. Accept a pre-existing absolute owner-controlled `0700` staging root whose
   complete entries are exactly `generation-N` directories containing only
   `0600` regular single-link `intake.json` and `intake.json.sig` files.
3. Select the numerically highest complete generation; fail closed for partial
   directories, malformed generation names, gaps, duplicate numeric aliases,
   unexpected entries, unsafe modes/types/links, or an empty root.
4. Strictly authenticate the selected pair using Slice 13 signing public
   metadata: strict bounded envelope, canonical 64-byte signature, matching key
   ID/algorithm, and OpenSSL verification over the exact manifest bytes. Then
   strictly parse and reconstruct the deterministic Slice 14 manifest and
   require byte-for-byte equality and directory-generation agreement.
5. `inspect` must require no private key, make no changes, and output only status,
   generation, status, timestamps, minimum protocol/version, key IDs, and exact
   manifest SHA-256. It must not print request/release URLs, user messages,
   signatures, or public-key bytes.
6. Mutating operations require the matching owner-controlled `0400` private key
   and explicit `--approve`. Without approval, validate and report a bounded
   non-sensitive proposed summary but create no files.
7. Derive generation `current + 1` and preserve all fields unless the operation
   explicitly owns them:
   - `rotate`: require a replacement Dropbox request URL and new publication and
     expiration timestamps; set the successor active whether the current state
     is active or disabled, preserve other policy fields, and allow an optional
     explicit minimum-version or message replacement.
   - `disable`: require new timestamps, omit the request URL, set disabled, and
     require a nonempty bounded user message.
   - `renew`: require new timestamps, preserve status, request URL, policy,
     release URL, message, and bundle key ID unchanged.
8. Require publication time to move forward, expiration after publication, and
   each new expiration to be later than the current expiration. Delegate all
   next-pair construction, private/public matching, signing, self-verification,
   atomic generation-directory rename, and durability classification to Slice
   14 without weakening it.
9. Add fake and disposable real-OpenSSL tests for authentication, exact-byte
   mutation, wrong key/signature, root inventory attacks, inspect non-disclosure,
   dry-run immutability, approval gating, field preservation/change ownership,
   monotonic generation/time rules, collision/race refusal, delegated failure,
   and committed-sync-uncertain propagation.
10. Wire the focused test into Make and Debian non-hardware CI and add a truthful
    implementation record and roadmap link.

## Constraints

- Do not generate or use production keys, metadata, manifests, signatures,
  Dropbox IDs, or request URLs; fixtures are explicitly reserved test values.
- Do not delete, overwrite, edit, rename, or repair an existing generation.
- Do not implement initial generation creation, signing-key rotation, bundle-key
  rotation, remote GitHub publication, post-publication retrieval, Dropbox API
  or browser administration, reminders, credentials, application activation,
  HTTP, UI, installer, service, Raspberry Pi, GPIO, I2C, transmitter, or RF work.
- Do not treat a directory scan as signature verification, or a valid signature
  as permission to skip deterministic schema and policy checks.
- No operation may print routing URLs, user messages, signatures, key bytes, or
  private-key content.

## Validation and evidence

- Run fake-tool and real disposable OpenSSL lifecycle suites locally.
- Run a clean Debian packaged-OpenSSL fixture.
- Independently verify each real generated successor signature and prove the
  predecessor bytes remain unchanged.
- Run Python syntax compilation and `git diff --check`.
- Perform an adversarial review of inventory selection, signature-before-
  disclosure, deterministic reconstruction, operation field ownership, approval
  gating, monotonicity, delegation, non-disclosure, races, durability status,
  tests, and documentation. Correct every actionable finding and repeat.

## Exit criteria

- The highest complete local generation can be authenticated and inspected
  without private material.
- Approved rotate/disable/renew operations derive and atomically stage exactly
  one authenticated successor while preserving the predecessor.
- All tests pass with fake and packaged OpenSSL.
- No remote publication, production material, or application activation occurs.
