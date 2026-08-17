# Issue 411 Slice 12: candidate publication result

## Result

Passed on 2026-08-17. The complete Issue 411 compile-time backend selection and
strict binary-install candidate was fast-forwarded into `devel`, validated by
GitHub Actions on the published commit, and documented for the original tester
in Issue 411. The issue remains open pending CP2112 hardware availability.

No pull request, history rewrite, installation, service action, device access,
hardware operation, test tone, transmission, or RF occurred.

## Published candidate

The validated product and CI correction tip is:

```text
3f99d1052c27207c6d629ddfa1dfeff401318cfd
```

It is a linear descendant of Slice 11 and was published by strict fast-forward
to `devel`. At the publication gate, local `devel` and `origin/devel` resolved
to that exact commit with a clean working tree.

## GitHub Actions evidence

The successful exact-SHA workflow run is:

- Workflow: `Debian non-hardware validation`
- Run: [32034607595](https://github.com/WsprryPi/WsprryPi/actions/runs/32034607595)
- Head SHA: `3f99d1052c27207c6d629ddfa1dfeff401318cfd`
- Conclusion: success

All three jobs passed:

- `strict-i2c-profile`, including the Ubuntu 24.04 strict build, factory test,
  non-root profile audit, file-access audit, and binary-only install regression;
- `ubuntu-gcc13-release`, including the GCC 13 toolchain check and all requested
  release backend profiles under warnings-as-errors; and
- `non-hardware-validation`, including parent semantics, UI unit/browser
  regressions, RP1 and transmitter contracts, WSPR Reference build/consumer,
  deterministic simulation, and the final hardware-access rejection audit.

## Corrected CI findings

Two failed publication attempts exposed container setup defects before the
strict build could execute:

1. Run
   [32034111357](https://github.com/WsprryPi/WsprryPi/actions/runs/32034111357)
   failed checkout because the minimal Ubuntu container lacked CA certificates.
   `ca-certificates` was added to that job's dependency list.
2. Run
   [32034366075](https://github.com/WsprryPi/WsprryPi/actions/runs/32034366075)
   then checked out successfully but Git metadata generation rejected the
   container-mounted workspace as dubious ownership. The strict job now records
   `GITHUB_WORKSPACE` as a safe directory, matching the established broader
   container job.

Neither failure was accepted or bypassed. Each correction was limited to the
strict CI environment, committed on Slice 12, fast-forwarded into `devel`, and
retested through the complete workflow.

## Issue 411 handoff

The verified tester instructions were posted at:

- [Issue 411 candidate instructions](https://github.com/WsprryPi/WsprryPi/issues/411#issuecomment-5316700623)

The comment identifies the exact integrated commit and successful Actions run,
provides Ubuntu dependencies, clean clone and preservation-safe update paths,
builds `BACKENDS=si5351 ANCILLARY_GPIO=0`, installs with `install-binary`, and
checks `--list-backends` and `--version`.

It also states that `install-binary` installs only the executable and does not
start a service, deploy the UI, change configuration, or alter device
permissions. It explicitly withholds live commands until the tester has the
adapter and the bus, address, driver, permissions, wiring, RF path, duration,
and stopping procedure are confirmed.

Issue 411 was verified open before and after the comment. It was not closed,
labeled, assigned, edited, or otherwise changed.

## Remaining work

Everything currently possible without the original tester's adapter is
complete. Hardware validation remains separately gated and must establish:

- CP2112 kernel-driver and adapter enumeration;
- the actual `/dev/i2c-N` path and non-root permissions;
- Si5351 address, wiring, voltage, pull-ups, and supported transactions;
- startup quiescence and bounded cleanup on that adapter;
- frequency and spectral behavior through an appropriate RF path; and
- an exact duration and stopping procedure before any live output.

The separate operator documentation remains Raspberry Pi-focused. Promotion of
this experimental Ubuntu path beyond the Issue 411 tester handoff requires a
separately authorized documentation change after hardware results are known.
