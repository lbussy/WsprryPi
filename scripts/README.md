# Wsprry Pi Scripts

These scripts are used for install and development orchestration.

## Installer dry runs

Set `DRY_RUN=true` to inspect the selected install or uninstall workflow
without changing the checkout or system. Dry runs write no installer log,
temporary merge output, staged executable, installed configuration, package,
service, boot, web, GPIO, or provider state. Passing `debug` prints the planned
commands to the console with shell-safe quoting that preserves exact argument
boundaries. Successful dry runs end with an explicit notice that the selected
plan completed without applying changes; they do not report an installation or
uninstallation as completed.

## RP1-GPCLK-DKMS provider

`install.sh` uses `rp1_gpclk_dkms_install.py` to resolve and validate the
independently owned provider before package mutation. Automatic Pi 5/CM5,
explicit override/opt-out, published release, immutable development source,
dry-run, exact-owned development no-op, foreign-state refusal, and
ownership-aware uninstall behavior are
documented in [RP1-GPCLK-DKMS installation](../docs/rp1-gpclk-dkms-installation.md).
Dry-run uses the installer's standard command wrapper and never starts the
Python helper. Passing `debug` displays the safely quoted helper command. The
wrapper suppresses child stdout and stderr in every execution mode and owns all
status and failure diagnostics.

Real installation success requires the resulting WsprryPi service to be active.
When web mode is enabled, the installed loopback `/wsprrypi/version` proxy must
also respond. A condition-skipped systemd start is not application readiness.

Run its offline hardware-free contract coverage with:

```sh
python3 scripts/tests/rp1_gpclk_dkms_install_test.py
python3 scripts/tests/installer_dry_run_purity_test.py
```

## Support-bundle collector

### Future support-route guard

`SupportRequestGuard` is not yet wired to HTTP routes. It limits a future
support route to loopback or directly connected interface subnets and validates
local Host/Origin identities. It is network-location access control, not user
authentication; forwarded headers are deliberately ignored.

`collect-support-bundle.sh` retains its interactive behavior by creating a
bundle in the current directory. A trusted future backend may instead invoke
it with `--output-dir /absolute/private/directory`. The directory must already
exist, be writable/searchable by the caller, not be a symlink, and not be
group- or world-writable. The collector selects the archive filename and will
not overwrite an existing archive, checksum, or result.

The collector writes a private `<archive>.result.json` alongside the archive
and `.sha256` sidecar. Its stable schema records success or failure, artifact
filenames and SHA-256 digest, generation time, requested collection options,
I2C probe state, and whether unprivileged execution may have omitted privileged
diagnostics. The JSON and console output intentionally contain no configuration
secrets.

Every bundle also contains a point-in-time process/resource snapshot: a wide
system process list, process tree, systemd cgroup view when available, and
WsprryPi `/proc` evidence resolved from the service's systemd `MainPID`. The
summary reports current RSS, virtual size, PSS when readable, threads, tasks,
and open-file-descriptor count. Unavailable, stopped, permission, and process
race states are labeled explicitly rather than reported as zero. This snapshot
does not provide historical resource monitoring.

I2C bus scanning is opt-in: `--probe-i2c` permits only `i2cdetect -y 1`.
Without it, the collector gathers passive I2C details but records that active
probing was skipped. Running as a non-root user is supported; restricted system
diagnostics are reflected in the result artifact.

Run the focused no-hardware regression coverage with:

```sh
bash scripts/tests/collect-support-bundle_test.sh
```
