# Issue 412: RP1 GPCLK DKMS consumer integration

> Historical implementation record. The current product behavior and
> completion authority is
> [Issue 412: RP1 GPCLK DKMS product contract](issue-412-rp1-gpclk-product-contract.md).
> Where this record describes route activation as an external follow-up or
> restricts Pi 5 application validation to GPIO4, the product contract
> supersedes it.

Status: implemented and hardware-free validated for the WsprryPi consumer.
Target installation, overlay activation, module lifecycle, GPIO output, timing,
and RF remain unqualified and require separate authorization.

## Immutable dependency

WsprryPi consumes only the public RP1-GPCLK-DKMS annotated tag v1.0.0, which
peels to reviewed release-decision commit
d8c45a33e9a8b16cf5ea9a89736347347bc14817.

The optional product is rp1-gpclk-dkms_1.0.0-1_all.deb, SHA-256
951289ee5d0e44cff41b59756f00161aba16f43f1450715ba57c4a3679a2e6b8.
Its canonical userspace UAPI is installed at
/usr/src/rp1-gpclk-dkms-1.0.0/include/uapi/linux/rp1_gpclk.h, SHA-256
1d411644352e61402bd4685a5692070d543ab2ee5b016d394294aa98970bd7fb.
The package owns both inactive route overlays under
/usr/lib/rp1-gpclk-dkms/overlays: rp1-gpclk-gpio4.dtbo and
rp1-gpclk-gpio20.dtbo.

The separately published qualification archive is not a WsprryPi build,
runtime, installer, packaging, or operator dependency.

A byte-identical copy of the canonical UAPI is retained under
src/WSPR-Transmitter/external/rp1-gpclk-dkms-v1.0.0 so generic Debian and macOS
builds do not require an installed DKMS package. The dependency regression
checks its exact SHA-256. The header is dual-licensed upstream as GPL-2.0-only
with Linux-syscall-note or MIT; WsprryPi uses it under MIT.

## Optional installation

Ordinary installation does nothing with RP1-GPCLK-DKMS. An administrator must
set INSTALL_RP1_GPCLK_DKMS=true and RP1_GPCLK_ROUTE=GPIO4 (or GPIO20) when
invoking scripts/install.sh. Before downloading, the helper confirms a
Raspberry Pi 5/CM5 BCM2712 identity and a literal route. It then uses the fixed
HTTPS release URL, a fresh temporary directory, exact package checksum,
Debian-member checks, and exact UAPI identity before conventional APT
installation.

The package installs both overlays and leaves both inactive. The installer does
not edit config.txt, load the module, apply an overlay, or silently substitute a
route. A route change therefore does not reinstall the package.

WsprryPi uninstallation preserves the independently owned package by default.
An explicit REMOVE_RP1_GPCLK_DKMS=true with ACTION=uninstall removes only the
recognized 1.0.0-1 Debian package through APT.

## Runtime fail-closed boundary

The existing gpio operator choice maps to RP1 only on a detected Pi 5; Pi 1-4
continue to map to the legacy backend, and other systems reject GPIO. Selection
remains explicit and never falls back after RP1 failure.

Before acquisition, the userspace provider queries the canonical v1 UAPI and
requires module identity rp1-gpclk-dkms, build identity 1.0.0, ABI v1, exactly
GPIO4 or GPIO20 as the active route, non-rejected compatibility, and all
execution, state, route, compatibility, cleanup, and live-eligibility
capabilities. Acquisition binds the exact queried route. Missing endpoints,
permissions, wrong versions/UAPI, unavailable routes, incompatible identities,
and non-live-eligible module state fail before submission. There is no fallback
to MMIO, /dev/mem, mailbox, Si5351, simulation, or the other RP1 route.

## Reconciliation and next gate

Implemented: immutable dependency pinning, optional Pi-5-only installer
selection, exact product/UAPI inventory checks, released UAPI integration,
route-bound provider acquisition, fail-closed compatibility checks, and
hardware-free GPIO4/GPIO20/provider lifecycle regressions.

Not qualified: installation on a target, DKMS compilation for a target kernel,
boot-overlay selection, module load/unload, endpoint permissions, actual query
identity, cancellation/cleanup against the released module, GPIO4/GPIO20
electrical behavior, timing, modes, transmission, or RF.

The next step is a separately authorized, digest-bound target-validation plan
for one named Pi 5, one route, one stock kernel/header identity, a stop
procedure, output-disabled lifecycle first, and explicit confirmation before
any boot, module, GPIO, or RF action. Issue #412 itself has not been modified or
closed.
