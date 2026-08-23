# Issue 412: RP1-GPCLK-DKMS v1.0.1 consumer contract

WsprryPi's Raspberry Pi 5 GPIO backend consumes the separately released
`RP1-GPCLK-DKMS` provider. The development integration is bound to frozen
upstream commit `d05b26c6843ad8cd31536067b64e27c8ae195490`; product and release
work must consume an explicitly compatible tagged module release instead of a
moving branch or development checkout.

The canonical endpoint is `/dev/rp1-gpclk`. WsprryPi does not fall back to the
historical `/dev/rp1-gpclk0` spelling, legacy DMA, `/dev/mem`, raw MMIO,
another RP1 route, Si5351, or simulation when the selected RP1 provider is
missing or rejected. A Raspberry Pi 5 may use Si5351 only when the operator
selects that exclusive I2C backend explicitly.

The canonical ABI is version 1. Its byte-authoritative header is
`src/WSPR-Transmitter/src/rp1_gpclk_uapi.h`, copied from the upstream
`include/uapi/linux/rp1_gpclk.h`, with SHA-256
`1d411644352e61402bd4685a5692070d543ab2ee5b016d394294aa98970bd7fb`.
Changing the endpoint, header bytes, existing ioctl identities, structure
layouts, route identities, or semantics reopens the upstream freeze and
invalidates this consumer integration.

At the provider boundary, WsprryPi keeps these facts distinct:

- implemented capabilities describe operations the provider implements;
- compatibility state and reason describe the exact module, kernel, resource,
  and route assessment;
- `LIVE_ELIGIBLE` is an independent capability controlled by the immutable
  live-output gate and exact positive compatibility policy; and
- GPIO4 route 1 and GPIO20 route 2 have independent compatibility evidence.

Endpoint enumeration and `QUERY` are read-only administration. An
output-disabled `ACQUIRE`, `GET_STATE`, `RELEASE`, or owner close establishes
only bounded ownership and cleanup behavior. None of these operations is a
clock, GPIO, timing, carrier, transmission, or RF qualification. Submission
remains unavailable unless both the load-time live-output gate and an exact
positive compatibility entry permit the selected route.

The consumer fails closed for missing or inaccessible endpoints, ABI or limit
mismatches, unknown flags/enums/capabilities/states, missing required
capabilities, route mismatches, incompatible compatibility results, stale
lease or generation identities, and cleanup failures. One open endpoint owns
at most one provider lease, and generations remain bound to that lease.

The bounded `rp1-gpclk-admin-probe` developer target performs only `QUERY`, an
administrative `ACQUIRE` when the provider permits it, `GET_STATE`, `RELEASE`,
and endpoint closure. It never submits WSPR or events. Building the probe does
not authorize running it; target use requires separate output-disabled
administration authorization.

The frozen 1.0.1 metadata has no positive GPIO4 or GPIO20 compatibility entry.
Accordingly, a current provider may validly report
`Compatible-unqualified` without `LIVE_ELIGIBLE`, and WsprryPi must not weaken
that result or present it as live qualification.
