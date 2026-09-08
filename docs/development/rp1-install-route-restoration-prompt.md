# RP1 installation route restoration implementation prompt

Work directly on the clean, current `devel` branch of WsprryPi. Inspect parent
and component status and instructions, preserve unrelated work, and refresh
`origin/devel` before editing. Implement, validate, conduct adversarial review,
repair actionable findings and repeat review until none remain, then commit and
push only `devel` to `origin`.

## Problem and outcome

On wspr5, setup successfully installed the runtime-controller DKMS profile but
finished at `neutral_ready` with a null route while the saved application
configuration selected `rp1-gpclk` and GPIO4. The application consequently lacked
`/dev/rp1-gpclk`, reported unresolved startup reconciliation, and inhibited
transmission. Setup nevertheless reported service/API readiness. Restore the
explicitly configured RP1 route during installation so software success requires
matching route readiness with transmission disabled. GPIO20 must behave equally.

Also reproduce and repair the observed configuration-update rejection,
`Si5351.I2C Address must be an integer`, without rejecting existing valid
hexadecimal addresses or changing unrelated settings.

Investigate the reported exponent/subnormal value in Reference calibration
(PPM), including initialization, reset, parsing, persistence and presentation.
Repair any reproducible software defect and test exact zero and custom-value
round trips; do not silently erase user calibration or hide corruption by
formatting it as zero. Distinguish confirmed cause from unresolved live evidence.

## Implementation boundaries

- WsprryPi owns orchestration and configuration. Use the installed provider's
  public digest-reviewed `route-plan` / `route-ensure` transaction, exact binding
  and source identities, and final readiness inspection. Preserve the DKMS
  package's initial neutral installation and reusable component boundaries.
- An explicit persisted `rp1-gpclk` backend and valid GPIO4/GPIO20 selection is
  the route to restore for this installation workflow. Do not infer route intent
  from a GPIO default for another backend, overwrite configuration, or select a
  different route. Preserve first-install neutral behavior where RP1 is not
  explicitly configured.
- Coordinate installation ordering, startup reconciliation and ownership
  records. Bind restoration to the current installation and configuration;
  reject drift, ambiguous state, invalid route, active transmission, conflicting
  ownership, stale plans and incomplete evidence. A completed checkpoint from
  an older binding must not suppress required work after reinstall.
- Preserve idle/stopped/masked service policy. Do not bypass provider lifecycle
  controls, hand-edit boot overlays, invoke raw module/overlay operations, grant
  execution authority, or enable transmission as a side effect.
- Make repeated installation and interruption recovery bounded and attributable.
  Failure must keep output disabled, retain diagnostic evidence and propagate
  failure rather than claim transmitter readiness from service/API checks.
- Trace the Si5351 value through defaults, INI/JSON parsing, validation,
  serialization, form population and update/persistence. Fix the actual defect
  with regression coverage for valid hex/numeric values and invalid input.
- Use Impeccable for affected UI workflow review; preserve the current design,
  draft values, feedback placement and responsive behavior. Render and inspect
  desktop and mobile using hardware-free fixtures when UI behavior is affected.
- Keep changes in WsprryPi. Review sibling operator documentation read-only and
  report the exact required follow-up; cross-repository writes are not approved.
- This prompt authorizes repository implementation, safe software validation,
  commit and push. It does not authorize installation on wspr5, GPIO or service
  mutation, reboot, transmission, or RF testing.

## Acceptance and evidence

1. Cover configured GPIO4 and GPIO20, non-RP1 configurations, absent/invalid
   configuration, disabled DKMS selection, exact current binding, configuration
   drift, stale checkpoint, active output, stopped/masked application, provider
   refusals and inconsistent final evidence. Exercise retry/interruption paths.
2. Verify ordering and failure propagation through the actual installer entry
   points using the existing fake-provider/non-hardware test infrastructure.
3. Reproduce the Si5351 rejection before repair and verify valid address
   round trips and unrelated updates preserve the address and saved values.
4. Inspect targets before running them. Use existing focused Makefile targets
   from `src` and the portable simulated-only profile on macOS where needed.
   Run shell/Python/source/UI regressions relevant to changed behavior.
5. Review the complete diff adversarially for lifecycle, identity, concurrency,
   compatibility, failure reporting and test blind spots. Record each finding,
   correction and affected validation; perform a fresh final assessment.
6. Update repository contracts and retain a concise implementation/validation
   report. Review the staged diff, commit, push `origin/devel`, and verify remote
   parity and final working-tree status. Report exact commands/results,
   components changed, documentation impact and outstanding live qualification.
