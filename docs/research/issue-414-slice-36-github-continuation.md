# Issue #414 Slice 36 — truthful GitHub continuation

## Outcome

After the upload-report endpoint durably accepts the user's assertion, the
Maintenance workflow now presents the correlation path selected before
collection. Existing issues receive a fixed WsprryPi issue link and a reviewable,
copyable public note. New issues receive a fixed-repository prefilled URL that
contains only safe fixed text and the case ID. The non-GitHub path confirms that
the private description and contact information are already inside the encrypted
bundle.

The application does not authenticate to GitHub, create an issue, post a comment,
or claim that either happened. GitHub sign-in and final submission remain explicit
browser actions by the user. Private problem descriptions, contact information,
diagnostics, transfer links, filenames, callsigns, locators, and network details
are not inserted into public content.

## Implementation boundary

- The continuation remains hidden after Dropbox page-open and after a failed
  upload-report request.
- Existing issue numbers come from the already validated collection context and
  are placed only below `https://github.com/WsprryPi/WsprryPi/issues/`.
- New issue query parameters are generated from a case ID and fixed safe copy.
- Clipboard failure selects the visible read-only note for manual copying.
- Deleting the retained case clears the panel, text, and both GitHub targets.

No GitHub token, API request, OAuth flow, anonymous intermediary, Dropbox
automation, maintainer confirmation, backend lifecycle change, hardware access,
installation, service operation, reboot, or RF operation was added.

## Validation

The UI source regression and browser integration test cover the fixed repository,
all three correlation paths, post-report reveal, existing-issue note and copy,
failure-before-reveal, private-field non-disclosure, and full reset.

- macOS: `node --check data/maintenance.js`, `npm run test:unit`, and the focused
  Chrome `support_intake_ui_integration_test.js` passed.
- Impeccable: the detector reported only two pre-existing advisory CSS-token
  findings in unchanged `maintenance.css`. Desktop and mobile completed-state
  renders and a dark-theme desktop render were inspected; the copy action was
  changed to the established primary outline treatment for dark-theme contrast.
- macOS parent `make semantics-test SUDO=` was attempted but stopped before this
  regression at the known Clang `-Wpessimizing-move` failure in
  `src/LCBLog/src/lcblog.tpp`.
- Authorized `wspr4` isolated snapshot: JavaScript syntax, the UI source test, and
  a freshly compiled parent `ui_source_regression_test` passed. The snapshot was
  removed and `/home/pi/WsprryPi` remained clean on `devel`.

## Documentation impact

The implementation plan now points to this slice and this record documents the
runtime boundary. The separate operator documentation repository was considered
but not changed because cross-repository documentation work was not authorized;
its eventual support-bundle guide should describe the three correlation outcomes
and emphasize that GitHub publication is manual.

## Remaining boundary

Maintainer receipt remains external to WsprryPi. Release qualification still
requires the contract's signed-out Dropbox acceptance test and operator
documentation in the separate documentation repository.
