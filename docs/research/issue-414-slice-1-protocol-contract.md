# Issue 414 Slice 1: Private Intake Protocol Contract

Status: Accepted design for subsequent Issue #414 slices

Implementation state: Protocol only; no runtime or UI behavior implemented

Baseline: `origin/devel` at `00f093c8523d2068740d0371526d2340d8d99379`

## Outcome

WsprryPi will extend the existing retained support-bundle job rather than replace its collector. The Pi will finalize and retain one readable `.tar.gz`, allow the browser user to download and review it, and only after explicit consent encrypt those same retained bytes to a project-specific `age` recipient.

The browser will download the encrypted artifact and open a signed-manifest-selected Dropbox File Request. The user will select the downloaded `.age` file on Dropbox's page. Version 1 will not embed Dropbox credentials or attempt an undocumented direct File Request upload.

This design preserves the trust property that the user can inspect a readable bundle, while making the uploaded artifact confidential and the upload destination replaceable.

## Evidence and existing baseline

Issue #352 already provides:

- asynchronous collection with `queued`, `running`, `succeeded`, and `failed` states;
- one retained job under `/var/lib/wsprrypi/support-bundles`;
- a validated `.tar.gz`, `.sha256`, and result JSON;
- digest verification before browser download;
- same-origin guarded create, status, download, and delete endpoints;
- UI creation, sensitive-data warning, progress, download, and deletion; and
- 24-hour Pi-side expiry with retry after cleanup failure.

The current UI deletes the Pi-side archive immediately after a successful browser download. A later implementation slice must change that behavior for the private-intake path: review download retains the finalized archive until explicit deletion, successful workflow cleanup, or expiry.

Debian 13 provides the packaged `age` command. Its stable v1 format accepts a public recipient, emits authenticated binary ciphertext, reports full-operation success only with exit status zero, and preserves backwards decryption compatibility. Version 1 selects a classic native X25519 recipient because it is supported by Debian 13's packaged `age` 1.2.1. A future key rotation may adopt a newer recipient type only after every supported client can encrypt to it.

The existing build already links OpenSSL 3 `libcrypto`, which supports Ed25519 verification through EVP. No additional signing library is required for intake-manifest verification.

Dropbox File Requests accept uploads from users without Dropbox accounts and can be closed to disable the public link. The public request remains a browser handoff; the documented Dropbox APIs for managing requests do not establish an anonymous application upload contract suitable for embedding here.

## Identifiers

### Case ID

The case ID is a non-secret, human-transcribable correlation value:

```text
AAAA-BBBB-CCCC
```

Each character is uppercase Crockford Base32 excluding `I`, `L`, `O`, and `U`. Twelve symbols encode 60 random bits. Generation uses the operating system CSPRNG. The value is validated with:

```text
^[0-9A-HJKMNP-TV-Z]{4}-[0-9A-HJKMNP-TV-Z]{4}-[0-9A-HJKMNP-TV-Z]{4}$
```

The case ID MUST NOT encode a callsign, email address, locator, username, hostname, IP address, timestamp, hardware identifier, or application installation ID.

A case ID is allocated when a candidate collection is requested and remains stable across review, encryption, retry, and issue correlation. Creating a new diagnostic collection creates a new case ID.

### Job ID

The existing 32-character job ID remains an internal API/storage identifier. It is not shown as the support case ID and is not accepted as public correlation data.

### Artifact ID

Every encryption operation receives a 128-bit random lowercase hexadecimal artifact ID:

```text
^[0-9a-f]{32}$
```

Ordinary retry reuses the retained ciphertext and artifact ID. If ciphertext is lost while the finalized readable archive remains valid, re-encryption creates a new artifact ID under the same case ID and is recorded in a replacement receipt.

## Support context and candidate collection

Before collection, the user chooses one support-context path:

1. Existing GitHub issue: normalized `https://github.com/WsprryPi/WsprryPi/issues/<number>`.
2. New GitHub issue later: a meaningful problem description and user-approved contact method are required until the resulting issue number is known.
3. No GitHub account or GitHub declined: a meaningful problem description and user-approved contact method are required.

The application does not validate identity or ownership of contact information.

The context is written into the readable archive so the user can review exactly what the maintainer will decrypt. Dropbox's name and email fields are separate provider metadata and never satisfy this requirement.

