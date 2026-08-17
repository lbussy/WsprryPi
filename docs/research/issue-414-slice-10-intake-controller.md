# Issue 414 Slice 10: Signed-Intake Controller

Status: Offline controller composition implemented; runtime construction deferred

Depends on:

- [Slice 7 offline signed-intake validation](issue-414-slice-7-intake-validation.md)
- [Slice 8 signed-intake rollback state](issue-414-slice-8-intake-state.md)
- [Slice 9 bounded HTTPS retrieval](issue-414-slice-9-https-retrieval.md)

## Outcome

WsprryPi now has one typed controller that composes the three existing intake
primitives in this fixed order:

1. load private rollback state;
2. retrieve the exact paired manifest and signature envelope;
3. authenticate and validate the exact bytes against pinned signing and bundle
   key metadata plus the loaded prior state; and
4. commit the validated generation and exact-byte manifest SHA-256.

An absent state file is accepted as first use. An unsafe, unreadable, or invalid
state fails before retrieval. Retrieval and validation failures stop before
commit. The Slice 8 commit remains the final authority if another writer
advances state after the initial load.

The controller returns a manifest only after a `committed` or `unchanged` result
confirms rollback-state durability. A `committed_sync_uncertain` result returns
no manifest; an identical retry may confirm durability through Slice 8's
directory re-sync and `unchanged` path. Typed stage statuses identify the failed
boundary without returning fetched bytes or unverified manifest fields.

Production composition calls the concrete Slice 7-9 primitives. A typed
in-process dependency seam exists only for deterministic tests and is not
available through CLI, configuration, environment, HTTP, or UI.

## Validation

`make support-bundle-intake-controller-test SUDO=` covers:

- exact load, retrieve, validate, and commit order;
- absent and loaded prior-state propagation;
- durable committed and unchanged success;
- state-load, retrieval, validation, and commit failures with no later calls;
- result non-disclosure even when a failing dependency supplies fields or bytes;
- a real-state competing advancement that rejects the stale controller commit;
- real ephemeral Ed25519 signing and exact-byte validation with real private
  state persistence; and
- published-but-uncertain state followed by an identical durability-confirming
  retry.

The focused target is included in Debian non-hardware CI. Tests do not contact a
network endpoint and no private signing key fixture is retained.

## Remaining work

- Select and provision production signing-public and bundle-recipient metadata
  only after permanent private-storage and recovery destinations are approved.
- Construct this controller from application-owned trust metadata, clock, and a
  private runtime state directory in a separate runtime-integration slice.
- Compose validated intake selection with archive encryption and Dropbox upload.
- Add disabled, upgrade-required, rotation, recovery, and operator UI workflows
  only after the runtime contract exists.

No production manifest, signature, key, recipient, Dropbox request ID, runtime
wiring, retry loop, cache, installer, UI, service, hardware, GPIO, I2C,
transmitter, or RF behavior changed in this slice.
