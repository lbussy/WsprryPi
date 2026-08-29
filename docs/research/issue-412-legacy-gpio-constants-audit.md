# Legacy GPIO Constants Audit

Short name: **Legacy GPIO Constants Audit**

## Record identity

- Audit type: report-only source and history review
- Audited branch: `codex/issue-412-external-rp1-provider`
- Audited commit: `f7b20f8d100ead23afa99d789da713681e9e76c3`
- Audit date: 2026-08-29
- Retained on: `devel`
- Hardware, host, network, GPIO, service, transmission, and RF activity: none

This record audits the non-RP1 Raspberry Pi GPIO transmitter and its shared
calibration flow. Line references describe the audited commit. Later source
changes may move the cited lines and must be revalidated before implementation.

The provisional wspr5 GPIO20/GPCLK0 RP1 XOSC result of approximately
`-41.203682 PPM` is not a production default, a legacy GPIO constant, or an
approved setting. It remains device-, route-, parent-, receiver-, and
measurement-specific evidence outside this repository.

## Outcome

The legacy GPIO correction path has a coherent signed-PPM execution formula
and generally strong snapshot testing, but it retains implicit hardware
policy, duplicated constants, inconsistent boundary handling, and two
actionable validation defects.

No blocker was found in the current sign convention or frame-freezing
behavior. The highest-risk results are:

1. **High:** provider estimate plus residual is not revalidated after
   composition. Two individually valid inputs can exceed the backend's
   `+/-200 PPM` contract and fail only during backend configuration.
2. **High:** provider skew is checked as `skew > 1.0`, not by magnitude. The
   qualifier silently assumes skew cannot be negative.
3. **Medium:** the application and transmitter independently infer hardware
   profile using different evidence and fallback rules.
4. **Medium:** nominal 500 MHz, 750 MHz, and 54 MHz rates are compiled-in
   profile policy rather than authoritative observations of the active clock.
5. **Low:** an inherited `500 MHz * (1 - 2.5 PPM)` constructor default remains
   as misleading latent state even though successful processor detection
   overwrites it.

A future device-specific calibration facility should not be implemented as
another unqualified generic `ppm` value. Final-composition bounds and the
identity/provenance model should be resolved first.

## Source-to-RF dataflow

### Legacy 500 MHz PLLD profile

```text
Pi model/generation and CPU revision
  -> scheduler labels LEGACY_500_MHZ_PLLD
  -> backend maps BCM2835/2836/2837 to nominal 500 MHz PLLD
  -> provider snapshot qualification
  -> qualified/stale estimate + residual, else manual, else uncalibrated zero
  -> effective signed PPM frozen into TransmissionRequest.calibration.ppm
  -> corrected source = 500 MHz * (1 + ppm * 10^-6)
  -> 12.12 GPCLK lower/upper divisors
  -> PWM-derived pacing and divider dithering
  -> one committed plan used for the bounded execution
  -> current correction provenance and effective PPM published in status
```

Primary evidence: `src/system_clock_frequency_estimate.cpp:119-173`,
`src/scheduling.cpp:157-166`, `src/scheduling.cpp:1129-1148`, and
`src/WSPR-Transmitter/src/wspr_transmit_backend_rpi.cpp:463-482,2924-2995`.

### BCM2711 750 MHz PLLD profile

The flow is identical except BCM2711 selects nominal 750 MHz PLLD. If its
12.12 divider cannot represent the requested range, planning tries a compiled
54 MHz oscillator and applies the same signed PPM to that selected source.

Primary evidence:
`src/WSPR-Transmitter/src/wspr_transmit_backend_rpi.cpp:526-558,2166-2182`.
The maintained 54 MHz fallback is documented and tested for 2200 m. The
separate 19.2 MHz experiment is rejected research, not production behavior
(`docs/research/issue-390-transmitter-qualification.md:32-44`).

### Backend boundary

Provider qualification, residual/manual selection, request freezing, and the
generic `CalibrationSnapshot.ppm` transport are shared by legacy GPIO and RP1.
The source-clock identity is not shared: legacy applies the scalar to its
500/750/54 MHz model, while RP1 applies it to its own selected-parent planner.
Si5351 disables system-clock estimation and uses its separate calibration
field (`src/config_handler.cpp:1222-1237`). Sharing the scalar transport is not
evidence that calibrations can transfer between backends.

## Inventory