## Readable archive contract

The existing `.tar.gz` remains the readable bundle. Its root adds:

```text
manifest.json
README.txt
```

`manifest.json` uses UTF-8 JSON, schema version 1, and contains:

```json
{
  "schema_version": 1,
  "contract_version": 1,
  "project_id": "wsprrypi",
  "project_version": "<display version>",
  "case_id": "7K3M-9QFX-2DPA",
  "created_at_utc": "2026-08-16T18:30:00Z",
  "collection_options": {
    "configuration_files_included": true,
    "full_logs_included": false,
    "i2c_probe_requested": false
  },
  "privacy_categories": ["callsign", "locator", "internal_ip", "logs"],
  "support_context": {
    "kind": "existing_github_issue",
    "issue_url": "https://github.com/WsprryPi/WsprryPi/issues/123",
    "problem_description": null,
    "contact": null
  },
  "collection_warnings": [],
  "files": [
    {
      "path": "system/uname.txt",
      "size": 123,
      "sha256": "<64 lowercase hexadecimal characters>"
    }
  ]
}
```

Rules:

- Paths are relative POSIX paths with no empty, `.`, `..`, absolute, backslash, control-character, or duplicate segments.
- `files` covers every regular diagnostic payload file except `manifest.json` itself.
- Entries are sorted bytewise by path.
- Sizes are non-negative integers and digests are lowercase SHA-256.
- Symlinks, hard links, devices, FIFOs, sockets, setuid/setgid bits, and absolute or traversal paths are prohibited.
- `README.txt` explains the support purpose, privacy review, case ID, and that the archive is readable local evidence rather than the file intended for public issue attachment.

The external result JSON remains a controller-to-collector result contract and is not a substitute for the internal manifest.

## Review and exclusions

Version 1 uses category-level collection choices, not an in-browser tar editor. The choices include the existing active I2C probe and any later approved configuration/log inclusion controls.

The workflow is:

1. Choose context and collection options.
2. Collect a candidate with a case ID.
3. Download the readable candidate without deleting the Pi-side bytes.
4. Review the archive and disclosed categories.
5. Either approve that candidate or reject it.

If the user changes an exclusion or collection option, the current candidate is deleted and a new candidate is collected with a new case ID. The new candidate requires a new review. Recollection is therefore allowed only before approval; after approval the finalized bytes are immutable.

The UI must not claim to know whether the user actually opened the archive. Consent means the user was given a reasonable review opportunity and explicitly chose to continue.

## Finalization and encryption

Approval transitions the retained candidate to `finalized`. Finalization records the archive size and SHA-256 already verified by the controller. The archive becomes read-only to the workflow; no collector, metadata, support context, compression, timestamp, or filename change is permitted afterward.

Encryption uses the Debian-packaged command in an argv-based child process without a shell:

```text
/usr/bin/age --encrypt --recipient <pinned-recipient> --output <exclusive-temp-path> <finalized-archive-path>
```

Requirements:

- The recipient is compiled or installed as public project data with a non-secret key ID and fingerprint.
- The executable path is fixed; `PATH` is not consulted.
- The output is created in the existing private job directory with mode `0600` through an exclusive temporary path.
- Existing output is never overwritten.
- Success requires normal exit status zero, a regular non-symlink file owned by the service user, mode `0600`, and a non-zero bounded size.
- The temporary output is atomically renamed only after validation.
- Failure deletes partial ciphertext and leaves the readable finalized archive available.
- Plaintext is never sent to Dropbox or substituted for failed ciphertext.

Encrypted filename:

```text
wsprrypi-support-<case-id>-<artifact-id>.tar.gz.age
```

The controller hashes the completed ciphertext and records its size and SHA-256 in the receipt. Structural `age` parsing is not treated as a substitute for an end-to-end decrypt fixture in tests.

## Local receipt

The browser can download a non-secret UTF-8 JSON receipt:

