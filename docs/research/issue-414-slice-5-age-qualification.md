# Issue 414 Slice 5: Debian `age` Qualification

Status: Real packaged-ciphertext round trip qualified; production key and workflow integration deferred

Depends on:

- [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)
- [Slice 2 local artifact primitives](issue-414-slice-2-local-artifacts.md)
- [Slice 4 readable review and finalization](issue-414-slice-4-review-finalization.md)

## Outcome

The support-bundle encryption primitive now has a repeatable integration gate
against Debian Trixie's packaged `age` 1.2.1. The fixture creates and finalizes
known readable bytes, encrypts them through `encrypt_support_bundle()`, checks
the published ciphertext's filename, ownership, `0600` mode, size, and SHA-256,
decrypts it with the matching ephemeral identity, and proves exact byte
equality with the finalized input.

The same run writes and validates the bounded receipt, confirms that it omits
contact and problem-description data, and verifies that no partial artifact is
left behind. The driver generates its X25519 identity only under a private
temporary directory and removes that directory on every exit.

## Durable gate

`make support-bundle-age-roundtrip-test` is intentionally separate from
ordinary host tests because it requires the packaged `age` and `age-keygen`
commands. The canonical Debian non-hardware workflow installs `age` and runs
both the deterministic primitive suite and this real round-trip gate.

## Validation evidence

Passed in an ephemeral `debian:trixie-slim` container on 2026-08-16:

```text
support_bundle_private_artifact_test: PASS
age version: 1.2.1
support_bundle_age_roundtrip_test: PASS
ephemeral identity cleanup: armed
```

The driver also passed `bash -n` and ShellCheck inside the container. The source
worktree was mounted read-only and copied into the disposable container before
build, so the ephemeral identity and build output were not written into the
repository.

The macOS host compiled the new fixture and reran the deterministic primitive
suite. It did not run the real round trip because `age` is not installed there.

## Remaining gate

No production WsprryPi recipient, key ID, fingerprint, or private identity was
created or published. Before runtime encryption can be enabled, a separately
reviewed maintainer action must provision the project-specific key pair, retain
and back up the private identity outside the repository, and publish only the
versioned public recipient metadata. Lifecycle, HTTP, UI, encrypted download,
receipt download, signed-intake, and Dropbox integration remain later slices.

No installer, application runtime, UI, service, hardware, GPIO, I2C,
transmitter, RF, Dropbox, or GitHub-posting behavior changed in this slice.
