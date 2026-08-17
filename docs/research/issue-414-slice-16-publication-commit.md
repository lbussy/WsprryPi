# Issue 414 Slice 16: Publication Candidate Commit

Status: Local bare-repository commit boundary qualified; network push deferred

Depends on:

- [Slice 15 local manifest lifecycle](issue-414-slice-15-manifest-lifecycle.md)

## Outcome

The repository now contains a maintainer-only tool that authenticates the
highest local Slice 15 generation and records its exact manifest/signature pair
as one verified commit in a dedicated bare publication repository. It performs
no clone, fetch, push, or network request.

The tool holds a shared lock on the lifecycle staging root from authentication
through Git ref update and verification, preventing a cooperating lifecycle
writer from advancing local state during publication preparation. Signature
verification and deterministic manifest reconstruction still occur before any
manifest bytes are used.

The publication repository must be an absolute owner-controlled `0700` bare
repository with symbolic `HEAD` at `refs/heads/main`, exactly one ref
(`refs/heads/main`), exactly one remote (`origin`), and exactly one origin URL:

```text
https://github.com/WsprryPi/support-intake.git
```

Alternates, shallow state, grafts, and replacement refs are rejected. Production
Git is fixed to root-owned `/usr/bin/git`. Git system/global configuration,
replace objects, terminal prompting, and hooks are disabled for controlled
plumbing operations.

## Commit boundary

Without `--approve`, the tool validates source and destination and reports only
a non-sensitive summary. It creates no objects, index, commit, or ref update.

With approval, it uses a private temporary index to read the current tree,
write the exact two authenticated blobs, and replace only:

```text
wsprrypi/intake.json
wsprrypi/intake.json.sig
```

It writes one tree and one single-parent commit, then atomically advances
`refs/heads/main` with compare-and-swap against the previously validated commit.
A competing ref update wins and is never overwritten.

After update, the tool verifies the exact parent, exact two-path diff, and exact
committed blob bytes. Verification failure attempts a compare-and-swap rollback
from the new commit to the prior commit. Rollback failure is a distinct
`PublicationRollbackError`; success is never reported in either failure case.
Unreachable objects may remain after a failed approved attempt, but the public
branch ref remains unchanged unless rollback itself fails.

Commit messages and command output contain only generation, intake status,
public key IDs, manifest SHA-256, commit IDs, and fixed target paths. They omit
routing URLs, messages, signatures, key bytes, credentials, and private data.

## Validation

`make support-bundle-intake-publication-commit-test SUDO=` uses disposable local
bare repositories and reserved fake signed pairs. It covers dry-run object/ref
immutability, exact committed bytes, exact two-path scope, parent preservation,
wrong remote/branch/non-bare rejection, competing ref updates, source-lock
retention, post-update rollback, rollback failure classification, and CLI
non-disclosure. The same hardware-free suite runs in Debian CI.

## Remaining work

- Create and authorize the real public `WsprryPi/support-intake` repository as a
  separate external action.
- Use the Slice 17 authenticated push controller for exact remote parent/
  candidate decisions and lease-protected push; live use remains unauthorized.
- Retrieve both public raw files over the production HTTPS endpoints and verify
  exact bytes, signature, generation, and digest before declaring publication.
- Provision and approve production identities and public metadata before any
  real manifest is prepared.
- Package approved public trust and activate the application only afterward.

No real remote, network, credential, production key, metadata, manifest,
signature, Dropbox ID, runtime, HTTP, UI, installer, service, hardware, GPIO,
I2C, transmitter, or RF behavior changed in this slice.