```json
{
  "schema_version": 1,
  "project_id": "wsprrypi",
  "case_id": "7K3M-9QFX-2DPA",
  "artifact_id": "0123456789abcdef0123456789abcdef",
  "created_at_utc": "2026-08-16T18:30:00Z",
  "archive_filename": "WsprryPi-support-<timestamp>.tar.gz",
  "archive_size": 26214400,
  "archive_sha256": "<64 lowercase hexadecimal characters>",
  "encrypted_filename": "wsprrypi-support-7K3M-9QFX-2DPA-0123456789abcdef0123456789abcdef.tar.gz.age",
  "encrypted_size": 26225000,
  "encrypted_sha256": "<64 lowercase hexadecimal characters>",
  "bundle_encryption_key_id": "wsprrypi-bundle-2026-01",
  "issue_url": null,
  "upload_state": "encrypted_artifact_downloaded"
}
```

The receipt contains no problem description, contact value, Dropbox name/email, private key, request URL, or diagnostic payload. The browser may update its downloaded copy only by downloading a replacement receipt; the server never claims to mutate an already downloaded file.

## State model

The protocol states are:

```text
queued
running
candidate_ready
candidate_downloaded
finalized
encrypting
encrypted_ready
encrypted_downloaded
upload_page_opened
upload_reported_complete
deleted
failed
expired
```

Allowed forward transitions:

```text
queued -> running
running -> candidate_ready | failed
candidate_ready -> candidate_downloaded | deleted | expired
candidate_downloaded -> finalized | deleted | expired
finalized -> encrypting | deleted | expired
encrypting -> encrypted_ready | finalized
encrypted_ready -> encrypted_downloaded | deleted | expired
encrypted_downloaded -> upload_page_opened | deleted | expired
upload_page_opened -> upload_reported_complete | deleted | expired
upload_reported_complete -> deleted | expired
```

Encryption failure returns to `finalized` with an actionable error; it does not discard the readable archive. Network, manifest, upgrade, or Dropbox failures do not move backward or regenerate artifacts. A new diagnostic collection is a new job and case.

`upload_reported_complete` means only that the user confirmed Dropbox displayed success. Maintainer receipt is external to the application state.

## Intake manifest

The client is compiled with:

- one or more HTTPS manifest endpoint URLs;
- the WsprryPi intake-signing Ed25519 public key and key ID;
- an integer upload protocol version; and
- permitted project ID and Dropbox request host rules.

The primary version-1 endpoint is reserved as:

```text
https://raw.githubusercontent.com/WsprryPi/support-intake/main/wsprrypi/intake.json
https://raw.githubusercontent.com/WsprryPi/support-intake/main/wsprrypi/intake.json.sig
```

The separate public repository and production files are established in the maintainer-tooling slice, not this slice. Test code receives endpoints through typed dependency injection only; no production CLI, INI, environment, query-string, or UI override is added.

`intake.json` is UTF-8 JSON with LF line endings. Verification covers the exact downloaded bytes, so JSON canonicalization is not required. The document is bounded to 16 KiB, contains no duplicate object keys, and contains only recognized top-level fields:

```json
{
  "schema_version": 1,
  "project_id": "wsprrypi",
  "generation": 1,
  "published_at": "2026-08-16T18:00:00Z",
  "expires_at": "2026-11-14T18:00:00Z",
  "status": "active",
  "minimum_client_protocol": 1,
  "minimum_upload_version": "1.3.0",
  "request_url": "https://www.dropbox.com/request/<opaque-id>",
  "release_url": "https://github.com/WsprryPi/WsprryPi/releases/latest",
  "user_message": null,
  "bundle_encryption_key_id": "wsprrypi-bundle-2026-01"
}
```

`intake.json.sig` is a bounded UTF-8 JSON signature envelope:

```json
{
  "schema_version": 1,
  "algorithm": "Ed25519",
  "key_id": "wsprrypi-intake-2026-01",
  "signature": "<unpadded base64url Ed25519 signature>"
}
```

The signature value decodes to exactly 64 bytes. The signature covers `intake.json` only, not the signature envelope.

## Intake validation

Validation order is:

1. Successful HTTPS fetch with normal certificate and hostname verification, redirect disabled, response-size limits, and a bounded timeout.
2. Parse and bound the signature envelope.
3. Match `algorithm` and pinned `key_id`.
4. Verify Ed25519 over the exact manifest bytes.
5. Parse strict JSON and reject duplicate or unknown top-level fields.
6. Match schema and `project_id`.
7. Require positive integer generation not lower than the highest previously accepted generation.
8. Validate UTC timestamps and require `published_at < expires_at`.
9. Require the current system time to fall between `published_at - 5 minutes` and `expires_at + 5 minutes`.
10. Validate status and version gating.
11. For `active`, require an HTTPS Dropbox request URL with exact host `www.dropbox.com`, path prefix `/request/`, no userinfo, non-default port, fragment, or unexpected query.
12. Require an HTTPS GitHub release URL and a recognized installed bundle-encryption key ID.