| ID | Value or expression | Audited location | Scope | Classification | Risk and disposition |
|---|---|---|---|---|---|
| C01 | `500e6` | `wspr_transmit_backend_rpi.cpp:526-529,2155-2163` | Pi 1-3 | Hardware-profile data | Centralize and bind to authoritative identity |
| C02 | `750e6` | `wspr_transmit_backend_rpi.cpp:526-529,2166-2169` | BCM2711 | Hardware-profile data | Centralize and bind to authoritative identity |
| C03 | `54e6` | `wspr_transmit_backend_rpi.cpp:541-557` | BCM2711 low-frequency fallback | Hardware-profile data | Name the parent and provenance explicitly |
| C04 | `500000000 * (1 - 2.5e-6)` | `wspr_transmit_backend_rpi.cpp:605-611` | Constructor | Obsolete latent behavior | Remove separately; initialize neutral or invalid |
| C05 | `nominal * (1 + ppm * 1e-6)` | `wspr_transmit_backend_rpi.cpp:463-482` | Legacy GPIO | Appropriate invariant | Retain positive-fast contract |
| C06 | `+/-200 PPM` | `system_clock_frequency_estimate.cpp:8-11`; backend `:465-476` | Shared GPIO | Duplicated policy | Centralize and validate the composed result |
| C07 | `estimate + residual` | `system_clock_frequency_estimate.cpp:133-153` | Legacy and RP1 flow | Correct formula, incomplete validation | Reject final values outside bounds |
| C08 | 3 samples; skew 1.0; residual 0.5; spread 0.1 PPM | `system_clock_frequency_estimate.hpp:61-70` | Provider qualification | Named policy | Document empirical basis; harden sign/range checks |
| C09 | current 300 s; stale 900 s | `system_clock_frequency_estimate.hpp:69-70` | Provider fallback | Named policy | Expose freshness and active snapshot clearly |
| C10 | 24-bit mask, scale 4096, MASH minimum 5 | `wspr_transmit_backend_rpi.cpp:487-511` | Legacy GPCLK | Hardware invariant | Retain and cite peripheral specification |
| C11 | floor lower word; optional `+1` LSB | `wspr_transmit_backend_rpi.cpp:564-581` | Legacy GPCLK | Synthesis invariant | Retain |
| C12 | `PWM_CLOCKS_PER_ITER_NOMINAL=1000` | `wspr_transmit_backend_rpi.hpp:518` | Legacy pacing | Evidence-bound design policy | Keep explicit; do not treat as calibration |
| C13 | DMA ring 1024, lead 64, poll 50 us | `wspr_transmit_backend_rpi.cpp:2394-2404,2539-2544` | Legacy lifecycle | Timing policy | Retain as named constants |
| C14 | generation 4 -> 750; otherwise 500 | `scheduling.cpp:1260-1272` | Application metadata | Implicit fallback | Replace with typed fail-closed resolution |
| C15 | update delta `0.01 PPM` | `PPM-Manager/src/ppm_manager.cpp:394-409` | Provider manager | Partly vestigial policy | Clarify or remove |
| C16 | UI `-200..200`, step `0.000001` | `WsprryPi-UI/data/views/config.php:833-845` | UI | Mirrored constraint | Also validate the composition |
| C17 | provider on; residual/manual zero | `config/wsprrypi.ini:108-122` | Stock configuration | Appropriate defaults | Retain |
| C18 | estimate, residual, effective PPM status | `scheduling.cpp:4969-4980,5124-5135` | Observability | Incomplete active provenance | Add committed value, profile, and parent identity |

## Actionable findings

### H1: composed correction can exceed the execution contract

Provider estimate and residual are each accepted within `+/-200 PPM`, then
added without a final bound check
(`src/system_clock_frequency_estimate.cpp:133-153`). For example, `199 + 2`
passes both input validators but is rejected later by the legacy backend
(`src/WSPR-Transmitter/src/wspr_transmit_backend_rpi.cpp:465-476`). RP1 has the
same planner boundary.

The common selection layer should reject non-finite or out-of-range composed
values before committing a request. It should not clamp. Future tests must
cover `199+2`, `-199-2`, exact boundaries, and legacy/RP1 parity.

### H2: skew validation relies on an unstated sign invariant

The qualifier rejects only `skew > 1.0`
(`src/system_clock_frequency_estimate.cpp:57-63`), while residual frequency is
properly checked by magnitude at lines 65-71. Chrony normally reports
nonnegative skew, but malformed or future provider data can bypass this gate.

