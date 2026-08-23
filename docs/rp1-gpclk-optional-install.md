<!-- SPDX-License-Identifier: MIT -->

# Optional RP1 GPCLK package installation

> Historical predecessor procedure

The commands below describe released package 1.1.1 only. Roadmap Step 4
consumes unreleased 1.1.2 development source and deliberately has no frozen
package identity. The ordinary installer now rejects
`INSTALL_RP1_GPCLK_DKMS=true`; it cannot install 1.1.1 as the current
development consumer or fabricate a 1.1.2 package. The Si5351 backend remains
available without this kernel module.

For historical reproduction only, use the released, immutable predecessor. Record its
release version and SHA-256 from the RP1-GPCLK-DKMS release, then run the
installer from a local WsprryPi checkout:

~~~sh
sudo env \
  INSTALL_RP1_GPCLK_DKMS=true \
  RP1_GPCLK_DKMS_PACKAGE=/absolute/path/rp1-gpclk-dkms_1.1.1-1_all.deb \
  RP1_GPCLK_DKMS_SHA256=247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d \
  RP1_GPCLK_DKMS_VERSION=1.1.1-1 \
  ./scripts/install.sh
~~~

The optional helper fails closed unless the host is arm64, identifies as a
Raspberry Pi 5 Model B, runs a stock +rpt-rpi-2712 kernel, and has an exact
linux-headers-$(uname -r) package available. It installs those exact headers
and then gives the local package to APT. The package owns its DKMS lifecycle;
WsprryPi does not run dkms add, dkms build, or dkms install itself.

The package install stages GPIO4 and GPIO20 overlay files, but it must not edit
/boot/firmware/config.txt, load the module, publish /dev/rp1-gpclk, select a
route, reboot, or enable a clock. The helper checks that boot configuration,
module-load state, and endpoint presence are unchanged across installation. It
refuses to begin if rp1_gpclk_dkms is already loaded or /dev/rp1-gpclk already
exists; establish an attributable inactive state before invoking it. Route
output-inhibited validation, activation, and hardware qualification remain separate, explicitly authorized
operations.

Package installation also leaves the route-manager socket disabled. After the
RP1 backend has been deliberately selected, enroll the fixed account from
`wsprrypi.service` and enable the exact package socket as a separate policy
step:

~~~sh
sudo env ENABLE_RP1_GPCLK_ROUTE_MANAGER=true \
  ./scripts/install-rp1-gpclk-package.sh enroll
~~~

Enrollment accepts no caller-selected account, group, unit, or socket path. It
derives the account from the fixed WsprryPi unit, uses only the fixed
`rp1-gpclk-route` group, verifies both installed unit hashes, and requires the
live socket to be `root:rp1-gpclk-route` mode `0660`. This enables closed
route-management requests; it does not select a route or establish live/RF
eligibility.

Package-build tools such as debhelper, dh-dkms, and device-tree-compiler
belong on the RP1-GPCLK-DKMS build host. They are not WsprryPi runtime
dependencies. A successful package or header build is offline compatibility
evidence, not GPIO, timing, transmission, or RF qualification.
