# Issue 414 Slice 33: Local Encryption and Receipt

Status: Implemented and non-hardware qualified; Dropbox remains inactive

## Outcome

The finalized readable candidate can now be encrypted locally after a fresh
guarded intake resolution confirms `active` and the production bundle key ID.
The runtime binds that ID to the reviewed production age recipient, revalidates
the descriptor-pinned archive, and invokes only `/usr/bin/age` through the
existing shell-free primitive.

The guarded API publishes separate encrypted-artifact and receipt downloads.
The receipt is not created when encryption finishes: it is created only after
the encrypted response completes successfully, so
`encrypted_artifact_downloaded` remains truthful. Failed or partial delivery
does not unlock the receipt. Repeated encryption is idempotent, and deletion or
expiry removes the retained job directory and all associated artifacts.

The UI adds inline local-encryption consent only after an active availability
check. It distinguishes encryption, ciphertext download, and receipt download,
and repeatedly states that no upload has occurred. It never renders, stores, or
opens the Dropbox capability.

## Validation

Local macOS validation passed:

- JavaScript and Python syntax checks;
- UI unit suite and complete Chromium browser suite;
- manager lifecycle, private-artifact, HTTP, wiring, and production-metadata
  tests using the simulated/GPIO-free build profile;
- Impeccable detector and desktop/mobile active, upgrade, loading, and failure
  renders; and
- `git diff --check`.

On wspr4, Debian `age` 1.2.1-1+b5 was explicitly authorized and installed. An
isolated owner-only `/tmp` source copy passed the manager, primitive, HTTP,
production-metadata, and real-age round-trip tests. The temporary tree was
removed. No application installation, service operation, reboot, GPIO/I2C,
transmission, or RF activity occurred.

wspr4 also exposed and qualified two portability hardenings: the encryption
process-group parent accepts the safe already-exited `ESRCH` race, and test
executables are copied to owner-only mode rather than depending on platform
build umasks.

## Documentation impact

- Added this implementation record and the Slice 33 prompt.
- Added an Issue #414 backlog note to make `age` an explicit installer
  dependency and verify both `/usr/bin/age` and `/usr/bin/age-keygen`.
- The separate operator guide still requires a coordinated rewrite after the
  Dropbox handoff and completion workflow are implemented.

## Remaining boundary

No Dropbox page was opened, no upload was attempted or claimed, and no GitHub
issue posting was added. The next slice should present the full Dropbox privacy
disclosure and require explicit handoff consent before opening a freshly
authenticated request URL.
