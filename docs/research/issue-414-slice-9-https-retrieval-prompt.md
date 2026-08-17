# Issue 414 Slice 9: Bounded HTTPS Intake Retrieval Prompt

## Objective

Implement and qualify the shell-free transport that retrieves the exact signed
intake manifest and detached signature-envelope bytes from the reserved WsprryPi
HTTPS publication location. Stop before signature validation orchestration,
rollback-state commit, production keys/files, runtime wiring, or UI.

## Verified context

- Slice 1 reserves paired `raw.githubusercontent.com/WsprryPi/support-intake`
  endpoints for `wsprrypi/intake.json` and `intake.json.sig`.
- Debian installs `/usr/bin/curl`; WsprryPi already uses curl operationally, but
  no application retrieval primitive exists.
- Slice 7 validates exact supplied bytes; Slice 8 persists accepted generation
  and hash. This slice must not parse or normalize either fetched document.

## Scope

1. Add a typed C++ retrieval primitive that accepts the paired reserved HTTPS
   endpoints, an absolute fixed curl executable, explicit connection/overall
   deadlines, and the Slice 1 byte limits.
2. Strictly require the exact allowed scheme, host, repository path, project
   path, filenames, and paired base location. Reject userinfo, ports, queries,
   fragments, alternate hosts, encoded ambiguity, control bytes, and non-HTTPS.
3. Launch curl directly without a shell and with a minimal controlled child
   environment. Disable curl configuration files, require HTTPS, use ordinary
   CA and hostname verification, do not pass insecure flags, and do not follow
   redirects.
4. Capture stdout as binary exact bytes, discard diagnostic stderr, enforce the
   independent 16 KiB manifest and 2 KiB signature limits while streaming, and
   return no partial bytes on any failure.
5. Bound connect and whole-operation time, terminate and reap timed-out or
   oversized children, require exit status zero, and retrieve the two documents
   independently so a second failure clears both from the result.
6. Add a compiled fake-curl fixture that verifies argv/environment and exercises
   success, empty, oversized, nonzero, signal, timeout, and partial-output
   failures without network access.
7. Add the focused target to Debian non-hardware CI and document the boundary.

## Constraints and non-goals

- Do not publish or fetch a production manifest, signature, key, recipient, or
  Dropbox request URL.
- Do not add endpoint overrides through CLI, INI, environment, query strings, or
  UI. Test injection remains a typed C++ executable-path seam only.
- Do not parse JSON, verify Ed25519, commit rollback state, retry, cache, persist
  response bytes, wire runtime construction, change installation, or add UI.
- Do not use a shell, expose response bytes in argv/logs/errors, follow redirects,
  disable TLS verification, inherit proxy/CA override variables, or fall back to
  plaintext HTTP.
- Do not perform service, hardware, GPIO, I2C, transmitter, or RF activity.

## Validation

- Focused C++ tests must inspect the fake child's complete argv and controlled
  environment and prove exact bytes plus every cleanup/failure branch.
- Run on macOS and clean Debian without network access.
- `git diff --check`, staged-diff review, and independent adversarial review;
  correct all actionable findings before commit.

## Exit criteria

Stop with a reusable retrieval primitive and documented outcome. Commit and push
only attributable Slice 9 files. Controller composition with Slices 7 and 8,
production publication, runtime wiring, and UI remain later slices.
