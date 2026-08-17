# Issue 414 Slice 11: Minimum Upload Version Gate

Status: Signed application-version policy implemented; runtime and UI deferred

Depends on:

- [Slice 7 offline signed-intake validation](issue-414-slice-7-intake-validation.md)
- [Slice 10 signed-intake controller](issue-414-slice-10-intake-controller.md)

## Outcome

The offline validator now requires the installed application version and
compares it with the authenticated manifest's `minimum_upload_version` using
SemVer 2 precedence. Both values are bounded and strictly parsed without
integer conversion, so arbitrarily large numeric identifiers cannot overflow.
Core versions, prerelease identifiers, and build metadata follow SemVer
precedence; build metadata does not affect ordering.

An empty, malformed, non-canonical, or oversized installed version is an
`invalid_client_version` failure. A well-formed installed version below the
signed minimum on an active manifest is the distinct `upgrade_required`
failure. A valid disabled manifest remains authoritative and is durably accepted
without upgrade guidance because it contains no upload route. Retrieval and
client protocol failures retain their existing categories and cannot become
upgrade requirements.

Exact-byte signature verification still occurs before manifest parsing or any
policy disclosure. The validator also completes project, schema, generation,
rollback, time, status/request URL, release URL, and bundle-key checks before it
returns upgrade guidance.

Only `upgrade_required` may carry a dedicated limited result containing:

- the authenticated minimum upload version;
- the validated official GitHub release URL; and
- the optional authenticated user message.

The ordinary manifest remains empty. The limited result has no request URL,
signature, fetched bytes, timestamps, generation, or signing/bundle key IDs.
Separately, validation supplies the controller only the authenticated generation
and exact manifest digest needed for rollback protection. The Slice 10
controller durably commits that state before propagating upgrade guidance and
continues to return no manifest. Commit failure, competing-writer rollback, or
uncertain durability returns no upgrade guidance; an identical retry may
confirm durability through Slice 8's `unchanged` path.

## Validation

The Slice 7 validation and Slice 10 controller tests cover:

- equal and newer major, minor, and patch releases;
- older releases and release-versus-prerelease ordering;
- numeric and lexical prerelease ordering plus build-metadata neutrality;
- extremely large numeric components without conversion or overflow;
- malformed, non-canonical, non-ASCII, and oversized versions;
- stale clients combined with bad signatures, wrong projects, expiry,
  rollback, invalid request/release URLs, and unknown bundle keys;
- authenticated upgrade guidance with request-URL non-disclosure; and
- stale installed version with a durably accepted disabled manifest and no
  upgrade guidance or request URL;
- durable higher-generation upgrade state followed by lower-generation replay
  rejection;
- competing-writer authority and uncertain-sync retry for upgrade state; and
- controller propagation with no manifest and unchanged categorization for
  protocol and other stage failures.

Both existing focused targets remain in Debian non-hardware CI; no new target is
needed. Tests use ephemeral signing keys and no network access.

## Remaining work

- Construct the controller from the installed application version, system UTC
  clock, approved public trust metadata, and private runtime state directory.
- Add runtime-facing status translation that preserves availability, clock,
  disabled, protocol, and upgrade-required distinctions.
- Add encryption/upload composition and then the operator UI in later slices.

No production keys, manifests, recipients, Dropbox request IDs, runtime wiring,
installer, UI, service, hardware, GPIO, I2C, transmitter, or RF behavior changed
in this slice.
