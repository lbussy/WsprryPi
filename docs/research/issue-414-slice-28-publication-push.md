# Issue 414 Slice 28: Production Publication Push

Status: Published and ref-confirmed; public-byte verification deferred

## Outcome

Production generation 1 is published on `WsprryPi/support-intake` at:

```text
refs/heads/main 3e0b4017bfe7612bd39ccc6e2f29d743174664b5
```

The qualified Slice 17 controller first returned `proposed`, proving that live
remote `main` was exactly the expected parent:

```text
770d63521cf23d1ccb5eb7c9911e040ab18032d7
```

The approved invocation then performed one exact lease-protected push of the
verified candidate to `refs/heads/main`. Its immediate post-push query returned
the candidate and status `published`. A separate controlled ref-only query also
returned the exact candidate.

## Published identity

The non-sensitive published identity is:

- generation: `1`;
- intake status: `active`;
- signing key ID: `wsprrypi-intake-2026-01`;
- bundle key ID: `wsprrypi-bundle-2026-01`;
- manifest SHA-256:
  `80902216b212ca1a8c2a9fd3e9693aac2c0aa17c7838d939bbebaa8887fb71e8`;
- parent commit: `770d63521cf23d1ccb5eb7c9911e040ab18032d7`; and
- published commit: `3e0b4017bfe7612bd39ccc6e2f29d743174664b5`.

The controller re-authenticated the staged manifest/signature and verified the
candidate's parent, exact two-path diff, and exact source bytes before both the
proposal and approved push. It used only the allowlisted macOS Keychain Git
credential helper, fixed origin, exact lease, and exact candidate-to-main
refspec. Credentials were not accepted, printed, or handled manually.

## Post-push validation

After confirmation:

- an independent controlled `ls-remote --refs` query returned only the exact
  candidate for `refs/heads/main`;
- local lifecycle inspection still authenticated unchanged generation 1 with
  the same policy, key IDs, and manifest digest;
- the local publication commit still changes exactly
  `wsprrypi/intake.json` and `wsprrypi/intake.json.sig`; and
- both local candidate blobs remain byte-identical to the authenticated staged
  source.

Focused validation passed:

- manifest lifecycle: 10 tests, one real-tool fixture skipped locally;
- publication commit: 8 tests;
- publication push: 8 tests;
- Python syntax and final diff checks.

Known macOS Make probes for Linux `/proc/meminfo` and `nproc` emitted warnings
without affecting the tests. GitHub CLI authentication was confirmed through
the system keyring before the external write; no credential value was exposed.

## Remaining boundary

This slice intentionally did not retrieve `intake.json` or `intake.json.sig`
from public raw endpoints. The next separately reviewed Slice 18 operation must
retrieve both exact public files through the qualified HTTPS boundary, compare
them byte-for-byte with the authenticated source, verify the signature and
manifest digest, and confirm generation 1 before publication is considered
end-to-end verified.

Application trust activation, upload orchestration, UI, installer, services,
Raspberry Pi, GPIO, transmitter, and RF activity remain unchanged and deferred.
