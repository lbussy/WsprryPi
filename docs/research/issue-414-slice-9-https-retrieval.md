# Issue 414 Slice 9: Bounded HTTPS Intake Retrieval

Status: Exact-byte retrieval primitive implemented; validation/controller wiring deferred

Depends on:

- [Slice 7 offline signed-intake validation](issue-414-slice-7-intake-validation.md)
- [Slice 8 signed-intake rollback state](issue-414-slice-8-intake-state.md)

## Outcome

WsprryPi now has a shell-free C++ transport that independently retrieves the
exact manifest and detached signature-envelope response bodies from the paired
reserved `raw.githubusercontent.com/WsprryPi/support-intake` endpoints.

The transport accepts only the two exact version-1 endpoint URLs and fixed byte
limits. Production requires the exact root-owned, non-symlinked, non-writable
`/usr/bin/curl`; a typed test-only seam permits a current-user-owned fake
executable. It executes curl directly with:

- curl configuration disabled before all other options;
- a minimal controlled environment without proxy or CA override variables;
- IPv4 transport, matching the qualified Raspberry Pi deployment where IPv6
  DNS answers are present without a usable IPv6 route;
- HTTPS-only initial and redirect protocol policy;
- ordinary curl CA-chain and hostname verification, with no insecure option;
- redirects left disabled and a required final HTTP status of 200;
- explicit connect and whole-operation deadlines; and
- curl plus controller-side streaming size limits.

The controller captures binary stdout, removes only its fixed four-byte HTTP
status trailer, and otherwise preserves body bytes exactly, including NULs and
newlines. It requires a dedicated child process group and terminates the whole
group, including surviving descendants, on timeout or oversize while ensuring
the direct child is reaped. Any
manifest or signature failure returns neither document, so partial or unpaired
bytes cannot flow into validation.

No response is parsed, logged, persisted, or treated as trusted by this slice.

## Validation

`make support-bundle-intake-retrieval-test SUDO=` uses a compiled fake curl
process without network access. The fixture checks the complete argv and exact
controlled environment and covers:

- binary exact-byte success for both responses;
- endpoint, limit, executable, symlink, and permissions rejection;
- exec failure, empty response, nonzero exit, signal termination, and partial
  output non-disclosure;
- manifest and signature streaming oversize enforcement;
- manifest and signature parent-deadline termination; and
- HTTP redirect-status rejection without following a location;
- fail-closed process-group setup; and
- termination of a signal-resistant descendant after its leader exits.

The focused target is included in Debian non-hardware CI.

## Remaining work

- Compose Slice 9 retrieval, Slice 7 authentication/policy, and Slice 8 state
  commit behind one typed controller with failure-safe ordering.
- Provision and publish production signing/bundle public metadata only after the
  private-storage and recovery destinations are selected.
- Wire the controller into application runtime and later UI/operator workflows.

No production manifest, signature, public key, bundle recipient, Dropbox request
ID, runtime wiring, installer, UI, service, hardware, GPIO, I2C, transmitter, or
RF behavior changed in this slice.
