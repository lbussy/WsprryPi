# Issue 414 Slice 17: Authenticated Publication Push

Status: Push protocol qualified with fake transport; live GitHub push deferred

## Outcome

The maintainer-only push controller authenticates the highest local lifecycle
generation, verifies the exact Slice 16 candidate, and compares exact remote
`refs/heads/main` identity. Remote main may equal only the candidate parent
(eligible), candidate (`already_published`), or neither (`remote_conflict`).
Missing, multiple, malformed, and failed queries are rejected.

Approved publication uses one push with an exact
`--force-with-lease=refs/heads/main:PARENT` and exact candidate-to-main refspec,
then re-queries. Only candidate identity is `published`; rejection and
`pushed_confirmation_uncertain` remain distinct, with no automatic rollback.

Git execution is shell-free, disables hooks, system/global configuration,
replacement objects, prompting, and proxies, resets credential helpers, and
enables one allowlisted helper: `osxkeychain`, `libsecret`, or `manager-core`.
Credentials are never accepted, read, printed, or placed in argv/environment.

## Validation

`make support-bundle-intake-publication-push-test SUDO=` uses a typed fake remote
and disposable repositories. It covers proposal, publication, idempotency,
conflicts, malformed refs, rejection, confirmation uncertainty, source
mutation, staging-lock retention, credential-helper rejection, exact push argv,
and non-disclosure. It runs with zero network access locally and in Debian CI.

## Remaining work

- Separately authorize and create the real `WsprryPi/support-intake` repository.
- Provision and approve production identities and public metadata.
- Add public raw-file retrieval and exact-byte/signature verification.
- Only then authorize a first live preparation, commit, push, and application
  trust/activation sequence.

No real remote, network, credential, production material, runtime, HTTP, UI,
installer, service, hardware, GPIO, I2C, transmitter, or RF behavior changed.
