# Issue 414 Slice 17 Execution Prompt: Authenticated Publication Push

## Objective

Add a maintainer-only controller that verifies one Slice 16 publication
candidate, compares the remote `main` identity with the candidate's exact
parent, and performs an approved HTTPS Git push protected by an exact
force-with-lease. Qualify all remote behavior through a typed fake transport;
do not contact or create the real public repository.

## Verified context

- Slice 16 creates a single-parent local candidate commit in a dedicated bare
  repository and verifies its exact two-path content.
- The candidate parent is the only acceptable pre-push remote `main` value.
- GitHub repository creation, credentials, live push, and public raw-file
  retrieval have not been authorized.

## Scope

1. Add `scripts/maintainer/push_support_bundle_intake_publication.py`.
2. Reuse Slice 15 source authentication and Slice 16 repository/candidate
   verification while holding a shared staging lock.
3. Require local `main` to be one unambiguous candidate commit whose single
   parent contains no intake-path changes relative to the authenticated exact
   source except the two fixed publication paths.
4. Query only exact remote `refs/heads/main`. Require one canonical object ID.
   Accept only:
   - parent: eligible to push;
   - candidate: idempotent `already_published` success;
   - anything else or absent/multiple/malformed: fail closed as remote conflict.
5. Without `--approve`, perform authenticated source/local/remote validation and
   report `proposed`; do not push or change local or remote refs.
6. With approval and remote at parent, invoke one fixed push equivalent to:
   `git push --porcelain --force-with-lease=refs/heads/main:PARENT origin
   CANDIDATE:refs/heads/main`.
7. Isolate Git system/global configuration, disable hooks, URL rewriting,
   prompting, replacement objects, SSH, proxies, and optional filters. Permit
   credentials only through an explicit allowlisted Git credential-helper name;
   never accept, read, print, copy, or place credentials in argv/environment.
8. Re-query remote `main` after a reported successful push. Return `published`
   only for exact candidate identity. A failed or ambiguous confirmation after
   push returns `pushed_confirmation_uncertain`; never attempt an automatic
   remote rollback.
9. A remote conflict, push rejection, transport failure before success, or
   malformed response returns a distinct typed state with no manifest, URL,
   message, signature, key-byte, credential, or remote diagnostic disclosure.
10. Keep production transport fixed and shell-free. Confine injected transport
    to a separately named typed test entry point.
11. Add adversarial fake-transport tests for proposed, published, already
    published, parent mismatch, absent/multiple/malformed refs, lease argv,
    credential-helper allowlist, rejection, exception containment, confirmation
    mismatch/failure, source/candidate mutation, staging-lock retention,
    idempotent retry, and CLI non-disclosure.
12. Wire the focused test into Make and Debian non-hardware CI and add a truthful
    implementation record and roadmap link.

## Constraints

- Do not create, clone, fetch, push, or contact a real GitHub repository during
  implementation or validation.
- Do not request or handle a GitHub token, password, cookie, SSH key, credential
  output, or Keychain content.
- Do not add production keys, metadata, manifests, signatures, Dropbox IDs, or
  request URLs.
- Do not implement repository creation, branch protection, release automation,
  public HTTPS retrieval, application activation, HTTP, UI, installer, service,
  Raspberry Pi, GPIO, I2C, transmitter, or RF behavior.

## Validation and evidence

- Run fake-transport tests locally and in clean Debian with zero network access.
- Prove exact lease/refspec argv, no push on dry-run/conflict/already-published,
  source-lock retention, idempotent retry, and uncertain confirmation behavior.
- Run Python syntax compilation and `git diff --check`.
- Adversarially review candidate identity, remote parsing, configuration and
  credential boundaries, proxy/URL rewrite resistance, push truthfulness,
  retry semantics, output disclosure, tests, and docs. Correct all findings.

## Exit criteria

- The push decision and exact lease protocol are fully qualified without a live
  remote or credentials.
- Only exact confirmed candidate identity is reported as published.
- No external repository or production material is changed.
