# Issue 412: RP1-GPCLK-DKMS v1.1.0 consumer contract

WsprryPi's Raspberry Pi 5 GPIO backend consumes the canonical ABI v2 header
from `RP1-GPCLK-DKMS` 1.1.0 at frozen source commit
`90f2fb38bcb75592383c6736d8b4b923e2baeb9b`. The sole endpoint remains
`/dev/rp1-gpclk`; there is no historical-endpoint, legacy-DMA, `/dev/mem`, raw
MMIO, route, or physical-backend fallback.

The byte-authoritative consumer header is
`src/WSPR-Transmitter/src/rp1_gpclk_uapi.h`, SHA-256
`998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384`.
The consumer issues `QUERY_V2`, requires ABI range support for version 2, and
validates structure identity, known capabilities, route, compatibility,
drive strengths, TONE limits, reserved fields, and complete module, build, and
compatibility identities. An old module's `EOPNOTSUPP` is a deterministic
rejection; ABI v1 is never reinterpreted as TONE support.

Ordinary operator TONE maps to `TONE_CONTINUOUS`, has `duration_ns == 0`, and
continues until an explicit generation-specific stop or ownership cleanup. An
explicitly duration-bounded request maps to `TONE_FINITE`; the exact duration
is carried into the kernel request and must fall within the provider's frozen
1,000,000 through 120,000,000,000 ns limits. The Slice 6 request is exactly
1,000,000,000 ns. A userspace watchdog is secondary cleanup and is not the
finite operation's primary bound.

WSPR remains `SUBMIT_WSPR`. QRSS, FSKCW, and DFCW remain finite
`SUBMIT_EVENTS` operations. GPIO4 route 1 and GPIO20 route 2 retain independent
compatibility evidence. Live acquisition requires the selected route's exact
positive compatibility identity and `LIVE_ELIGIBLE`; saving or discovering a
route does not establish live eligibility.

The 1.1.0 freeze deliberately transfers no prior GPIO4 evidence, and GPIO20
remains unavailable. Consumer compilation, QUERY negotiation, mocks, and
read-only target inspection are not carrier, timing, transmission, or RF
qualification. Product/release work must consume an explicitly compatible
tagged DKMS release rather than this moving development branch.
