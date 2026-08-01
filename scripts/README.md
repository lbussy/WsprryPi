# Wsprry Pi Scripts

These scripts are used for install and development orchestration.

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

I2C bus scanning is opt-in: `--probe-i2c` permits only `i2cdetect -y 1`.
Without it, the collector gathers passive I2C details but records that active
probing was skipped. Running as a non-root user is supported; restricted system
diagnostics are reflected in the result artifact.

Run the focused no-hardware regression coverage with:

```sh
bash scripts/tests/collect-support-bundle_test.sh
```
