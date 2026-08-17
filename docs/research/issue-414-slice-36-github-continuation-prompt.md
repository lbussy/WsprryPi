# Issue #414 Slice 36 — truthful GitHub continuation

## Objective

Complete the optional public case-correlation step after the user has reported a
successful private Dropbox upload. Keep GitHub authentication and publication
under the user's control, and keep private support context out of public URLs and
issue text.

## Verified starting point

- Slice 35 records `upload_reported_complete` only after the user explicitly
  asserts that Dropbox displayed its success message.
- Collection already records one of `existing_github_issue`, `new_github_issue`,
  or `no_github`; description and contact for the latter two are inside the
  encrypted bundle.
- The application has no GitHub credential and cannot create an anonymous issue
  or post a comment on the user's behalf.

## Scope

1. Reveal an inline GitHub continuation panel only after the upload report endpoint
   returns `upload_reported_complete`.
2. For an existing issue, provide a fixed-repository issue link and a read-only,
   copyable status note containing only the case ID and a statement that no
   diagnostic or transfer link is attached.
3. For a new issue, provide a fixed-repository prefilled issue link whose title
   and body contain only the case ID and safe fixed text. The private problem
   description and contact MUST NOT enter the URL.
4. Explain that GitHub sign-in is required and that the user must review and
   submit the issue or comment themselves.
5. For `no_github`, explain that no public issue is needed and that the encrypted
   bundle already contains the required private description/contact.
6. Reset the continuation completely when the retained case is deleted or a new
   workflow starts.

## Security and truthfulness constraints

- Use only `https://github.com/WsprryPi/WsprryPi/issues/...` destinations built
  from a validated numeric issue number or fixed `/issues/new` path.
- Do not call a GitHub API, store a token, claim an issue/comment was submitted,
  or infer GitHub authentication.
- Do not place diagnostics, Dropbox URLs, encrypted filenames, email addresses,
  callsigns, locators, network details, private problem descriptions, or contact
  information in public content or query parameters.
- Do not reveal this step after page-open alone or after a failed upload-report
  request.
- Clipboard failure must leave selectable text and an actionable explanation.

## Validation

- Extend source regression and browser integration coverage for existing, new,
  and non-GitHub paths; early disclosure; copy success/failure; safe URL content;
  report retry; and reset.
- Run JavaScript syntax, UI source, browser integration, and relevant parent
  wiring tests on macOS.
- Run the applicable hardware-free UI tests on the authorized `wspr4` isolated
  snapshot if practical; do not install, operate services, reboot, access GPIO,
  or transmit.
- Run the Impeccable detector exactly once after UI edits and inspect desktop and
  mobile renders in light and dark themes.
- Run `git diff --check`, review the complete staged diff, commit only this slice,
  and push only the current integration branch.

## Documentation impact

Update the implementation plan and add a Slice 36 implementation record. Review
the separate operator documentation repository but do not modify it without
separate cross-repository authorization.

## Non-goals

- GitHub API integration, OAuth, anonymous issue creation, or automatic comments.
- Publishing the private problem description or contact information.
- Dropbox automation or maintainer-confirmed receipt.
- Changes to collection, encryption, signed intake, retention, hardware, RF,
  installer, service, or reboot behavior.

## Exit criteria

The three correlation choices have truthful, safe post-report outcomes; private
fields cannot reach public GitHub navigation; tests and visual review pass; the
documentation boundary is accurate; and an adversarial final diff review finds
no actionable blocker.