The controller persists the highest accepted generation and its manifest SHA-256 under private WsprryPi state. A lower generation is rejected as rollback. The same generation is accepted only when the exact manifest hash matches the cached value. A different manifest with the same generation is rejected.

If the device clock cannot validate the signed window, upload is blocked with a clock-specific recovery message. It is not reported as an upgrade requirement.

`disabled` omits `request_url`. It blocks handoff without blocking collection, review, or encrypted download.

`minimum_client_protocol` is the enforceable integer compatibility gate. `minimum_upload_version` is the user-facing minimum release string. If the client protocol is too old, the UI blocks handoff, does not reveal the request URL, preserves local artifacts, and offers `release_url`.

## Key rotation

Bundle-encryption and intake-signing keys are separate.

- Bundle key IDs use `wsprrypi-bundle-YYYY-NN`.
- Intake key IDs use `wsprrypi-intake-YYYY-NN`.
- Private material never enters this repository, the application, installer output, CI, logs, Dropbox, or the intake manifest.
- Public encryption recipients and manifest-verification keys are versioned application data.

Bundle-key rotation requires the application to recognize the manifest-selected public key ID. The maintainer retains old private identities while artifacts may remain. An unknown bundle key ID blocks encryption and handoff.

Signing-key rotation requires an application release that pins the replacement key before manifests signed only by it are published. A transition release may pin current and next keys with distinct IDs. The manifest cannot authorize its own new signing key.

## Dropbox handoff

Version 1 is a deliberate browser workflow:

1. Validate the current signed intake manifest immediately before handoff.
2. Download the retained `.age` artifact and receipt to the user's browser.
3. Explain that the browser chose the local save location.
4. Display the exact encrypted filename to select.
5. Present Dropbox's name/email metadata disclosure.
6. Open the File Request in a new browser context only after explicit user action.
7. Let the user select and upload the `.age` file on Dropbox.
8. Offer **I finished uploading** and **Keep for later** actions in WsprryPi.

The application does not automate Dropbox form completion, infer completion from opening or returning from the page, or receive Dropbox credentials. It never exposes the request URL before signed-manifest validation and version gating.

## Retention and deletion

The existing 24-hour Pi-side retention remains the maximum for candidate, finalized, and encrypted artifacts. All artifacts for a case live under one private job directory and are deleted together.

- Review download no longer auto-deletes the Pi-side candidate.
- Explicit **Delete from Pi** deletes readable archive, checksum, internal result, ciphertext, and server receipt together.
- After `upload_reported_complete`, the UI recommends deletion but requires explicit user action in version 1.
- Expiry deletes all Pi-side case artifacts and clears active state.
- Browser-downloaded files remain under user control; deletion from the Pi or Dropbox cannot delete them.
- Restart cleanup retains only directories that pass the future complete-state validation contract; ambiguous or partial encryption output is removed.

## Failure matrix

| Failure | Upload allowed | Readable archive retained | User recovery |
|---|---:|---:|---|
| Collection failed | No | No valid candidate | Retry collection |
| Candidate rejected by user | No | Until delete/expiry | Delete or recollect with new case ID |
| Encryption executable missing | No | Yes | Upgrade or repair installation |
| Encryption failed or partial output | No | Yes | Retry encryption |
| Manifest network unavailable | No | Yes | Retry later; save local artifacts |
| Signature/schema/project invalid | No | Yes | Retry later; report intake configuration failure |
| Manifest expired or clock invalid | No | Yes | Correct time or wait for maintainer renewal |
| Manifest disabled | No | Yes | Save bundle and retry when service returns |
| Client protocol too old | No | Yes | Upgrade from official release URL |
| Unknown bundle key ID | No | Yes | Upgrade to a client containing that key |
| Encrypted browser download failed | No | Yes | Retry the same artifact download |
| Dropbox unavailable | Not confirmed | Yes until delete/expiry | Keep downloaded `.age`; retry later |
| User reports upload complete | Externally reported | Yes until explicit delete/expiry | Retain case ID and receipt |

