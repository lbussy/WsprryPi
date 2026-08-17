# Issue 414 Slice 18: Public Publication Verification

## Outcome

This slice adds an inactive maintainer controller that verifies a Slice 17
publication through the two fixed public `raw.githubusercontent.com` paths. A
publication is `verified` only after the controller reauthenticates the local
manifest, verifies the expected candidate commit, retrieves both public files,
compares both bodies byte for byte, and independently authenticates the fetched
manifest/signature pair with the pinned public signing metadata.

The production transport invokes the fixed root-owned `/usr/bin/curl` without a
shell. Curl configuration and proxy use are disabled; only HTTPS is permitted;
ordinary certificate and hostname validation remain enabled; redirects are not
followed; connect and total deadlines apply; HTTP status must be 200; and the
manifest and signature-envelope bodies have independent bounds.

Results are limited to typed status, generation, manifest SHA-256, and candidate
commit. Retrieval failures, byte mismatches, authentication failures, and local
validation failures remain distinct and expose no fetched content or routing,
message, signature, or key fields.

## Validation boundary

`make support-bundle-intake-publication-verification-test SUDO=` uses only a
typed fake transport and disposable local Git repositories. It covers both
endpoint failures, oversize classification, partial/mutated/swapped bodies,
independent authentication failure, candidate mismatch, staging-lock retention,
repeat verification, and result non-disclosure. It never contacts GitHub.

## Deferred production work

This slice does not create or contact `WsprryPi/support-intake`, use credentials,
publish production material, activate the application runtime, or exercise a
service, installer, Raspberry Pi, GPIO, transmitter, or RF path. Production use
still requires the separately approved repository, backed-up private signing and
bundle identities, Slice 19 compilation of reviewed public trust metadata, an
actual Slice 17 push, and an explicit invocation of this verification controller.
