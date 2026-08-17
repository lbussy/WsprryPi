# Issue 414 Slice 34 Execution Prompt: Dropbox Handoff Boundary

## Objective

Add the first deliberate transition from the local support-bundle workflow to
the project Dropbox File Request. Present the complete privacy disclosure,
require explicit handoff consent, and open only a freshly authenticated active
request capability. Stop at opening the Dropbox page.

## Verified starting point

- Slice 33 creates, reviews, finalizes, encrypts, and downloads an exact local
  candidate and its receipt without uploading anything.
- The signed-intake production resolver already authenticates, version-gates,
  expires, disables, and rotates the Dropbox request URL.
- The browser currently validates active intake for encryption but does not
  render, persist, copy, or open the request capability.

## Requirements

1. Add a guarded navigation endpoint that invokes the production intake
   provider for every handoff attempt. Redirect only a coherent `active` result
   whose request URL is an exact Dropbox File Request URL. Fail closed without
   disclosing a URL for disabled, upgrade-required, unavailable, malformed, or
   throwing providers.
2. Apply `Cache-Control: no-store`, `X-Content-Type-Options: nosniff`, and
   `Referrer-Policy: no-referrer` to every handoff response. Do not add
   permissive CORS behavior.
3. Reveal the handoff panel only after the encrypted artifact has completed its
   local browser download. Keep the readable archive and encrypted artifact on
   the Pi for retry under the existing retention contract.
4. Before enabling navigation, disclose verbatim in substance that Dropbox
   will receive the encrypted bundle; asks for a name and valid email; requires
   no Dropbox account; and can observe filename, size, upload time, network
   information, and entered name/email, while it cannot read encrypted content.
5. Require an unchecked explicit consent control. State that opening Dropbox
   is not upload confirmation and instruct the operator to select the
   downloaded `.age` file on the Dropbox page.
6. Open the handoff in a new tab through the local navigation endpoint. Never
   place the Dropbox capability in rendered text, DOM attributes, JavaScript
   storage, logs, or a build-time constant.
7. Revoke and reset handoff state when intake becomes non-active, the workflow
   is deleted/reset, or a new candidate begins.
8. Add backend tests for guarded access, empty-body enforcement, fresh provider
   invocation, exact redirect, strict URL rejection, non-disclosure, and
   private headers. Add DOM/browser tests for disclosure, consent gating,
   encrypted-download sequencing, safe local handoff URL, revocation, reset,
   and truthful copy.
9. Use Impeccable in Operate mode, inspect desktop and mobile states, and run
   the detector exactly once after UI changes.
10. Update the implementation record and plan. Report the separate operator
    documentation follow-up without modifying its repository.

## Non-goals

Do not automate file selection or upload, infer Dropbox success, mark an upload
complete, contact GitHub, modify the installer, alter production intake
metadata, operate services, install the application, reboot, access GPIO/I2C,
transmit, or perform RF qualification.

## Exit criteria

The handoff is impossible without completed ciphertext download, explicit
consent, same-origin guard approval, and a fresh authenticated active intake.
All focused backend/UI tests, responsive visual review, detector review, and
`git diff --check` pass, with no Dropbox capability disclosed on failure.
