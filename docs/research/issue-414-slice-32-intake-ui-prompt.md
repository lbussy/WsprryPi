# Issue 414 Slice 32 Execution Prompt: Intake Availability UI

## Objective

Consume the guarded support-intake endpoint from the existing Support Bundle
workflow and render its four states after local review and finalization, without
opening Dropbox, encrypting, or uploading anything.

## Verified context

- Slice 31 exposes one guarded, on-demand `GET /api/support-intake` endpoint.
- The existing UI keeps the readable archive local, requires explicit review,
  and finalizes the exact reviewed bytes before a transfer can proceed.
- The endpoint returns authenticated `active`, `disabled`, and
  `upgrade_required` states, or the exact non-disclosing `unavailable` state.
- The Dropbox request URL is a capability. This slice may validate an active
  response but must neither display, persist, log, copy, nor open that URL.

## Scope and requirements

1. Add the intake endpoint to the existing centrally configured proxy/direct
   REST endpoint definitions.
2. Do not fetch intake state on page load, collection, polling, download, or
   review. Reveal an explicit **Check private upload availability** action only
   after the candidate is successfully finalized.
3. On activation, perform one no-store guarded GET, prevent duplicate requests,
   show a truthful busy state, and validate the complete response shape before
   rendering it.
4. Render these inline, accessible states:
   - `active`: private upload is available, with expiry and optional signed
     message; do not render or retain the request URL;
   - `disabled`: private upload is temporarily disabled, with expiry and
     optional signed message;
   - `upgrade_required`: name the minimum version, provide the authenticated
     official release link, and show an optional signed message; and
   - `unavailable`: explain that availability could not be checked and provide
     an explicit retry action.
5. Treat non-OK responses, exceptions, malformed JSON, unknown/extra fields,
   wrong field types, inconsistent fields, and unsafe release URLs as the same
   non-disclosing unavailable state.
6. Use text-safe DOM assignment for signed messages and metadata. Never insert
   server-derived HTML.
7. Reset and delete paths must clear the availability state and any transient
   response data. A second finalized workflow must start with a fresh explicit
   check.
8. Preserve the readable download, review consent, finalization, deletion,
   support-context validation, polling, and error-recovery behavior.
9. Keep feedback adjacent to the initiating action, announce dynamic state,
   preserve keyboard focus, support long text wrapping, and retain the existing
   desktop/mobile design language without a modal.
10. Add source and browser behavior coverage for disclosure timing, duplicate
    prevention, all four states, malformed/error erasure, retry, reset, and
    non-disclosure of the Dropbox capability.

## Constraints and non-goals

- Do not open Dropbox, expose a transfer link, encrypt a bundle, create an
  encrypted artifact, upload, claim upload success, post to GitHub, or change
  backend intake semantics.
- Do not add startup/background resolution, caching, local storage, cookies,
  telemetry, CLI/INI/environment controls, installer/service behavior,
  hardware access, transmission, or RF activity.
- Do not expose signatures, digests, public keys, controller enums, filesystem
  paths, arbitrary server errors, or private material.

## Validation and exit criteria

- Run the UI unit/source suite and a DOM-capable browser regression exercising
  the real Maintenance view with mocked endpoint responses.
- Run the parent UI source regression and Slice 31 endpoint/wiring tests.
- Render and inspect the finalized availability workflow at desktop and mobile
  sizes, including active, upgrade, loading, unavailable, focus, wrapping, and
  responsive action layout.
- Run JavaScript syntax and `git diff --check`, inspect the complete diff, and
  correct every actionable finding.
- Record implementation, test, visual, non-goal, hardware, and documentation
  evidence, then commit and push only the current Issue 414 integration branch.