## Maintainer intake contract

The maintainer records the Dropbox filename, size, receipt time, project, case ID, and artifact ID. Processing then:

1. Decrypts with the private identity selected by key ID from the receipt or case record.
2. Hashes the decrypted `.tar.gz` and compares it with the receipt when available.
3. Lists and validates the archive before extraction.
4. Rejects absolute paths, traversal, duplicate paths, links, devices, FIFOs, sockets, permission escalation bits, and configured expansion, file-count, or per-file limits.
5. Reads and validates `manifest.json` before opening diagnostic payloads.
6. Correlates the case ID with the GitHub issue or encrypted support context.
7. Moves processed material out of `Incoming` and applies retention policy.

The Dropbox filename is advisory because Dropbox may append the submitter's name.

## Test fixtures and gates

Later implementation slices must provide:

- deterministic case-ID validation tests with injected random bytes;
- manifest schema/path/hash fixtures and malicious tar fixtures;
- exact-byte archive hash before encryption and after maintainer decryption;
- real `age` CLI integration using test-only keys and no production private identity;
- encryption interruption, partial output, ownership, mode, size, collision, retry, and cleanup tests;
- Ed25519 valid, corrupt, wrong-key, wrong-project, duplicate-key, unknown-field, expiry, clock-skew, generation rollback, same-generation mutation, disabled, upgrade, URL-restriction, redirect, timeout, and oversized-response tests;
- HTTP state-transition and guarded-route tests;
- browser UI tests for review, consent, download, Dropbox disclosure, GitHub and non-GitHub context, disabled, upgrade, retry, deletion, and truthful upload state;
- Impeccable desktop/mobile visual and accessibility validation in the UI slice; and
- a signed-out Dropbox acceptance test with byte comparison before release.

Production endpoints, keys, request IDs, and private identities are never test fixtures.

## Operator terminology

Use these terms consistently:

- **Readable support bundle**: the local `.tar.gz` the user can inspect.
- **Encrypted support bundle**: the `.age` file selected on Dropbox.
- **Case ID**: the non-secret support correlation value.
- **Upload page opened**: Dropbox was opened; no completion claim.
- **Upload reported complete**: the user says Dropbox displayed success.

Error hierarchy is: what failed, what remains safe, and the next action. Messages must explicitly say when the readable bundle was retained and when no upload occurred. Internal error codes remain secondary diagnostics.

Impeccable review found that this direct terminology and state hierarchy fits the existing precise, technical, restrained Maintenance interface. No UI rendering applies to this documentation-only slice.

## Explicit non-goals

This slice does not:

- implement runtime, API, installer, collector, UI, or maintainer tooling;
- upload directly to Dropbox or use Dropbox credentials;
- create anonymous GitHub issues;
- prove user identity;
- provide strong hostile-user rate limiting;
- change the existing CLI collector behavior;
- change RF, GPIO, transmitter, scheduler, service, or installation behavior; or
- update the separate operator-documentation repository.

## Slice 2 handoff

Slice 2 should implement the archive, identifier, support-context, finalization, encryption, receipt, retention, and cleanup primitives behind typed C++ boundaries, without adding the full UI or remote intake handoff.

It should begin with tests for case IDs, internal manifest validation, state transitions, exact-byte hashing, `age` execution, failure cleanup, and decrypt round-trip fixtures. Installer dependency work may add Debian's `age` package only after its dry-run, install, upgrade, and removal implications are reviewed.

Remote intake retrieval and Ed25519 verification remain a separate subsequent slice so encryption failures and routing-policy failures stay independently testable.

## Primary sources

- [`age` project documentation](https://github.com/FiloSottile/age/blob/main/README.md)
- [`age(1)` format and command contract](https://github.com/FiloSottile/age/blob/main/doc/age.1.html)
- [Debian 13 `age` package](https://packages.debian.org/trixie/age)
- [Debian 13 `age(1)` manual](https://manpages.debian.org/trixie/age/age.1.en.html)
- [Dropbox File Request documentation](https://help.dropbox.com/share/create-file-request)
