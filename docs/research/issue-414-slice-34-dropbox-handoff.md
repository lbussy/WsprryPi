# Issue 414 Slice 34: Dropbox Handoff Boundary

Status: Implemented and non-hardware qualified; upload completion remains deferred

## Outcome

The Maintenance workflow now presents the complete Dropbox privacy disclosure
after the encrypted artifact finishes downloading. It explains Dropbox's name
and email request, the no-account path, the metadata visible to Dropbox and the
maintainer, the encrypted-content boundary, selection of the downloaded
`.age` file, and the fact that opening the page is not upload confirmation.
The navigation action remains disabled until the operator explicitly consents.

The browser does not retain the signed Dropbox capability as application state
or a rendered link after validating availability. Its action targets a guarded
local job-specific endpoint.
That endpoint requires the job's completed encrypted download and receipt,
resolves the signed intake again for every attempt, independently restricts the
destination to the exact Dropbox File Request URL shape, and redirects only
for a coherent active production result. Disabled, incompatible, unavailable,
malformed, unguarded, and provider-failure paths disclose no destination.

Every response is non-cacheable and carries `nosniff`; handoff responses also
set `Referrer-Policy: no-referrer`. Reset, deletion, and a later non-active
intake result revoke the browser handoff state.

## Validation

Local macOS validation passed:

- JavaScript syntax and whitespace checks;
- support-bundle HTTP, manager, and web-server wiring tests using the
  simulated/GPIO-free profile;
- complete UI unit and Chromium browser suites;
- backend cases for body rejection, guard rejection, missing encrypted
  download, exact fresh redirect, strict destination rejection, disabled
  intake, and provider failure;
- browser cases for sequencing, full disclosure, consent gating, safe local
  navigation, revocation, and reset;
- Impeccable detector with no findings; and
- desktop and mobile visual inspection of the complete handoff workflow.

An isolated owner-only source snapshot on wspr4 passed the HTTP, manager, and
web-server wiring targets with the simulated/GPIO-free profile. The temporary
tree was removed and the normal wspr4 checkout was not modified.

No Dropbox page was opened and no upload was attempted during automated
validation. No application installation, service operation, reboot, GPIO/I2C,
transmission, or RF activity occurred.

## Documentation impact

- Added the Slice 34 prompt and this implementation record.
- Updated the implementation plan to identify the handoff boundary as current.
- The separate operator guide still requires a coordinated update describing
  the privacy disclosure, manual `.age` selection, retry behavior, and the
  distinction between page-opened, user-reported, and maintainer-confirmed
  upload states.

## Remaining boundary

This slice stops when the private Dropbox File Request opens. It does not
automate file selection, claim success, change the receipt's upload state,
collect a user's completion assertion, confirm maintainer receipt, or annotate
a GitHub issue. The next slice should implement the truthful post-handoff
completion state without treating page navigation as upload success.
