# RP1 GPCLK Phase 9: operator visibility and documentation boundary

## Outcome

The Raspberry Pi 5 RP1 GPCLK provider remains available to explicit engineering
and qualification workflows, while the normal public capability response keeps
that transmitter path unavailable. The operator interface therefore does not
offer the RP1 drive controls and directs a normal Pi 5 installation toward an
explicit Si5351 selection. A retained engineering GPIO configuration is not
silently rewritten by display logic or an unrelated autosave.

While the gate is closed, the web API preserves the existing GPIO subsection
instead of accepting hidden GPIO edits. This keeps invalid browser drafts or
stale hidden pin values from blocking unrelated saves without normalizing or
rewriting the engineering configuration. CLI and INI engineering workflows
continue to use the ordinary runtime validation path.

The gate is intentionally authoritative and closed: `operator_exposes_rp1_gpio()`
returns false. It is separate from `platform_supports_gpio_clock_transmission()`,
which continues to describe whether the runtime can use the provider. Promotion
later requires changing the public gate deliberately and repeating the relevant
operator, installation, and qualification review.

This is a presentation and capability-discovery gate for development, not an
authorization boundary. Explicit CLI, INI, and configuration-API engineering
workflows remain available when the matching provider is installed; the normal
installer and operator interface do not advertise or deploy that path.

## Installation and documentation boundaries

The standard installer contains no RP1 GPCLK kernel, provider, overlay, or
`live_output` deployment path. A regression check protects that boundary.

The operator procedure is recorded in the separate documentation repository as
`docs/Experimental/rp1_gpio.md`. It is marked orphaned and excluded from search,
so it is available for engineering review without appearing in ordinary
navigation or search results.

## Validation boundary

This phase performs source, browser, build, and clock-disabled validation only.
It does not install artifacts, change GPIO state, start services, reboot a Pi,
or transmit RF. Prior Issue 399 evidence remains bounded to its recorded
hardware, kernel, provider, pin, mode, drive setting, and procedure; this phase
does not broaden those claims.

## Remaining promotion work

- Decide that the RP1 transmitter path is ready for ordinary operators and open
  the public gate deliberately.
- Define and validate the supported installation and recovery workflow.
- Complete the desired hardware and RF qualification matrix without treating
  pad-drive milliamps as calibrated RF output power.
- Link and edit the operator page for the supported product workflow.
- Repeat the final cross-repository review after promotion changes.

## Documentation Impact

- Updated: an intentionally unlinked and unsearchable experimental operator page
  in `Wsprry_Pi_Docs` covering prerequisites, limits, recovery, and qualification.
- Considered but unchanged: normal operator navigation, installation pages, and
  public Pi compatibility guidance, because RP1 GPIO remains gated off.
- Still required: supported installation and linked operator instructions when
  the capability is promoted.
