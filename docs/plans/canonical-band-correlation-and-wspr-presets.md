# Canonical Band Correlation and WSPR Preset Contract

Status: provisional reference contract for issue #332. Issue #401 must finish,
and the implementation and external frequency references must be reviewed again,
before this becomes an implementation contract.

## Purpose

WsprryPi needs one application-wide band correlation model for WSPR, Test Tone,
QRSS, FSKCW, and DFCW. The correlated band selects configured Band GPIO/filter
behavior and associates a request with backend/profile/mode qualification.

Qualification is a project capability record, not a determination of an
operator's legal authority. A backend that is not qualified for a correlated
band and mode may be blocked. Issue #401 owns qualification states, experimental
overrides, immutable policy propagation, and non-bypassable representability and
hardware-safety checks.

Canonical WSPR names and dial frequencies are a separate WSPR-only convenience
layer. They must not control the meaning of a band for CW modes.

## Ownership and boundaries

The parent WsprryPi application should own the single frequency-to-band
correlation catalog because it coordinates every mode and Band GPIO/filter
selection. `WSPR-Transmitter` should consume the immutable correlated band and
mode when applying its qualification records. It should not retain a second
frequency-edge table after migration.

The transmitter remains authoritative for final numeric, backend,
representability, lifecycle, cancellation, output-inhibition, and cleanup
checks. Frequencies outside the correlation catalog must not infer a nearest
band or filter; issue #401's experimental controls and explicit selector rules
apply.

Qualification records are keyed by correlated band, backend, hardware profile,
and mode. Band-level qualification may be based on representative-frequency
evidence without claiming that the entire correlation envelope was swept. The
retained evidence must identify the representative plan and state that coverage
is a band-level project classification.

## Correlation catalog

The ordinary amateur correlation scope is 2200 m through 70 cm. Experimental,
trial, ISM, Part 15, special-research, and individually granted extension
frequencies are excluded. The 22 m entry is not part of the canonical amateur
catalog.

| Canonical band | Correlation envelope |
| --- | ---: |
| `2200m` | 130-190 kHz |
| `630m` | 472-479 kHz |
| `160m` | 1.8-2.0 MHz |
| `80m` | 3.5-4.0 MHz |
| `60m` | 5.25-5.45 MHz |
| `40m` | 7.0-7.3 MHz |
| `30m` | 10.1-10.15 MHz |
| `20m` | 14.0-14.35 MHz |
| `17m` | 18.068-18.168 MHz |
| `15m` | 21.0-21.45 MHz |
| `12m` | 24.89-24.99 MHz |
| `10m` | 28.0-29.7 MHz |
| `6m` | 50-54 MHz |
| `4m` | 69.9-70.5 MHz |
| `2m` | 144-148 MHz |
| `1.25m` | 219-225 MHz |
| `70cm` | 420-450 MHz |

These are correlation envelopes. They can contain jurisdiction-specific gaps
and do not assert operating authority.

## Canonical band identity

Canonical persisted and API names are lowercase unit-bearing identifiers from
the table above. Matching may be case-insensitive. `lf` and `mf` may remain
accepted compatibility aliases for `2200m` and `630m`, but output should use the
canonical name. Display formatting such as `1.25 m` is presentation, not
identity.

## WSPR preset identity

A band has one correlation identity but may have multiple WSPR frequency
presets. A common convention can use the bare band name, while a convention that
needs disambiguation uses a qualified WSPR-only preset identifier, for example:

```text
60m:legacy
60m:wrc15
60m:us
60m:uk
```

All of these correlate to the same `60m` band. Qualified preset names do not
create new bands and do not apply to QRSS, FSKCW, or DFCW.

Existing installations and configurations must retain the current meaning of a
bare `60m` unless the operator selects another WSPR frequency profile or local
preference. The current convention should also receive an explicit stable name,
such as `60m:legacy`, during migration.

Do not add regional variants speculatively. Each built-in preset must have a
maintained source and a demonstrated operator-convenience need.

## WSPR frequency profiles and local preferences

The parent application should provide an explicitly selected WSPR frequency
profile. Initial compatibility behavior is `Existing/Common`. Maintained
country or locality profiles may choose a different default preset for a band.
IARU regions alone are insufficient where national conventions differ.

The application must not infer locality from an IP address or callsign. A
profile is an operator-selected convenience default, not a regulatory claim.

Local per-band preferences belong in the normal parent WsprryPi configuration
lifecycle, alongside the selected WSPR profile. They do not belong in
`WSPR-Transmitter`. Start with configuration-backed preferences rather than a
separate database. A future schema might conceptually express:

```ini
[WSPR]
Frequency Profile = Existing/Common

[WSPR Band Preferences]
60m = 60m:us
20m = 20m
```

User-defined local presets may later supply a band, integral USB dial frequency,
and label. The exact INI/JSON/UI schema is unresolved until the post-#401
configuration-lifecycle review.

WSPR frequency resolution precedence is:

1. explicit numeric frequency;
2. explicit qualified preset such as `60m:us`;
3. local per-band preference;
4. selected WSPR frequency profile;
5. built-in Existing/Common preset.

Changing profiles must not rewrite explicit numeric entries, explicit qualified
presets, or saved local preferences.

## Compatibility and migration

- Preserve current bare aliases and dial frequencies by default.
- Add explicit names for legacy conventions before changing profile behavior.
- Remove `22m` from the canonical amateur and WSPR catalogs with an explicit
  compatibility diagnostic; do not silently remap it.
- Keep a one-effective-preset-per-band response for compatibility while adding
  a separate complete preset catalog capable of returning multiple presets per
  band.
- Include both canonical band and WSPR preset identity in new control-plane
  responses and diagnostics.

## Required post-#401 reference pass

Before implementation:

1. inspect the final #401 parent/submodule request and enforcement boundaries;
2. inventory every band-correlation, selector, filter, parser, Test Tone, and
   qualification consumer;
3. re-verify the international correlation envelopes;
4. re-research current official and community WSPR dial conventions;
5. identify only bands that genuinely need multiple presets;
6. review defaults, parsing, validation, persistence, serialization, scheduling,
   UI, and documentation impacts;
7. define the exact `22m` compatibility behavior;
8. define independent parent and submodule tests and commit ordering.

No implementation work is authorized by this reference contract.
