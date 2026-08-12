# Phase 8: operator-selectable RP1 GPIO drive strength

Phase 8 adds a Raspberry Pi 5-specific RP1 pad-drive setting to the existing
GPIO transmitter configuration path. Operators may select 2, 4, 8, or 12 mA;
2 mA remains the safe default. These values are RP1 electrical drive settings,
not calibrated RF power readings.

## Implemented contract

- The canonical JSON default now includes `RP1 Drive mA: 2`.
- JSON and managed INI persistence retain valid selections without converting
  them to the legacy Raspberry Pi 1-4 GPIO power scale.
- Validation accepts exactly 2, 4, 8, and 12 mA and rejects other values.
- Raspberry Pi 5 runtime resolution passes the selected value unchanged to the
  RP1 GPCLK provider; earlier Raspberry Pi generations continue to use the
  existing 0-7 GPIO power setting.
- The Setup > Transmitter UI shows the RP1 selector only when GPIO is selected
  on Raspberry Pi 5. It preserves the inactive value when another backend is
  selected and shows invalid retained values next to the control without
  silently replacing them.

## Validation

The core semantic and UI source-regression targets passed on `wspr5`. The
focused browser integration passed its Pi 5/Pi 4 visibility, all-value
serialization, backend-switch preservation, and invalid-value
assertions. The RP1 Linux provider test exercised all four values and verified
that each reached the provider ioctl unchanged.

The real application then completed one bounded QRSS `E` operation for each
value through the production Raspberry Pi 5 GPIO runtime path. The installed
provider remained `live_output=N` throughout. Every run returned zero and
logged the requested 2, 4, 8, or 12 mA selection. Final state was GPIO4 input,
GPCLK0 prepare/enable counts zero, `live_output=N`, and `wsprrypi.service`
active. This is clock-disabled runtime qualification, not RF-output or
calibrated-power qualification.

The UI was rendered and inspected at desktop and mobile widths in light and
dark themes. The selector follows the existing Setup design language, presents
2 mA as the safe default, and explicitly says that the setting is not a
calibrated RF power measurement. Impeccable's final detector reported four
pre-existing color advisories in unchanged `site.js` visualization code; the
Phase 8 additions introduced no detector finding.

## Known validation limitations

The focused Phase 8 browser assertions completed successfully, but Chromium
occasionally left its temporary profile nonempty during test teardown after
screenshots were already written. A separate pre-existing CW browser test also
did not finish within its bounded window in the isolated worktree. Neither
failure involved the RP1 selector assertions, but both are recorded rather
than hidden.

## Documentation impact

The separate `Wsprry_Pi_Docs` operator repository still describes Pi 5 GPIO RF
as unsupported and documents only the legacy 0-7 control. Its compatibility,
installation, command-line, INI, REST, and Setup > Transmitter pages must be
updated in a separate authorized documentation slice after Issue 399's
remaining qualification supports the intended product claims. No files in
that repository were changed during Phase 8.
