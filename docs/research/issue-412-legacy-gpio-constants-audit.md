# Legacy GPIO Constants Audit

Short name: **Legacy GPIO Constants Audit**

## Record identity

- Audit type: report-only source and history review
- Audited branch: `codex/issue-412-external-rp1-provider`
- Audited commit: `f7b20f8d100ead23afa99d789da713681e9e76c3`
- Audit date: 2026-08-29
- Historical review added: 2026-08-29, `Legacy_1.2.3`
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
5. **Medium:** the authoritative historical BCM2835/RPi1 `-2.5 PPM` profile
   correction survives only as an ineffective constructor initializer. The
   current runtime overwrites it and therefore no longer honors the original
   RPi1-only frequency-generation model.

A future device-specific calibration facility should not be implemented as
another unqualified generic `ppm` value. Final-composition bounds and the
identity/provenance model should be resolved first.

## Source-to-RF dataflow

### Legacy 500 MHz PLLD profiles

```text
Pi model/generation and CPU revision
  -> select BCM2835 or BCM2836/BCM2837 hardware profile
  -> select nominal 500 MHz PLLD
  -> apply intrinsic profile correction: BCM2835=-2.5 PPM; later profiles=0
  -> select qualified/stale provider estimate or manual alternative
  -> add explicitly configured conducted residual
  -> validate the final signed correction
  -> freeze the complete correction model into the execution request
  -> corrected source = 500 MHz * (1 + effective_ppm * 10^-6)
  -> 12.12 GPCLK lower/upper divisors
  -> PWM-derived pacing and divider dithering
  -> one committed plan used for the bounded execution
  -> current correction provenance and effective PPM published in status
```

Primary evidence: `src/system_clock_frequency_estimate.cpp:119-173`,
`src/scheduling.cpp:157-166`, `src/scheduling.cpp:1129-1148`, and
`src/WSPR-Transmitter/src/wspr_transmit_backend_rpi.cpp:463-482,2924-2995`.

The `Legacy_1.2.3` implementation documents the missing profile distinction in
`src/wspr.cpp:310-339`. It describes a measured 2.5 PPM difference on RPi1
between the converged NTP correction and the crystal's RF-relevant frequency
offset, says the difference was absent on RPi2, RPi3, and RPi4, and applies
`500000000.0 * (1 - 2.500e-6)` only to RPi1. RPi2 and RPi3 retain an exact
500 MHz model and RPi4 uses 750 MHz. The adjustment predates the retained 1.0
through 1.2 release history; raw measurement artifacts are not retained.

For this work, that legacy behavior is accepted as the authoritative
historically accurate BCM2835/RPi1 frequency-generation model. It is not a
universal 500 MHz correction and is not contingent on whether NTP, manual, or
other additional correction is selected. Modern empirical revalidation is not
possible without a BCM2835 device and is not required to preserve the model.

### BCM2711 750 MHz PLLD profile

The flow is identical except BCM2711 selects nominal 750 MHz PLLD. If its
12.12 divider cannot represent the requested range, planning selects the
compiled 54 MHz oscillator and applies the correction model to that selected
source. PLLD and oscillator are distinct parent paths and both require
hardware validation, including frequencies immediately around every
source-selection boundary.

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
| C04 | `500000000 * (1 - 2.5e-6)` | Current constructor `wspr_transmit_backend_rpi.cpp:605-611`; authoritative origin `Legacy_1.2.3:src/wspr.cpp:310-339` | BCM2835/RPi1 only | Authoritative historical profile correction stranded in ineffective initialization | Preserve as explicit BCM2835 intrinsic profile data; remove only the misleading constructor encoding |
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

### M2: parent rates are model constants that still need runtime identity

The legacy backend does not query an authoritative clock provider for active
parent identity and rate. It assumes 500 MHz for BCM2835/36/37, 750 MHz PLLD
for BCM2711, and a BCM2711-only 54 MHz oscillator fallback. These values have
historical and qualification support but remain board-class assumptions.

Keep these rates as explicit frequency-generation profile data. Where
available, validate active source identity and rate through provider/readback
evidence. Additional NTP, manual, or conducted corrections refine the selected
profile model; they do not define or replace its constants.

### M3: current status can differ from the active committed correction

Correction is frozen into each request, but status publishes the mutable
`current_gpio_correction` (`src/scheduling.cpp:5124-5135`). A provider refresh
during transmission can therefore make status show a value other than the one
in the active plan.

Publish current-candidate and active-committed correction separately, including
profile, parent, nominal rate, mode, source signature, and snapshot time.

### M4: authoritative BCM2835 correction is no longer active

The current constructor's `-2.5 PPM` adjustment is overwritten during
successful processor detection. The legacy source establishes that the value
was an intentional RPi1-only frequency-model correction, while RPi2/RPi3 used
an unadjusted 500 MHz model. The current runtime therefore neither applies the
historical correction to RPi1 nor exposes its provenance.

