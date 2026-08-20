# Issue #414 Slice 40 — wspr4 production installation and non-RF qualification

## Objective

Install the Issue #414 integration branch on `wspr4`, restart the `wsprrypi`
service, and qualify the production signed-intake boundary without transmission,
GPIO, I2C, transmitter, or RF activity. Correct attributable defects found by
the installation and repeat the focused qualification until clean.

## Safety and scope

Confirm the target's configured transmission policy is disabled before
installation. Use an isolated exact-branch checkout and the repository's
installer completion-marker contract. Install only after explicit operator
approval. Do not reboot, change radio configuration, enable transmission, probe
hardware, or exercise RF output. Preserve the target's normal development
checkout.

The installed feature-branch version is expected to compare below the public
manifest's canonical `3.2.0` minimum. Qualify that production endpoint as the
authenticated, non-disclosing `upgrade_required` path. Use a separately built,
typed, non-disclosing diagnostic with canonical version `3.2.0` to prove the
active validation path without publishing or opening the request URL.

## Validation

Run the focused age dependency, installer, encryption round-trip, signed-intake,
and endpoint tests before installation. After restart, verify the service is
active and enabled, transmission remains disabled, fixed age executables and
state storage have safe ownership and modes, the localhost endpoint returns the
expected limited response, rollback state is durable, and repeated canonical
active resolution succeeds. Inspect service logs and report any non-attributable
operational limitation separately.

Review the complete diff adversarially, correct every attributable finding, run
focused local and target checks, update the Issue #414 record and roadmap, then
commit and push only the attributable slice files on the current integration
branch.
