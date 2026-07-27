# CW Timing Presets

Status: Proposed  
Implementation state: Not implemented  
Repositories affected: `WsprryPi` and `WsprryPi-UI`

## Purpose

Add clear QRSS1, QRSS3, and QRSS6 timing presets to the WsprryPi web interface while preserving support for custom CW timing.

The design must:

- Default new CW configurations to QRSS3.
- Make standard timing easy to select and understand.
- Preserve existing custom timing configurations.
- Keep advanced controls available without presenting every operator with editable low-level timing fields.
- Preserve the existing backend configuration format.
- Apply consistently to QRSS, FSKCW, and DFCW where they share CW timing.
- Group related timing controls together in the UI.

## Current Behavior

The CW Control panel currently presents these settings as independent editable fields:

- CW modulation mode:
  - QRSS
  - FSKCW
  - DFCW
- Dot Seconds
- Frequency Offset
- Base Frequency
- Frequency calibration
- Start minute
- Repeat interval
- Intra-Element Gap
- Inter-Character Gap
- Inter-Word Gap

The backend already uses:

- `CW.Dot Seconds` as the shared dot duration for QRSS, FSKCW, and DFCW.
- `CW.Intra Element Gap` as a dot-length multiplier.
- `CW.Inter Character Gap` as a dot-length multiplier.
- `CW.Inter Word Gap` as a dot-length multiplier.

Actual gap durations are calculated from:

```text
gap duration = dot duration × gap multiplier
```

The current default timing is:

```text
Dot Seconds:          3
Intra Element Gap:    1
Inter Character Gap:  3
Inter Word Gap:       7
```

This corresponds to QRSS3 with standard Morse spacing.

## Proposed Operator Model

### Modulation Mode

Continue to provide the existing modulation choices:

- QRSS
- FSKCW
- DFCW

These choices control how the signal is generated. They are separate from the speed preset.

### Speed Preset

Add a Speed control with four choices:

- QRSS1
- QRSS3
- QRSS6
- Advanced

The choices map to dot duration as follows:

| Speed | Dot duration |
|---|---:|
| QRSS1 | 1 second |
| QRSS3 | 3 seconds |
| QRSS6 | 6 seconds |
| Advanced | Operator-defined positive duration |

QRSS3 is the default when a new or incomplete CW configuration requires a default.

### Dot Duration

Keep the Dot Duration field visible for all speed selections.

For QRSS1, QRSS3, and QRSS6:

- Populate the corresponding duration.
- Disable manual editing.
- Display the selected value clearly.
- Explain that the duration comes from the selected speed preset.

For Advanced:

- Enable manual editing.
- Preserve the current valid duration when switching into Advanced.
- Require a finite value greater than zero.
- Continue using the backend’s existing validation limits and semantics.

Selecting Advanced must not blank or arbitrarily reset the current duration.

### Spacing Mode

Add a separate Spacing control with two choices:

- Standard
- Advanced

This control is independent of the Speed preset. An operator may therefore use:

- QRSS1 with standard spacing
- QRSS3 with advanced spacing
- QRSS6 with standard spacing
- An advanced dot duration with either standard or advanced spacing

### Standard Spacing

Standard spacing uses these multipliers:

| Gap | Multiplier |
|---|---:|
| Intra-element | 1× dot |
| Inter-character | 3× dot |
| Inter-word | 7× dot |

When Standard spacing is selected:

- Populate the gap fields with `1`, `3`, and `7`.
- Disable manual editing of the three multiplier fields.
- Continue displaying the fields so the operator can see the effective rules.
- Display the calculated duration for each gap.

Examples:

| Speed | Intra-element | Inter-character | Inter-word |
|---|---:|---:|---:|
| QRSS1 | 1 second | 3 seconds | 7 seconds |
| QRSS3 | 3 seconds | 9 seconds | 21 seconds |
| QRSS6 | 6 seconds | 18 seconds | 42 seconds |

### Advanced Spacing

When Advanced spacing is selected:

- Enable the three existing gap multiplier fields.
- Preserve their current valid values.
- Require every multiplier to be finite and greater than zero.
- Display the calculated duration beside or beneath each field.
- Recalculate the displayed duration whenever the dot duration or a multiplier changes.

The persisted values remain multipliers, not seconds.

## Proposed UI Organization

Place all related timing controls together near the top of the CW Control panel.

Recommended organization:

