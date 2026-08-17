# Issue 411 Slice 9: integration closeout result

## Result

Passed on 2026-08-17. The complete Issue 411 implementation chain was reviewed
as a linear descendant of `origin/devel` and passed the required non-hardware
validation. The validated tip is suitable for fast-forward integration into
`devel`.

This result qualifies the compile-time backend-selection contracts, strict
Si5351-only Linux build, non-root privilege policy, capability reporting,
fail-closed selection, and isolated file-access behavior. It does not qualify
physical I2C electrical behavior, Si5351 output, GPIO timing, installation,
services, frequency accuracy, or RF.

## Reviewed implementation chain

The branch contained these eight implementation and evidence commits before
the Slice 9 closeout record:

1. `471bafe` — fortified shutdown-wake write handling;
2. `9307563` — backend capability foundation;
3. `b0c4777` — compile-time backend-profile activation;
4. `604aef2` — compiled-capability reporting;
5. `df79ce7` — strict Si5351 I2C build profile;
6. `fb62a9f` — GPIO-free non-root privilege policy;
7. `a18099c` — strict file-access audit; and
8. `b7e3267` — bounded `wspr5` Si5351 startup-quiescence evidence.

The complete diff was attributable to Issue 411. It added no UI, installer,
service-management, or unrelated product behavior.

## Cross-platform validation

### Ubuntu 24.04 x86_64, GCC 13

An isolated Ubuntu 24.04 container was run as `linux/amd64` on the Mac host.
GCC major version 13 was confirmed. Without libgpiod development packages, the
following strict profile passed under `-Werror`:

```sh
make -j1 release backend-profile-factory-test \
  BACKENDS=si5351 ANCILLARY_GPIO=0 SUDO=
```

The strict executable's non-root profile regression also passed.

The syscall audit could not produce evidence in this emulated x86 container:
guest `strace` emitted no trace even for an independent minimal Python
`os.open()` control. This is an emulation/tooling limitation, not an accepted
audit failure. The audit implementation was not weakened or bypassed; its
required evidence was obtained in the native Debian environment below.

### Native Debian isolated environment

With no hardware device nodes passed into the container, all of these checks
passed:

- strict `BACKENDS=si5351 ANCILLARY_GPIO=0` release build;
- backend-profile factory test;
- strict-profile non-root execution test;
- strict I2C file-access audit as an unprivileged user, proving that the only
  attempted device path was the deliberately nonexistent
  `/dev/i2c-2147483646`;
- backend-capability generator regression;
- backend-capability Make integration regression; and
- `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1 make -j2 semantics-test SUDO=`, including
  runtime semantics, cleanup lifecycle, UI/source regression, GPIO band
  fail-closed behavior, log timestamps, and update comparison.

No real device was accessed and no RF was generated.

## wspr5 preservation check

The final read-only check found `/home/pi/WsprryPi` clean on `devel` at the
pre-integration commit `00f093c8523d2068740d0371526d2340d8d99379`. The
`wsprrypi.service` unit was inactive, and no process held an I2C, gpiochip, or
RP1 device handle. No file, service, configuration, device, or hardware state
was changed.

The Pi checkout intentionally remains at the pre-integration commit. Updating
it to the newly integrated `devel` is a separate repository-resynchronization
step and does not imply installation or runtime qualification.

## Issue and publication boundary

Issue 411 remains open and otherwise unchanged. Publication consists only of
committing this prompt/result record, pushing the Slice 9 branch, and
fast-forwarding `devel` to the same validated commit. No pull request, issue
mutation, history rewrite, installation, service action, or transmission is
part of this closeout.
