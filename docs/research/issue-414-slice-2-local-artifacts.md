# Issue 414 Slice 2: Local Artifact Primitives

Status: Implemented local foundation; not integrated into the support workflow

Depends on: [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)

## Outcome

This slice adds typed, hardware-free C++ primitives for the private-intake
workflow without changing the existing collector, job manager, HTTP API, or UI.

Implemented behavior:

- deterministic case-ID and artifact-ID generation with injectable entropy;
- strict WsprryPi GitHub and non-GitHub support-context validation;
- exact-size and SHA-256 finalization of an owned private readable archive;
- descriptor-pinned, read-only finalized input;
- shell-free encryption-process execution with private output, timeout,
  process-group cleanup, collision rejection, output validation, SHA-256, and
  no-overwrite publication;
- deterministic bounded receipt JSON that excludes description and contact
  data; and
- focused tests using a deterministic test-only encryption helper.

The production executable default is `/usr/bin/age`. No production recipient,
private identity, endpoint, Dropbox request ID, or credential is included.

## Safety properties

Finalization rejects symlinks, non-private modes, wrong ownership, empty or
oversized input, and digest mismatch. The finalized descriptor remains open and
is rehashed immediately before encryption, so the child reads the approved
inode rather than a later pathname replacement.

Encryption uses argv rather than a shell, gives the child an isolated process
group, applies a `0077` umask, and supplies the input through its inherited
read-only descriptor. Partial output is removed on process, timeout, type,
ownership, mode, size, digest, or publication failure. The readable archive is
not removed by any encryption or receipt failure.

Receipt publication and ciphertext publication reject existing destinations
and use a same-directory hard-link publication boundary followed by removal of
the private partial name. They never overwrite an existing artifact.

## Validation

Passed on the macOS development host:

```text
make support-bundle-private-artifact-test
support_bundle_private_artifact_test: PASS
```

The focused test covers deterministic entropy and failure, malformed IDs,
support-context rules, digest mismatch, read-only finalization, symlink
rejection, post-finalization mutation, exact-byte child input, successful
publication, collision, nonzero exit, empty output, wrong output mode,
oversized output, timeout, partial cleanup, ciphertext digest, receipt schema,
receipt collision, and unsafe receipt values.

Existing result-validator, archive-digest, and download-preparation tests also
passed with the repository's GCC-only flag removed for the macOS Clang run.
The existing job-manager and startup-cleanup tests reached pre-existing,
platform-sensitive assertions on this macOS host; this slice does not change
their source or dependencies.

## Validation still required

- A real Debian 13 packaged-`age` encrypt/decrypt round trip is required. The
  macOS development host does not have `age` installed, so this slice qualifies
  the process and file-safety boundary, not the `age` ciphertext format.
- Linux CI must compile and run the new focused target.
- Installer dependency behavior must be reviewed before adding `age` to the
  installed product.
- Job state, restart recovery, HTTP, UI, signed-intake, Dropbox handoff, and
  maintainer decryption remain later slices.

## Non-goals retained

No UI, endpoint, remote-network, Dropbox, GitHub-posting, service, installer,
hardware, GPIO, I2C, transmitter, or RF behavior changes in this slice.
