# Issue 414 Slice 33 Execution Prompt: Local Encryption and Receipt

## Objective

Connect the authenticated production bundle key to the immutable finalized
candidate, encrypt it locally with the qualified `age` primitive, and let the
operator download the ciphertext and truthful receipt. Stop before Dropbox.

## Requirements

1. Compile the reviewed production `wsprrypi-bundle-2026-01` public recipient
   into a narrowly typed encryption-key mapping and test exact agreement with
   its checked-in public metadata.
2. Accept encryption only for the current finalized private candidate and only
   when a fresh guarded intake resolution is `active` with the matching key ID.
3. Revalidate the descriptor-pinned archive size and SHA-256, use only
   `/usr/bin/age`, and preserve the readable candidate on every failure.
4. Make encryption idempotent, prevent concurrent attempts, reject key mismatch,
   and publish only the primitive-validated mode-`0600` ciphertext.
5. Add guarded POST encryption, ciphertext GET, and receipt GET endpoints with
   no-store/nosniff headers and safe attachment names.
6. Do not write a receipt at encryption completion. After the ciphertext GET
   provider reports complete delivery, write the bounded receipt with
   `upload_state=encrypted_artifact_downloaded`; only then expose receipt GET.
7. In the UI, after an explicit successful availability check, disclose that
   encryption occurs on the Pi and add explicit **Encrypt reviewed candidate**
   consent. Show progress, failure recovery, encrypted download, and receipt
   download inline without a modal.
8. A repeated availability check that is non-active or changes key ID must
   remove encryption authorization. Reset/delete must clear transfer state.
9. Never display, persist, log, copy, or open the Dropbox request URL. Do not
   open external transfer pages or claim upload.
10. Add manager, HTTP, source, DOM-browser, and production-metadata tests for
    lifecycle, idempotency, mutation, mismatch, failed/partial download,
    receipt timing, guards, headers, reset, duplicate actions, and failures.

## Validation

- Run focused local tests, the complete UI unit/browser suite, JavaScript syntax,
  Impeccable detector, desktop/mobile renders, and `git diff --check`.
- On wspr4, use an isolated owner-only temporary directory to run the real-age
  round trip and focused non-hardware tests. Do not install, operate services,
  reboot, access GPIO/I2C, transmit, or use RF.
- Commit and push only this branch after correcting actionable review findings.

## Non-goals

No Dropbox navigation/upload, completion confirmation, GitHub posting,
installer/service mutation, hardware activity, transmission, or RF.