```text
CW Control

  Modulation
  Mode: [ QRSS ] [ FSKCW ] [ DFCW ]

  CW Timing
  Speed: [ QRSS1 ] [ QRSS3 ] [ QRSS6 ] [ Advanced ]

  Dot duration:
  [ 3 seconds — disabled ]
  Determined by the QRSS3 preset.

  Spacing:
  [ Standard ] [ Advanced ]

  Intra-element:
  [ 1 — disabled ] × dot = 3 seconds

  Inter-character:
  [ 3 — disabled ] × dot = 9 seconds

  Inter-word:
  [ 7 — disabled ] × dot = 21 seconds

  Frequency and Schedule
  Base frequency | Frequency offset
  Frequency calibration
  Start minute | Repeat interval
```

The exact responsive arrangement may vary, but the semantic grouping must remain clear.

On narrow screens, controls should stack in this order:

1. Modulation mode
2. Speed preset
3. Dot duration
4. Spacing mode
5. Gap controls
6. Frequency controls
7. Scheduling controls

## Configuration Compatibility

### Existing Keys

Continue using the existing configuration keys:

```json
{
  "CW": {
    "Dot Seconds": 3.0,
    "Intra Element Gap": 1.0,
    "Inter Character Gap": 3.0,
    "Inter Word Gap": 7.0
  }
}
```

Do not require new backend configuration keys for the initial implementation.

In particular, do not initially persist:

- `Speed Preset`
- `QRSS Mode`
- `Advanced Dot Timing`
- `Spacing Mode`
- `Advanced Spacing`

These are UI interpretations of the effective timing values.

### Inferring the Speed Selection

When loading configuration:

| Persisted Dot Seconds | Selected speed |
|---:|---|
| Exactly `1` | QRSS1 |
| Exactly `3` | QRSS3 |
| Exactly `6` | QRSS6 |
| Any other valid positive value | Advanced |

Use numeric comparison appropriate for parsed configuration values. Do not use formatted display strings to infer the selection.

### Inferring the Spacing Selection

When loading configuration:

| Persisted multipliers | Selected spacing |
|---|---|
| Exactly `1`, `3`, and `7` | Standard |
| Any other valid positive combination | Advanced |

A custom configuration must remain custom after loading and saving.

### Defaults

When no valid CW timing is available, use:

```text
Speed:               QRSS3
Dot Seconds:         3
Spacing:             Standard
Intra Element Gap:   1
Inter Character Gap: 3
Inter Word Gap:      7
```

An existing valid configuration always takes precedence over these defaults.

## State-Transition Rules

### Selecting QRSS1, QRSS3, or QRSS6

When an operator selects a preset:

1. Set Dot Seconds to the preset value.
2. Disable manual editing of Dot Duration.
3. Recalculate all displayed gap durations.
4. Validate the effective configuration.
5. Use the existing autosave workflow.

### Selecting Advanced Speed

When an operator selects Advanced:

1. Retain the current valid Dot Seconds value.
2. Enable the Dot Duration field.
3. Focus behavior should not unexpectedly select, erase, or replace the value.
4. Revalidate changes through the existing live-validation workflow.

### Selecting Standard Spacing

When an operator selects Standard:

1. Set the multipliers to `1`, `3`, and `7`.
2. Disable manual editing of the multiplier fields.
3. Recalculate displayed gap durations.
4. Validate the effective configuration.
5. Use the existing autosave workflow.

Switching from Advanced spacing to Standard is an intentional configuration change. It replaces custom multipliers with `1`, `3`, and `7`.

### Selecting Advanced Spacing

When an operator selects Advanced spacing:

1. Retain the current valid multipliers.
2. Enable the three multiplier fields.
3. Continue showing calculated durations.
4. Revalidate changes through the existing live-validation workflow.

### Loading Existing Configuration

During configuration population:

1. Load the four persisted timing values.
2. Infer Speed from Dot Seconds.
3. Infer Spacing from the three multipliers.
4. Set enabled and disabled states without causing an unintended autosave.
5. Calculate the displayed effective durations.
6. Synchronize the autosave baseline only after population is complete.

Configuration loading must not overwrite custom timing merely because a preset selector now exists.

### Disabled Fields and Serialization

Disabled timing fields still represent active configuration values.

Before collecting the configuration payload:

- Read the effective timing state rather than relying on browser form submission semantics.
- Ensure Dot Seconds and all three gap multipliers remain present.
- Serialize the same four existing `CW` keys.
- Never treat a disabled control as an absent or optional setting.

## Validation

### Dot Duration

Dot Duration must be:

- numeric
- finite
- greater than zero

