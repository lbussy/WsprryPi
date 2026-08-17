# Issue 411 Slice 11: strict Si5351 install-path result

## Result

Passed on 2026-08-17. WsprryPi now provides an explicit binary-only Make install
path for any selected release profile. The canonical Ubuntu x86 candidate is:

```sh
cd src
make release BACKENDS=si5351 ANCILLARY_GPIO=0 SUDO=
make install-binary BACKENDS=si5351 ANCILLARY_GPIO=0 \
  PREFIX=/usr/local/bin
```

The new `install-binary` target builds the selected release profile, creates
only the requested prefix, and installs that exact `wsprrypi` executable with
mode `0755`. It performs no package, configuration, web, systemd, service,
device, or runtime operation.

The existing default all-backend build, Make `install`, `debuginstall`, and
`uninstall` targets, and `scripts/install.sh` Raspberry Pi product installer
were not changed.

## Regression coverage

`scripts/tests/strict_i2c_install_path_test.sh` installs into a new disposable
prefix and proves:

- the installed file is executable and byte-identical to the selected build;
- its mode is exactly `0755`;
- it reports compiled backends `si5351` and ancillary GPIO disabled;
- it has no libgpiod runtime dependency;
- the prefix contains no other installed file; and
- a deliberately failing `systemctl` sentinel is never invoked.

The strict Ubuntu 24.04 CI job now runs this regression after the established
strict profile and file-access audits.

## Ubuntu 24.04 x86_64 validation

An ephemeral Ubuntu 24.04 `linux/amd64` container confirmed GCC major version
13 and intentionally omitted libgpiod development packages. The following
passed:

- strict release build under the project's warning-as-error policy;
- backend-profile factory test;
- strict-profile non-root capability and ancillary-GPIO rejection test; and
- the new disposable-prefix install-path regression.

The first isolated attempt omitted `.git`, causing required build-metadata
generation to stop before compilation completed. The fixture was corrected by
including repository metadata, its partial temporary build was discarded, and
the unchanged validation then passed. This was a validation-fixture failure,
not a product finding.

## Native Debian validation

An isolated native Debian environment with hardware access disabled passed:

- backend-capability generator regression;
- backend-capability Make integration regression;
- strict release build and backend-profile factory test;
- strict-profile capability and libgpiod audit;
- strict I2C file-access audit as `nobody`, observing only the deliberately
  nonexistent `/dev/i2c-2147483646` path;
- the new disposable-prefix install-path regression; and
- `make -j2 semantics-test SUDO=`, covering runtime semantics, cleanup
  lifecycle, UI/source regression, GPIO band fail-closed behavior, log
  timestamp display, and update comparison.

The first native invocation mistakenly called the unprivileged file-access
audit as root. The audit correctly refused to run. It was immediately rerun as
`nobody` and passed before remaining validation continued.

No validation installed onto the Mac or Raspberry Pi, operated a production
service, opened a real device, or generated RF.

## Documentation impact

The in-repository compile-time backend-selection research record now contains
the exact candidate build/install commands and clearly distinguishes the
binary-only target from the Raspberry Pi product installer.

The separate `Wsprry_Pi_Docs` repository was reviewed but not modified because
cross-repository documentation changes were not authorized. Its installation
guide remains deliberately Raspberry Pi-focused. If the Ubuntu x86 path is
promoted beyond the Issue 411 tester workflow, a separately authorized
development/experimental installation page will be required there.

## Remaining boundary

This result qualifies the build and binary-copy installation contract only. It
does not qualify CP2112 driver support, USB adapter behavior, `/dev/i2c-N`
permissions on the tester's host, I2C electrical behavior, Si5351 frequency or
RF output, or a persistent service arrangement.

The candidate must still be reviewed and integrated into `devel` before tester
instructions are published. Issue 411 remains open and otherwise unchanged.
