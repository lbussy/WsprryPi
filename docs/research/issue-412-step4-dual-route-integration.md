# Issue 412 Roadmap Step 4: ABI-v2 dual-route integration

This hardware-free development integration consumes RP1-GPCLK-DKMS source
revision `9ec6bb617d8259df50b376bb08f0e5973a8fee41`, module development version
1.1.2, canonical ABI-v2 UAPI SHA-256
`998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384`,
and Qualification Harness revision
`18246c76d2918dbbf2358ba01df0872839856d53`.

The only accepted compatibility identities are:

- GPIO4: `v1.1.2-pi5-gpio4-6.18.34-development-candidate-r2`
- GPIO20: `v1.1.2-pi5-gpio20-6.18.34-development-candidate-r2`

Both remain `Experimental`. The unreleased 1.1.2 source identity is not a final
Debian package, tag, product inventory, or public compatibility identity. The
released 1.1.1 package and its GPIO4/GPIO20 output-inhibited evidence are
retained as historical predecessor evidence only and cannot satisfy current
development compatibility, authorization, or qualification.

## Policy and route state

Requested, persisted, boot-configured, active-overlay, module-reported,
reconciled, live-eligible, and operator-confirmed routes remain distinct. The
provider can be acquired only after exact agreement, exactly-one-route
topology, an attributable resolved route transaction, affirmative
`live_output=1`, idle scheduling, endpoint exclusivity, and a current one-use
operation/route/identity confirmation. ABI v1 is diagnostic only. No route,
backend, MMIO, `/dev/mem`, or authorization fallback exists.

The UI reports route-manager and development state separately. Installation,
route reconciliation, module presence, an endpoint, a saved setting, a plan,
or historical evidence never implies development eligibility or product
qualification.

## External Harness plans

`scripts/rp1_gpclk_step4_plans.py` creates a new immutable destination and five
distinct application plans: finite Tone, WSPR, QRSS, FSKCW, and DFCW. It uses
only the published `wsprrypi-qualification validate-application-plan` command
with structured arguments; no Harness implementation is imported or copied.
Each plan binds the exact route-specific identity, 2 mA drive, WsprryPi source
and executable identity, adapter revision, DKMS revision, and Harness revision.
The finite carrier has an explicit one-second kernel duration.
Each application plan has a separate hardware-free validation record containing
the actual plan-file digest, launch, cancellation, lifecycle, cleanup, and
non-execution result. Capture, analysis, physical-path, operator-window,
authorization-digest, and immutable artifact-destination values remain explicit
`STEP5_REQUIRED` placeholders rather than invented hardware facts.

Generated hardware-free results are non-authorizing and nonqualifying. A new
destination is mandatory, and semantic validation must pass. GPIO4 and GPIO20
plans and evidence do not transfer between routes.

## Step 5 boundary

Before any live operation, a separately reviewed plan must replace every
`step5_required_values` entry with an exact operator window, target and receiver
identity, physical conducted path, attenuation and safe-level basis,
calibration decision, frequency and timing bounds, immutable destination,
cleanup and terminal-silence procedure, and authorization of the final plan
digest. Step 4 performs no installation, target access, endpoint use, GPIO,
clock, DMA, transmitter, SDR, or RF operation and establishes no timing,
frequency, spectral, keyed-mode, WSPR, transmitter, receiver, or RF
qualification.
