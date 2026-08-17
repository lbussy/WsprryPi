# Issue 414 Slice 12: Support Intake Runtime Construction Prompt

## Objective

Implement and qualify the hardware-free runtime boundary that constructs and
invokes the signed-intake controller using the installed application version,
UTC system clock, fixed private support-bundle storage root, fixed retrieval
policy, and explicitly supplied public trust metadata. Stop before main/web
server activation, production trust publication, encryption/upload, or UI.

## Verified context

- Slice 10 composes state load, exact HTTPS retrieval, signature/policy
  validation, and durable rollback-state commit.
- Slice 11 requires a canonical installed SemVer and provides replay-safe,
  durably committed upgrade guidance.
- The existing support-bundle runtime and installer own the fixed private
  owner-only root `/var/lib/wsprrypi/support-bundles`; startup cleanup ignores
  non-job names, including Slice 8's `intake-state.json`.
- `get_exe_version()` is the existing build-metadata source for the installed
  executable version.
- Production signing public keys and bundle-encryption public metadata have not
  yet been approved or published and MUST NOT be invented in this slice.

## Scope and required behavior

1. Add a dedicated typed intake-runtime boundary separate from job collection.
   Its trust input contains only pinned Ed25519 public signing keys and
   recognized public bundle-key IDs. It must have no field capable of carrying
   a signing or decryption private key, Dropbox request URL, or bearer secret.
2. Production construction SHALL use:
   - `/var/lib/wsprrypi/support-bundles` as the fixed rollback-state root;
   - `get_exe_version()` as the installed upload version;
   - current `std::chrono::system_clock` UTC epoch seconds;
   - client protocol 1; and
   - the Slice 9 default fixed HTTPS endpoints, curl path, byte limits, and
     deadlines without override.
3. Validate public trust metadata before any controller/retrieval call: require
   nonempty, unique, syntactically valid project key IDs; require each signing
   key to contain a nonzero 32-byte public value; reject malformed, duplicate,
   empty, or oversized trust collections.
4. Validate the installed version with the same strict SemVer parser used by
   Slice 11, and require a positive representable UTC epoch value. Invalid
   runtime inputs fail before controller/retrieval invocation.
5. Return a typed runtime status distinguishing invalid trust metadata, invalid
   runtime environment, controller completion, and unexpected resolution
   failure. Preserve the complete typed controller result only after an actual
   controller invocation; do not add strings, fetched bytes, URLs, signatures,
   public-key bytes, or sensitive diagnostics to runtime errors or logs.
6. Production resolution must call the concrete Slice 10 controller. Provide a
   typed in-process dependency seam only for deterministic tests to inject
   version, clock, state root, and controller callback. Add no CLI, INI,
   environment, HTTP, or UI override.
7. Tests must prove exact production constants and constructed request fields,
   all trust/version/time preflight failures before invocation, collection
   bounds, exception containment, unchanged propagation of success, disabled,
   upgrade, retrieval, validation, state, and durability controller outcomes,
   and absence of network access.
8. Add a focused Make target and Debian non-hardware CI coverage. Document the
   implemented boundary and remaining production-metadata/runtime-wiring work.

## Constraints and non-goals

- Do not add, generate, select, or publish production signing keys, bundle
  recipients, manifests, signatures, Dropbox IDs, or request URLs.
- Do not call this runtime from `main`, `WebServer`, support-bundle HTTP routes,
  collection, encryption, or upload code.
- Do not change the installer or create/chmod/chown the production state root;
  use the already provisioned private support-bundle root contract.
- Do not add retries, caches, background refresh, logs containing intake data,
  UI, operator screenshots, GitHub posting, services, hardware, GPIO, I2C,
  transmitter, or RF behavior.
- Tests must not contact GitHub, Dropbox, or any external endpoint.

## Validation and evidence

- Run Slice 7 through Slice 12 focused tests locally.
- Run the new runtime target in a clean Debian container without network access.
- Run `git diff --check`, inspect the complete staged diff, and obtain an
  independent adversarial review. Correct every actionable finding and repeat
  review until no blockers remain.

## Exit criteria

Stop with a reusable runtime boundary that can safely consume later-approved
public trust metadata but is not yet activated by the application. Commit and
push only attributable Slice 12 files. Production metadata, main/web wiring,
encryption/upload composition, and UI remain separate slices.
