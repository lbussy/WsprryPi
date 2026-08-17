# Issue 414 Slice 35 Execution Prompt: Truthful Upload Reporting

## Objective

Implement the post-handoff state boundary without inferring Dropbox success.
Record that the upload page opened only when the guarded endpoint issues its
fresh authenticated redirect, and record upload completion only when the user
explicitly reports that Dropbox displayed success. Keep maintainer confirmation
external.

## Verified starting point

- Slice 34 requires a completed encrypted download, full disclosure, explicit
  handoff consent, and fresh signed-intake resolution before redirecting.
- The frozen protocol distinguishes `encrypted_downloaded`,
  `upload_page_opened`, `upload_reported_complete`, and maintainer confirmation.
- The version-1 receipt currently and truthfully records only
  `encrypted_artifact_downloaded`; updating it is optional, not required.

## Requirements

1. Extend the retained private lifecycle with `encrypted_downloaded`,
   `upload_page_opened`, and `upload_reported_complete`.
2. Move to `encrypted_downloaded` only after the exact ciphertext completion
   callback validates the bytes and publishes the initial receipt.
3. Move to `upload_page_opened` only after job eligibility and a fresh coherent
   active intake have passed and immediately before issuing the Dropbox
   redirect. Repeated valid handoffs are idempotent and remain retryable.
4. Add a guarded, empty-body POST for the user completion assertion. Permit it
   only after `upload_page_opened`; make repeated reporting idempotent; expose
   only the typed state and no Dropbox capability or private context.
5. Do not accept or store Dropbox name/email, timestamps supplied by the user,
   filenames, URLs, arbitrary notes, or provider data in the assertion.
6. After the handoff action, show an inline return step stating that page-opened
   is not success. Require a separate unchecked assertion that Dropbox displayed
   `Finished uploading` after the `.age` file was selected and submitted.
7. On success, state exactly that the user reported Dropbox success, that this
   is not maintainer confirmation, and that the readable archive remains on the
   Pi until delete/expiry. Preserve the case ID and retry/delete controls.
8. Disable duplicate submissions, retain the assertion on transient failure,
   provide an actionable retry, and revoke/reset this state on deletion or a new
   workflow.
9. Keep the downloaded receipt at
   `upload_state=encrypted_artifact_downloaded`; do not claim to mutate a file
   already saved by the browser and do not introduce a replacement receipt in
   this slice.
10. Add focused manager, HTTP, source, and DOM/browser tests for every allowed
    and rejected transition, idempotency, guards, body rejection, failure
    recovery, truthful copy, reset, and non-disclosure.
11. Use Impeccable in Operate mode, inspect the finished desktop/mobile states,
    and run its detector exactly once after UI changes.
12. Update the implementation record and plan; identify the separate operator
    documentation follow-up without modifying that repository.

## Non-goals

Do not automate Dropbox, infer provider success, poll Dropbox, accept Dropbox
credentials or metadata, claim maintainer confirmation, change the receipt
schema/file, post to GitHub, modify the installer, operate services, install the
application, reboot, access GPIO/I2C, transmit, or perform RF qualification.

## Exit criteria

Every state transition is forward-only, idempotent where retried, guarded, and
truthfully named. Opening a page cannot produce a completion claim, only an
explicit user assertion can produce `upload_reported_complete`, and nothing in
the application can produce maintainer confirmation. Focused Mac/wspr4 tests,
responsive visual review, the one detector pass, and `git diff --check` pass.