Provider normalization should reject negative skew explicitly or the common
qualifier should compare magnitude. Tests should also cover negative age and
malformed retained-source counts.

### M1: hardware profile is resolved twice

The scheduler maps generation 4 to the BCM2711 profile and every other legacy
case to the 500 MHz profile (`src/scheduling.cpp:1260-1272`). The backend
independently decodes CPU revision and throws on unknown processors
(`src/WSPR-Transmitter/src/wspr_transmit_backend_rpi.cpp:2118-2175`). Platform
admission reduces normal exposure but does not provide a single authoritative
identity.

Resolve one typed profile, require agreement at the backend boundary, and fail
closed on unknown or contradictory hardware.

### M2: parent rates are assumptions, not observed state

The legacy backend does not query an authoritative clock provider for active
parent identity and rate. It assumes 500 MHz for BCM2835/36/37, 750 MHz PLLD
for BCM2711, and a BCM2711-only 54 MHz oscillator fallback. These values have
historical and qualification support but remain board-class assumptions.

Keep expected rates as explicit profile data and, where available, validate
the active source and rate through provider/readback evidence.

### M3: current status can differ from the active committed correction

Correction is frozen into each request, but status publishes the mutable
`current_gpio_correction` (`src/scheduling.cpp:5124-5135`). A provider refresh
during transmission can therefore make status show a value other than the one
in the active plan.

Publish current-candidate and active-committed correction separately, including
profile, parent, nominal rate, mode, source signature, and snapshot time.

### L1: inherited constructor calibration

The constructor's `-2.5 PPM` adjustment is overwritten during successful
processor detection, but encodes an obsolete sign convention and an
unidentified device calibration. Remove it in separately authorized cleanup.

### L2: inconsistent CLI boundary behavior

Residual PPM rejects out-of-range input
(`src/arg_parser.cpp:3470-3485`), while legacy `-p` manual PPM clamps and logs
(`src/arg_parser.cpp:4076-4095`). Stored configuration rejects. Prefer
consistent fail-closed rejection.

## Test assessment

Independent coverage exists for the positive-fast formula, physical RF
direction, exact PPM boundaries, non-finite rejection, BCM2711 54 MHz fallback,
provider-plus-residual addition, manual/stale fallback, and request-level PPM
freezing. Representative evidence is
`src/WSPR-Transmitter/src/startup_quiesce_test.cpp:654-725` and
`src/tests/dial_frequency_semantics_test.cpp:7981-8036`.

Material gaps remain:

- composed results beyond `+/-200 PPM`;
- negative skew and age;
- scheduler/backend profile disagreement;
- active status versus committed correction;
- proof that the constructor's `-2.5 PPM` is unobservable;
- authoritative active-parent identity/rate;
- explicit non-transfer tests among legacy GPIO, RP1, and Si5351 provenance.

## Future RP1 device-calibration constraints

A later WsprryPi-owned setting must bind device, board, GPIO route, GPCLK
instance, provider, exact parent identity, and nominal/observed rate. It must
preserve the positive-fast sign convention; record measurement method,
timestamp, receiver configuration, and reference identity; and state whether
receiver correction has already been applied.

It must remain separate from Chrony residual and Si5351 calibration, define
one-time precedence/composition rules, validate the final result, fail closed
on identity mismatch, freeze one value per frame or keyed program, and publish
both configured and active provenance. No calibration may transfer between
GPIO4 and GPIO20, between different parent selections, or between legacy and
RP1 without evidence.

## Unknowns and next gate

Source review cannot establish actual firmware parent rates, active parent
selection, device-to-device source error, conducted RF accuracy, phase noise,
spurs, timing, or stability across temperature, boot, parent, route, and
provider versions.

The next hardware-free step is typed identity, final-composition validation,
and active-plan observability design. Physical validation remains a separate
authorization requiring route-specific conducted measurements with a
calibrated receiver/reference, parent-rate readback, bounded execution, and
verified cleanup.

## Adversarial reassessment

The second pass challenged source, unit, sign, bound, fallback, profile,
snapshot lifetime, duplicate constants, and backend separation. It found H1,
H2, M1, M3, and L2. It found no current evidence of double application,
residual replacement, mid-frame mutation of a committed plan, Si5351 leakage,
production use of 19.2 MHz, or a repository default containing the provisional
RP1 measurement.