When a preset is selected, validation applies to the effective preset value even though the field is disabled.

### Gap Multipliers

Every multiplier must be:

- numeric
- finite
- greater than zero

When Standard spacing is selected, validation applies to the effective `1`, `3`, and `7` values.

### Repeat Interval

Existing message-duration and repeat-interval validation remains applicable.

The implementation must continue detecting configurations in which the calculated CW message duration exceeds the configured repeat interval.

This is especially important for:

- QRSS6
- long messages
- large advanced gap multipliers
- custom dot durations

## Accessibility and Interaction Requirements

- Use a fieldset and legend or an equivalent accessible grouping for Speed.
- Use a separate accessible group for Spacing.
- Do not communicate disabled or advanced state through color alone.
- Associate all explanatory text and calculated-duration output with the applicable control.
- Keep keyboard operation complete and predictable.
- Ensure disabled values remain readable in both light and dark themes.
- Announce calculated-duration changes appropriately without creating excessive screen-reader chatter.
- Use operator-facing names and avoid exposing internal variable or JSON key names as primary labels.
- Preserve visible validation feedback for advanced editable fields.
- Explain how to enable editing when a field is disabled.

Suggested operator-facing labels:

```text
Speed
Dot duration
Spacing
Intra-element gap
Inter-character gap
Inter-word gap
```

Suggested supporting text:

```text
QRSS3 uses a three-second dot duration.

Standard spacing uses 1×, 3×, and 7× the selected dot duration.

Select Advanced to enter a custom dot duration.

Select Advanced spacing to edit the gap multipliers.
```

## Impeccable Requirement

This is UI work and must use the Impeccable skill as required by the repository’s `AGENTS.md`.

Before implementation:

1. Confirm that the Impeccable skill is installed and available.
2. Read and follow its instructions.
3. Inspect the existing WsprryPi design language and responsive behavior.

During and after implementation:

1. Use Impeccable to review the proposed grouping and hierarchy.
2. Render the actual Setup page.
3. Exercise QRSS1, QRSS3, QRSS6, Advanced speed, Standard spacing, and Advanced spacing.
4. Inspect desktop and mobile layouts.
5. Inspect both supported themes.
6. Review disabled, enabled, invalid, saving, and saved states.
7. Address applicable findings.
8. Report any finding intentionally not adopted and why.

If Impeccable is unavailable, stop before changing UI files.

Do not commit `.agents/`, `.impeccable/`, or other local skill/runtime state unless explicitly requested.

## Repository Boundaries

### `WsprryPi-UI`

Expected UI work belongs in the root `WsprryPi-UI` submodule, including:

- timing control markup
- labels and help text
- responsive layout
- selection and enablement logic
- calculated-duration presentation
- form population
- autosave interaction
- UI validation
- UI-specific tests

Inspect the submodule at the exact commit recorded by the parent repository before making changes.

### `WsprryPi`

Expected parent-repository work may include:

- UI/source regression tests
- integration-contract tests
- configuration compatibility tests
- message-duration and repeat-policy regression coverage
- documentation or planning references
- a reviewed update to the `WsprryPi-UI` submodule pointer

The backend configuration schema should not require modification for the initial implementation.

### `src/` Submodules

No changes to dependency submodules under `src/` are expected.

If implementation appears to require such a change, stop and report why before modifying a dependency.

## Expected Implementation Seams

Confirm current paths before implementation.

Likely UI seams:

```text
WsprryPi-UI/data/views/config.php
WsprryPi-UI/data/index.js
WsprryPi-UI/data/site.js
WsprryPi-UI/data/index.css
WsprryPi-UI/data/site.css
```

Likely parent-repository seams:

```text
src/config_handler.hpp
src/config_handler.cpp
src/arg_parser.cpp
src/scheduling.cpp
src/tests/ui_source_regression_test.cpp
src/tests/dial_frequency_semantics_test.cpp
src/tests/non_wspr_repeat_policy_test.cpp
```

Do not modify every listed file automatically. Inspect the current contracts and change only what the approved implementation requires.

## Required Test Coverage

### UI State Tests

Verify:

- Missing timing defaults to QRSS3.
- QRSS1 sets Dot Seconds to `1`.
- QRSS3 sets Dot Seconds to `3`.
- QRSS6 sets Dot Seconds to `6`.
- Preset dot fields are disabled.
- Advanced enables Dot Duration.
- Entering Advanced preserves the current valid value.
- A custom dot duration loads as Advanced.
- Standard spacing sets `1`, `3`, and `7`.
- Standard spacing disables the multiplier fields.
- Advanced spacing enables the multiplier fields.
- Custom multipliers load as Advanced spacing.
- Switching from Advanced spacing to Standard intentionally restores `1`, `3`, and `7`.
- Switching into Advanced spacing retains the current values.