Restore the correction as named BCM2835 profile data, always included when
that exact profile is selected. BCM2836/BCM2837 must not inherit it; their own
intrinsic system-to-RF difference begins at zero for discovery and is promoted
only through the accepted measurement procedure. Remove the constructor
expression only after the BCM2835 value has been relocated without changing
its historical meaning.

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
- proof that BCM2835 receives exactly `-2.5 PPM` while BCM2836, BCM2837, and
  BCM2711 never inherit that value and receive only their own parent-specific
  promoted `D`;
- proof that intrinsic profile correction remains present with provider,
  manual, and uncalibrated-zero additional correction modes;
- source-selection boundary and generated-frequency tests for both BCM2711
  750 MHz PLLD and 54 MHz oscillator paths;
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

## Accepted frequency-generation model

The implementation path shall use an intrinsic system-to-RF difference for
each processor/parent model. The historical BCM2835 value is fixed; constants
for available hardware are derived by the conducted procedure below:

| Processor profile | Parent | Nominal rate | Intrinsic system-to-RF difference | Evidence status |
|---|---|---:|---:|---|
| BCM2835 / RPi1 | PLLD | 500 MHz | `-2.5 PPM` | Authoritative historical legacy model; no current device available |
| BCM2836 / BCM2837 class, including Zero 2 W | PLLD | 500 MHz | To be derived; zero is the discovery baseline | Legacy excludes the BCM2835 constant; Zero 2 W available as the accepted representative |
| BCM2711 / RPi4 | PLLD | 750 MHz | To be derived; zero is the discovery baseline | RPi4 available as the accepted representative |
| BCM2711 / RPi4 | oscillator | 54 MHz | To be derived independently; zero is the discovery baseline | Distinct RPi4 parent path available for validation |

Intrinsic profile correction is part of frequency generation regardless of
whether the operator selects a provider estimate, a manual alternative, or no
additional correction. Conceptually:

```text
effective_ppm =
    intrinsic_profile_ppm
  + selected_additional_correction_ppm

selected_additional_correction_ppm =
    usable_provider_ppm + configured_conducted_residual_ppm
  or configured_manual_ppm
  or 0
```

This preserves current provider/manual precedence while making the intrinsic
profile model unconditional. The implementation must not apply provider and
manual values together. It must validate the final sum, freeze it for the
bounded execution, and publish every component separately.

### Deriving a transportable intrinsic difference

The purpose of the empirical campaign is not to promote one board's absolute
uncorrected clock error into a chipset constant. The target quantity is the
stable difference between the RF parent-clock error and the simultaneous local
system-clock estimate:

```text
S = frozen qualified NTP system-clock correction in PPM
P = RF parent-clock error inferred from receiver-corrected carrier measurement
D = intrinsic system-to-RF difference for the processor/parent model

D = P - S
```

All three quantities use the current source-rate convention: positive means
the relevant physical clock runs fast and would place an uncorrected carrier
high; negative means it runs slow. Calculations may retain exact rate ratios,
but reported and composed values use PPM and must declare rounding precision.

### Campaign-only SDR calibration

All discovery, perturbation, and closure analysis shall use this supplied SDR
calibration:

```text
detected_frequency_hz - reference_frequency_hz =
    0.4484 Hz + reference_frequency_hz * 1.01012 PPM
```

The exact inverse applied to every raw detected carrier is:

```text
calibrated_sdr_hz =
    (raw_detected_sdr_hz - 0.4484 Hz)
    / (1 + 1.01012 * 10^-6)

calibrated_rf_error_hz = calibrated_sdr_hz - requested_rf_hz

calibrated_rf_error_ppm =
    calibrated_rf_error_hz / requested_rf_hz * 10^6
```

The inverse, rather than an approximate subtraction, is authoritative for
these campaigns. The intercept and proportional term are applied exactly once
before `P`, `D`, discovery error, closure error, or acceptance is calculated.
Both the raw detected frequency and calibrated result must be retained.

When the receiver observes a harmonic, calibration occurs at the raw detected
harmonic frequency before translation to the transmitter fundamental:

```text
calibrated_harmonic_hz =
    (raw_detected_harmonic_hz - 0.4484 Hz)
    / (1 + 1.01012 * 10^-6)

calibrated_fundamental_hz = calibrated_harmonic_hz / harmonic_number
```

Dividing before removing the affine intercept would scale the `0.4484 Hz`
term incorrectly. The harmonic number and requested fundamental must be
authenticated campaign inputs, not inferred from the nearest apparent tone.

This calibration corrects the measurement instrument only. It is required
campaign methodology and evidence metadata, but it must never become a
WsprryPi runtime setting, chipset/parent constant, intrinsic `D`, provider
residual, manual correction, configuration default, or production frequency
calculation. It applies only to the SDR and receiver configuration for which
the user declares it authoritative. Another receiver or changed configuration
requires separately supplied calibration evidence.

