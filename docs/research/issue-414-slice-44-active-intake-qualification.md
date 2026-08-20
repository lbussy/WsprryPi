# Issue #414 Slice 44 — Release-like Active-Intake Qualification

Date: 20 August 2026

## Outcome

Current `devel` was merged into the clean Issue #414 integration branch. An
isolated clone at merge commit `7d88741561c9d81398e489a6f653079e48a8d97b`
used branch `main` and exact local-only tag `v3.2.1-qualification.1`. Build
metadata and the target binary both reported exact version
`3.2.1-qualification.1` with a clean source state.

The guarded installer completed on `wspr4` and created `~/finished`. The
installed service is active and enabled, the configured transmit gate remains
false, and the service log confirms scheduling and transmission were skipped.
The ordinary localhost support-intake endpoint returned HTTP 200, generation 1,
minimum upload version `3.2.0`, and status `active`. Validation recorded only
that a request URL was present; its value was not printed or retained.

## Validation

Local validation passed for build-metadata generation, GNU Make metadata
regression, the focused signed-intake chain, real Homebrew-age round trip, HTTP
endpoint, server wiring, UI source behavior, and diff checks. Apple Make's lack
of GNU `--output-sync` and sandboxed localhost binding were explicitly rerun
with GNU Make and normal localhost permissions.

The isolated `wspr4` checkout passed age dependency, runtime installer, real
Debian-age round trip, intake validation/state/retrieval/controller/runtime/
production, HTTP, and server-wiring tests before installation. `/usr/bin/age`
and `/usr/bin/age-keygen` are root-owned regular `0755` files. The private state
root is root-owned `0700`, and `intake-state.json` is root-owned `0600`.

The first guarded installer invocation failed before changing production because
SSH supplied an unknown terminal type. A second attempt compiled successfully
but failed staging because the isolated clone's local-path origin caused an
incorrect executable name; the installer restarted the unchanged old service
and left the completion marker absent. Correcting only the isolated clone's
origin metadata to the canonical public repository produced `wsprrypi` with the
verified exact version. The final guarded installation succeeded. These
failures were not concealed as qualification evidence.

The production `/home/pi/WsprryPi` checkout remained clean on synchronized
`devel`. No reboot, GPIO, I2C, transmitter, RF, Dropbox upload, support-data
processing, or support-data deletion occurred.

## Remaining boundary

`wspr4` is ready for the fresh signed-out Dropbox upload exercise. That exercise
must retain one artifact and its receipt for the subsequent maintainer
inspection/promotion transaction. Operator and maintainer documentation in the
separate `Wsprry_Pi_Docs` repository remains separately authorized work.