### Calculated-Duration Tests

Verify:

| Dot duration | Multipliers | Expected durations |
|---:|---|---|
| 1 | `1/3/7` | `1/3/7` seconds |
| 3 | `1/3/7` | `3/9/21` seconds |
| 6 | `1/3/7` | `6/18/42` seconds |
| 2.5 | `1/3/7` | `2.5/7.5/17.5` seconds |
| 3 | `1.5/4/8` | `4.5/12/24` seconds |

Presentation formatting must not change the persisted numeric values.

### Persistence Tests

Verify that configuration collection continues emitting:

```text
CW.Dot Seconds
CW.Intra Element Gap
CW.Inter Character Gap
CW.Inter Word Gap
```

Verify:

- Preset values save correctly.
- Disabled fields are still serialized.
- Advanced values survive save and reload.
- Existing custom configurations round-trip without loss.
- Loading configuration does not trigger an unintended normalization autosave.
- No new backend keys are required.

### Backend Regression Tests

Verify:

- Dot duration remains shared across QRSS, FSKCW, and DFCW.
- Standard and advanced multipliers produce the expected runtime durations.
- All four values remain subject to backend validation.
- Message-duration calculations continue including inter-element, inter-character, and inter-word spacing.
- Repeat-policy rejection still works for messages longer than the repeat interval.
- Existing configurations without UI metadata remain valid.

### Visual and Workflow Tests

Using Impeccable and the rendered application, verify:

- desktop layout
- mobile layout
- light theme
- dark theme
- QRSS1, QRSS3, and QRSS6 selections
- Advanced dot editing
- Standard and Advanced spacing
- disabled-field legibility
- validation errors
- calculated-duration updates
- autosave feedback
- reload and restoration of custom values

Automated tests do not replace this workflow exercise.

## Acceptance Criteria

The feature is acceptable when:

- CW Modes defaults to QRSS3 when no valid existing timing is available.
- Operators can select QRSS1, QRSS3, or QRSS6 directly.
- Presets reliably produce one-, three-, or six-second dots.
- Dot Duration is editable only in Advanced speed.
- Standard spacing reliably produces `1/3/7` multipliers.
- Gap multipliers are editable only in Advanced spacing.
- The UI displays calculated gap durations.
- Timing controls are grouped coherently.
- Existing custom configurations remain custom and round-trip without loss.
- No backend configuration migration is required.
- QRSS, FSKCW, and DFCW continue sharing the established timing contract.
- Relevant UI, persistence, backend, and repeat-policy tests pass.
- Impeccable review and real rendered-page inspection are complete.
- Documentation impact has been reviewed.
- No `src/` dependency submodule changes are required.
- Parent and UI submodule changes remain separately reviewable.
- Hardware or live-transmission validation is not claimed unless separately performed and authorized.

## Non-Goals

This initial feature does not:

- change Morse dash duration from three dots
- introduce independently persisted preset names
- change the backend configuration schema
- create different dot durations for QRSS, FSKCW, and DFCW
- alter RF frequency behavior
- alter frequency offset semantics
- alter scheduling semantics
- redesign the entire Setup page
- modify transmitter hardware behavior
- modify dependency submodules under `src/`
- authorize live RF, GPIO, service, installation, or hardware testing
- commit or push either repository

## Documentation Impact

Implementation should review the operator documentation covering:

- CW mode selection
- QRSS timing
- FSKCW and DFCW timing
- CW gap multipliers
- repeat intervals
- the Signal Setup web interface
- screenshots depicting the CW Control panel

Documentation must distinguish:

- speed presets
- custom dot duration
- standard spacing
- advanced spacing
- multiplier values
- calculated durations in seconds

Screenshots should be replaced only when the changed UI makes the existing image materially inaccurate. Age alone is not a reason to replace a contextually correct screenshot.

## Implementation Gate

This document is a proposed implementation contract, not authorization to modify code.

Before implementation:

1. Inspect the current parent repository and recursive submodule state.
2. Confirm the exact `WsprryPi-UI` revision.
3. Confirm that Impeccable is available.
4. Compare current implementation behavior with this document.
5. Report any material drift or ambiguity.
6. Obtain explicit approval to implement.