The preferred discovery run applies `S` while the intrinsic and configured
residuals are zero. After receiver correction is applied exactly once, the
remaining signed carrier error directly estimates `D`. An equivalent analysis
may measure `P` without correction and subtract the simultaneous frozen `S`,
but the preferred run exercises the real provider-composition path.

Every discovery requires a closure run. The second bounded transmission
applies `S + D` and must move the receiver-corrected carrier to the requested
frequency within the declared tolerance. Positive and negative perturbations
must independently confirm the sign and prove that neither `S` nor `D` is
applied twice.

The representative-board assumption is explicit: the derived `D` is accepted
as authoritative for that processor/parent model even though only one board is
measured. It must not cross parent, processor, GPIO route, receiver/reference,
or backend boundaries. BCM2711 PLLD and oscillator therefore produce separate
constants from separate discovery and closure runs.

The campaign must retain the exact NTP snapshot and qualification, source
signature, selected parent and nominal rate, executable and source revision,
requested fundamental and authenticated harmonic relationship, GPIO route,
SDR identity and configuration, the exact calibration equation, raw detected
frequency, calibrated SDR frequency, receiver correction application count,
reference identity, temperature/time context, raw captures,
repeated-frequency results, and final calculation. Provider changes during a
run do not alter the frozen execution correction.

## Phased resolution path

1. **Freeze the model and measurement contract.** Define distinct BCM2835,
   BCM2836/BCM2837, and BCM2711 profiles; define BCM2711 PLLD and oscillator
   parents; define `D = P - S`; specify discovery, closure, sign-perturbation,
   repetition, SDR inverse-calibration, harmonic-ordering, one-time receiver
   correction, and evidence rules; and independently bind selection and
   composition in hardware-free tests. Measurement-analysis fixtures must use
   known synthetic raw frequencies to prove the exact inverse and demonstrate
   that calibration-before-harmonic-division is enforced.
2. **Harden correction selection.** Validate provider metadata and the final
   composed value; remove inconsistent clamping; preserve provider/manual
   precedence; and prove one-time application and execution-level freezing.
3. **Unify profile and parent selection.** Resolve processor identity once,
   fail closed on unknown or contradictory identity, select the parent before
   correction, and remove the stranded constructor encoding only after the
   BCM2835 rule is active in profile data.
4. **Expose provenance.** Report current candidate and active committed values,
   including processor profile, parent, nominal rate, intrinsic correction,
   provider/manual component, conducted residual, final correction, and
   snapshot identity.
5. **Run NTP-relative discovery and closure on available legacy hardware.** On
   the Zero 2 W, prove the BCM2835 residual is excluded, freeze qualified `S`,
   derive the 500 MHz profile's `D`, and verify `S + D` by closure. On the RPi4,
   independently derive and close `D` for both 750 MHz PLLD and 54 MHz
   oscillator generation, covering all parent-selection regions, transition
   boundaries, positive/negative perturbations, bounded keyed modes, and
   cleanup. Every result and acceptance decision uses the calibrated SDR value,
   while retaining the raw detection. Hardware activity requires separate
   explicit authorization and a predeclared conducted plan.
6. **Promote and close the evidence record.** Promote a constant only after its
   calibrated discovery and calibrated closure evidence passes. Retain BCM2835
   as historically authoritative but not modern-hardware-revalidated. Record
   Zero 2 W and RPi4 constants separately without transferring them to another
   processor, parent, route, RP1, or Si5351. Retain the SDR calibration as
   evidence methodology only; do not promote it or any component of it into
   WsprryPi production state.

## Unknowns and next gate

Source review cannot establish actual firmware parent rates, active parent
selection, device-to-device source error, conducted RF accuracy, phase noise,
spurs, timing, or stability across temperature, boot, parent, route, and
provider versions.

The BCM2835 `-2.5 PPM` value cannot be modern-hardware-revalidated with the
available Zero 2 W or RPi4; it is retained as the authoritative historical
model. The Zero 2 W can validate the later 500 MHz exclusion path. The RPi4 can
validate both maintained clock-parent paths, including selection boundaries.

The next gate is approval of the phased hardware-free implementation plan.
Physical validation remains separately authorized work requiring exact host,
GPIO route, frequencies, parent-selection expectations, calibrated
receiver/reference, bounded modes and durations, stopping procedure, and
verified cleanup.

## Adversarial reassessment

The second pass challenged source, unit, sign, bound, fallback, profile,
snapshot lifetime, duplicate constants, and backend separation. It found H1,
H2, M1, M3, and L2. It found no current evidence of double application,
residual replacement, mid-frame mutation of a committed plan, Si5351 leakage,
production use of 19.2 MHz, or a repository default containing the provisional
RP1 measurement.
