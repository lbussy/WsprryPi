# Issue 414 Slice 27: Production Publication Candidate

Status: Local candidate committed and verified; live push deferred

## Outcome

The authenticated production generation-1 manifest and signature are now
recorded as one local commit in the owner-controlled bare `support-intake`
publication repository. The repository's `main` ref advanced exactly once:

```text
previous: 770d63521cf23d1ccb5eb7c9911e040ab18032d7
candidate: 3e0b4017bfe7612bd39ccc6e2f29d743174664b5
```

The candidate is a single-parent commit whose parent is the README-only initial
commit. It changes exactly:

```text
wsprrypi/intake.json
wsprrypi/intake.json.sig
```

`README.md` remains present and byte-identical. The repository still contains
only `refs/heads/main`, and the local candidate has not been pushed.

## Authenticated source

The Slice 16 controller re-authenticated the source through the Slice 15
lifecycle boundary before using any signed bytes. The non-sensitive identity is:

- generation: `1`;
- status: `active`;
- signing key ID: `wsprrypi-intake-2026-01`;
- bundle key ID: `wsprrypi-bundle-2026-01`; and
- manifest SHA-256:
  `80902216b212ca1a8c2a9fd3e9693aac2c0aa17c7838d939bbebaa8887fb71e8`.

A pre-approval run produced status `proposed`. The branch ref and complete Git
object inventory were identical before and after that run. The approved run
used the qualified compare-and-swap controller and returned `committed`.

The first approved invocation ran inside a filesystem sandbox that denied the
external repository write. It failed before creating an object or advancing the
ref, as confirmed by the unchanged ref and object inventory. The explicitly
authorized retry outside that restriction produced the candidate above.

## Independent verification

Controlled local Git plumbing independently established that:

- the candidate has exactly the expected parent;
- the parent-to-candidate diff contains exactly the two fixed intake paths;
- both committed blobs are byte-identical to the authenticated staged files;
- `README.md` has the same SHA-256 before and after the candidate;
- `refs/heads/main` is the only ref and resolves to the candidate; and
- post-commit lifecycle inspection still authenticates the unchanged staged
  generation with the same policy, IDs, and manifest digest.

The manifest and signature contents were not displayed. The staging root and
generation directory remain owner-controlled mode `0700`; both source files
remain owner-owned, regular, single-link mode `0600` files.

Focused validation passed:

- manifest lifecycle: 10 tests, one real-tool fixture skipped locally;
- publication commit: 8 tests;
- Python syntax and final diff checks.

Known macOS Make probes for Linux `/proc/meminfo` and `nproc` emitted warnings
without affecting the tests.

## Publication boundary and remaining work

No `support-intake` clone, fetch, push, remote query, public raw-file retrieval,
Dropbox change, or application activation occurred. The public
`support-intake` GitHub repository therefore remains unchanged by this slice.

The next separately reviewed step is the Slice 17 authenticated live push. It
must independently establish the remote parent and use the qualified
lease-protected push boundary. Exact public-byte verification remains a
separate Slice 18 operation after a successful push.

No UI, installer, service, Raspberry Pi, GPIO, transmitter, or RF state changed.
