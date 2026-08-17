# Issue 414 Slice 12: Support Intake Runtime Construction

Status: Hardware-free runtime boundary implemented; application activation deferred

Depends on:

- [Slice 10 signed-intake controller](issue-414-slice-10-intake-controller.md)
- [Slice 11 minimum upload version gate](issue-414-slice-11-minimum-upload-version.md)

## Outcome

WsprryPi now has a dedicated runtime boundary that constructs the signed-intake
controller request from fixed production policy and explicitly supplied public
trust metadata.

Production construction uses:

- `/var/lib/wsprrypi/support-bundles`, the existing private owner-only support
  storage root, for `intake-state.json`;
- the build-metadata-backed `get_exe_version()` value;
- current `std::chrono::system_clock` UTC epoch seconds;
- intake client protocol 1; and
- the unchanged Slice 9 retrieval defaults, including exact endpoints,
  `/usr/bin/curl`, deadlines, and byte limits.

The trust input can contain only pinned Ed25519 public signing keys and
recognized public bundle-key IDs. Before invoking the controller, the runtime
requires nonempty bounded collections, exact project key-ID syntax, unique IDs,
and a nonzero 32-byte value for each signing public key. It also requires a
strict installed SemVer, positive UTC epoch, absolute state root, and complete
runtime dependencies.

Preflight failures occur before controller invocation and are reported only as
typed `invalid_trust_metadata` or `invalid_runtime_environment` states.
Dependency or controller exceptions are contained as `resolution_failed`. A
completed controller invocation preserves its typed result unchanged, including
success, disabled, upgrade, retrieval, validation, state, and durability
outcomes. Runtime errors contain no fetched bytes, URLs, signatures, public-key
bytes, or string diagnostics.

A typed in-process seam permits deterministic tests to inject version, clock,
state root, and controller callback. Production uses the concrete Slice 10
controller. There is no CLI, INI, environment, HTTP, or UI override.

The boundary is not called by `main`, `WebServer`, or support-bundle HTTP code,
so this slice performs no production retrieval or upload.

## Validation

`make support-bundle-intake-runtime-test SUDO=` covers:

- exact production constants and complete controller-request construction;
- fixed Slice 9 endpoints, executable, timeouts, and byte limits;
- missing, malformed, duplicate, zero-valued, and oversized public trust sets;
- invalid or missing state root, version, clock, and dependency providers;
- provider and controller exception containment;
- no controller invocation after any preflight failure; and
- unchanged propagation of every controller failure class, ordinary disabled
  success, and limited upgrade guidance.

The focused target is included in Debian non-hardware CI. It uses a controller
callback and performs no network, service, hardware, or RF activity.

## Remaining work

- Use the separately reviewed Slice 6 and Slice 13 maintainer tools to provision,
  back up, recovery-test, approve, and publish the two distinct production
  public identities.
- Compile the approved public metadata into versioned application data.
- Activate this runtime from the support workflow and translate its typed
  outcomes for later UI consumption.
- Compose archive encryption and Dropbox upload only after activation is
  qualified.

No production key, manifest, signature, recipient, Dropbox request ID, main/web
wiring, installer, UI, service, hardware, GPIO, I2C, transmitter, or RF behavior
changed in this slice.
