# Issue 430 reconciliation into devel

Follow-up operator requirement: intrinsic compensation and composed RF totals
are internal only. The UI, status payloads, and correction logs show additional
correction, never the intrinsic offset. The original merge description below
records its earlier reporting design and is superseded on that point.

This integration merges `codex/issue-430` (`923a2ac`) into the shared-chipset
offset implementation on `devel` (`b2b6a40`). It restores the unmerged Issue 429
clock-model, correction-validation and provenance work together with Issue 430
Mailbox revision parsing. It does not restore superseded calibration defaults.

## Preserved contracts

- `src/Chipset-Offsets` remains the sole intrinsic offset selector: Pi1 -2.5 ppm,
  BCM2836/BCM2837 and BCM2711 zero, RP1 -46.245 ppm.
- Requests carry additional correction. Physical GPIO backends add intrinsic
  correction once; scheduler provenance reports the components and total.
- Caller correction keeps its full +/-200 ppm range. Intrinsic RF correction
  does not change the PWM/DMA timebase.
- RP1 retains its 200 MHz planner contract, the recorded GPIO20 sweep evidence,
  and the untested status of 6 m/2 m. This merge is not RF qualification.
- Mailbox revision failures, including zero and malformed input, fail closed
  without caching a fabricated BCM2835 identity.

## Review repairs

Reconciliation removed the incoming scheduler/backend double-application path,
obsolete expected offsets, and missing standalone link dependencies. Provider
validation now rejects an old first observation rather than treating it as a
previously qualified stale value; fallback age includes the original sample age.
The UI retains quality, age, and snapshot diagnostics, rejects malformed numeric
provenance instead of displaying a fabricated zero, and labels unused residuals.

The restored tests use explicit RP1 route-confirmation mocks. Review also found
an older direct-tone test lacking scheduler execution suppression. Its attempted
Mailbox open was denied; the fixture now suppresses backend configuration.

## Validation boundary

Validation uses hardware-independent component tests and an isolated checkout
on wspr5 (`/home/pi/issue430-merge-test.pIG6Kt`), not the installed executable
or canonical checkout. The checkout was moved from `/tmp` after that filesystem
ran out of compiler scratch space. UI review uses
fixture data in isolated headless Chrome, desktop/light and mobile/dark, with
long identifiers and no panel overflow. No live transmission, installation,
service change, or GPSDO control is part of this merge.

Validation commands, run from the parent repository unless noted:

All commands below passed on the reconciled tree. The full semantics suite
includes 23 UI publication tests; its cleanup fault-injection diagnostics are
expected test output, not live hardware failures.

```sh
make -C src chipset-offsets-test mailbox-memory-flag-test \
  legacy-gpio-clock-model-test gpio-frequency-correction-test rp1-gpclk-planner-test
make -C src semantics-test
make -C src rp1-gpclk-transmit-backend-test rp1-gpclk-lifecycle-test \
  rp1-gpclk-transition-test rp1-gpclk-backend-test
make -C src/WSPR-Transmitter/src startup-quiesce-test
make -C src/Mailbox/src test
node WsprryPi-UI/tests/gpio_correction_provenance_test.js
node --check WsprryPi-UI/data/site.js
php -l WsprryPi-UI/data/views/operation.php
git diff --check
```

Mailbox `test` is compile-only; startup-quiesce uses fake adapters. UI review
used Impeccable's audit and hardening guidance. Its detector reported four
pre-existing findings outside the changed panel (one typography clamp and three
colors); these were not expanded into unrelated redesign work.

The WSPR-Transmitter and Mailbox READMEs are updated. The old RP1 zero-baseline
proposal is explicitly historical. Operator documentation in the separate
Wsprry_Pi_Docs repository still needs the candidate/committed panel and
additional-versus-total PPM explanation; that repository was not modified.
