# Issue 414 Slice 32: Intake Availability UI

Status: Implemented and tested; transfer remains inactive

## Outcome

The Maintenance Support Bundle workflow now offers an explicit **Check private
upload availability** action after a readable candidate has been downloaded,
reviewed, and finalized. No intake request occurs on page load or during
collection, download, review, or finalization.

The inline result distinguishes active, disabled, upgrade-required, and
unavailable states. Active and disabled results show authenticated expiry and
optional signed guidance. Upgrade-required results show the minimum version and
an authenticated WsprryPi release link. Failure and malformed results fail
closed to one retryable unavailable state while preserving the local bundle.

The client strictly checks field sets, types, key IDs, timestamps, versions,
Dropbox request-capability form, and release URL policy. The Dropbox request URL
is reduced to an availability decision: it is not retained in UI state,
rendered, linked, copied, logged, persisted, or opened.

## Validation

Passed on the macOS development host:

```text
node --check data/maintenance.js
node --check tests/support_intake_ui_integration_test.js
npm test
npm run test:browser

make -C src build/bin/ui_source_regression_test \
  BACKENDS=simulated ANCILLARY_GPIO=0 \
  COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' SUDO=
src/build/bin/ui_source_regression_test

make -C src support-bundle-http-test support-bundle-web-server-wiring-test \
  BACKENDS=simulated ANCILLARY_GPIO=0 \
  COMMON_FLAGS='-Wall -Werror -Wno-pessimizing-move -MMD -MP' SUDO=
```

The DOM-capable test drove the production Maintenance view through collection,
download, review, finalization, explicit intake resolution, all four rendered
states, duplicate-click suppression, malformed/unsafe-response erasure, retry,
and deletion reset. It proved that no intake request occurs before the explicit
post-finalization action and that the Dropbox capability never enters rendered
text or a link.

The ordinary `semantics-test` aggregate was also attempted with the macOS
simulated/GPIO-free options. Its dial-frequency test still includes
`gpiod.hpp`, which is unavailable on this host, so the directly relevant UI
source binary was built and run separately. This known macOS portability gap is
not caused by Slice 32.

## Visual and accessibility review

The mandatory Impeccable Operate-mode hardening workflow was applied. The real
Maintenance view was rendered and inspected at 1440 by 1100 and 390 by 844 for
active, upgrade-required, loading, and unavailable states. Feedback remains
next to the initiating action; live status is announced; busy actions disable;
mobile actions use full-width targets; text wraps without horizontal overflow;
and the existing panel hierarchy, contrast, typography, spacing, focus, and
responsive behavior remain consistent. No modal was added.

## Documentation impact

- Updated the Issue 414 implementation plan and Slice 31 handoff to point to
  this prompt and result.
- Considered the separate operator guide at
  `Wsprry_Pi_Docs/docs/User_Interface/Maintenance/support_bundle.md`; it already
  predates the current review/finalization workflow and needs a coordinated
  rewrite when the transfer workflow is complete.
- Did not modify the separate documentation repository because this slice did
  not authorize a cross-repository change and does not yet let an operator
  encrypt or upload a bundle.

## Remaining boundary

No bundle encryption, encrypted artifact, upload consent, Dropbox navigation,
upload, completion claim, GitHub posting, installer/service operation, hardware
access, transmission, or RF activity was added or qualified. The next slice may
consume a freshly authenticated active capability only as part of a separately
reviewed encryption-and-transfer boundary.
