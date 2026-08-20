# Issue #414 Slice 40 — wspr4 production installation and non-RF qualification

## Outcome

The Issue #414 integration branch was installed on `wspr4` and the `wsprrypi`
service was restarted. Transmission remained disabled and no reboot, GPIO, I2C,
transmitter, or RF operation was performed.

Two installation/target defects were corrected. The age dependency validator
now parses metadata correctly under the installer's intentionally restricted
global `IFS`, and retains only a typed, path-free failure reason for debug
diagnosis. Signed-intake retrieval now pins curl to IPv4 for the two fixed public
GitHub endpoints. This avoids the demonstrated target failure where the resolver
returned IPv6 addresses but the host had no IPv6 route; TLS certificate,
hostname, CA, URL, protocol, redirect, size, and timeout policies remain intact.

## Qualification evidence

The isolated target checkout passed the focused age dependency, runtime
installer, age round-trip, production intake, HTTP, and HTTPS retrieval tests.
The installer completed successfully and created its guarded completion marker.
The installed service is active and enabled, `/usr/bin/age` and
`/usr/bin/age-keygen` are root-owned regular `0755` files, and the private state
root is root-owned `0700`.

The installed feature-branch build correctly returned HTTP 200
`upgrade_required`, with only the minimum version and authenticated release
location, because its prerelease SemVer sorts below the manifest minimum
`3.2.0`. It persisted the authenticated generation-one state as a root-owned
`0600` file. A typed diagnostic using canonical installed version `3.2.0`
resolved the same production manifest three consecutive times as active and
ready, with unchanged durable state. No request URL, signature, manifest bytes,
key material, or personal diagnostic data was printed or retained as evidence.

The restart exposed an existing service-lifecycle limitation: the prior daemon
logged its normal exit but systemd still found subordinate tasks after the
ten-second stop deadline and killed the old control group. The replacement
service started successfully and reports a successful current result. This is
not an intake activation or RF failure, but it remains separate service shutdown
work rather than being concealed as Slice 40 completion.

## Documentation impact

This implementation record and the private-intake roadmap now distinguish the
qualified production installation and signed-intake boundary from the remaining
human/provider workflow. The separate operator documentation repository was
reviewed but not changed because cross-repository writing was not authorized.

## Remaining boundary

The signed-out Dropbox submission, maintainer receipt confirmation, decrypted
inspection/promotion exercise, retention enforcement, final contract
reconciliation, and operator/maintainer documentation remain. The feature-branch
installation intentionally cannot expose the active request URL through its
ordinary endpoint until a canonical release satisfying `3.2.0` is installed.
