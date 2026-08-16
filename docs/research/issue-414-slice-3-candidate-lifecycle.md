# Issue 414 Slice 3: Candidate Manifest and Lifecycle

Status: Implemented opt-in candidate boundary; HTTP and UI integration deferred

Depends on:

- [Slice 1 protocol contract](issue-414-slice-1-protocol-contract.md)
- [Slice 2 local artifact primitives](issue-414-slice-2-local-artifacts.md)

## Outcome

The collector can now create a private-intake readable candidate with a case ID,
review instructions, support context, and a deterministic internal
`manifest.json`. Legacy collection remains supported and does not add the
internal protocol manifest.

The external result records `case_id` and `manifest_included`. Result validation
accepts both the historical result shape and the new explicit legacy shape, but
requires private metadata to be complete and consistent. The job manager retains
a validated case ID and internal `candidate_ready` lifecycle marker. Current
HTTP serialization deliberately continues to expose the existing `succeeded`
state and does not disclose the new fields.

## Private collector interface

Private mode requires `--case-id` and either:

- `--github-issue` with a normalized WsprryPi issue URL; or
- `--context-kind` plus private description and contact files.

Description and contact values are not command-line arguments. The collector
requires their files to be absolute, regular, non-symlink, owned by the caller,
not group/world writable, bounded, valid UTF-8, one-line, and free of control
characters.

## Archive manifest

The manifest contains schema and contract version 1, project and case metadata,
collection options, privacy categories, support context, warnings, and a
bytewise-sorted inventory of every regular payload file. Each inventory entry
has its relative path, exact size, and lowercase SHA-256.

Symbolic links copied from diagnostic sources are omitted and recorded as a
warning. Other special nodes and multiply linked regular files fail private
collection. Context staging files are never copied into the archive.

Private `README.txt` and `NEXT-STEPS.txt` identify the `.tar.gz` as the readable
review copy and instruct the user to upload only the later `.age` artifact.

## Compatibility boundary

This slice adds no way for the current HTTP/UI request to supply private
metadata. The opt-in collector interface and internal manager lifecycle are
dormant integration seams for the next application slice. Existing HTTP JSON,
download behavior, deletion behavior, and UI source are unchanged.

## Validation

Passed in an offline Debian Trixie Podman container:

- `scripts/tests/collect-support-bundle_test.sh`
- `make support-bundle-result-validator-test`
- `make support-bundle-job-manager-test`
- `make support-bundle-http-test`
- `make support-bundle-runtime-test`
- `make support-bundle-private-artifact-test`

The collector test covers GitHub and non-GitHub context, JSON escaping, file
inventory order/size/hash, context non-leakage, legacy output, malformed and
conflicting metadata, multiline rejection, symlinked context rejection, and
the existing collector/process/failure matrix.

`bash -n`, ShellCheck, and `git diff --check` also pass. The cached container
image lacked the nonessential `file` utility used by one legacy isolated-PATH
fixture, so that run supplied a container-only `/usr/bin/true` stub for `file`;
the repository and production collector were not changed for the image defect.

## Remaining work

- Add guarded HTTP input and UI workflow for support context and collection
  choices.
- Generate the case ID in the application rather than requiring the collector
  caller to provide it.
- Retain readable candidates after review download and implement explicit
  approval/finalization transitions.
- Connect finalization and encryption primitives from Slice 2.
- Validate a real packaged-`age` round trip on Debian.
- Add signed intake retrieval, version gating, Dropbox handoff, and operator
  documentation in later slices.

No installer, service, hardware, GPIO, I2C, transmitter, RF, remote-network,
Dropbox, or GitHub-posting behavior changed in this slice.
