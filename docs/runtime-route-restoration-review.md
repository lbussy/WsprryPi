<!-- SPDX-License-Identifier: MIT -->

# Runtime route restoration review, 2026-08-31

The application companion, startup policy, readiness acknowledgement and route
feedback were reviewed alongside the DKMS restoration transaction. No kernel,
UAPI or reusable transmitter-component source changed in this application work.

Repaired findings included saving the pin before an overlay succeeded,
automatic transmission through Always/Follow startup preferences, startup lock
contention, listener readiness, stale completion presentation, and bootstrap installation resolving
the helper relative to the invoking script instead of the selected checkout.
The final reassessment found no remaining actionable findings in the implemented
canonical-service workflow and offline validation scope.

Validation passed:

- Debian release build with the default compiled backend set.
- Route-service and runtime-wiring C++ tests, including readiness messages,
  deferred configuration ownership and stale-controller presentation.
- Seven configuration/installer companion tests, including atomic failure,
  preservation of unrelated content and file mode, all three boot policies,
  missing/old installations and bootstrap/dry-run behavior.
- Complete `semantics-test` in a fresh network-disabled Debian container,
  including idle restoration overriding Never, Follow and Always without
  changing the saved boot preference.
- Route UI behavior tests, installer shell syntax and whitespace checks.
- Impeccable desktop/mobile inspection of the existing route panel with local
  assets and no page network access; no horizontal overflow. Failed preflight
  and lost-connection tests verified that switches are not retried automatically.

The initial broad semantics run used mixed Mac/Linux build artifacts and failed
to execute those artifacts. The isolated rerun passed. A C++ comparison error
found during iteration was fixed before the final targeted build/test pass.

## Documentation Impact

Updated `docs/runtime-route-workflow.md` describes successful idle restoration,
stopped/masked behavior, installation, explicit recovery and limitations.
The separate Wsprry_Pi_Docs repository was inspected but not modified: follow-up
updates belong in `docs/Install/index.md`,
`docs/Advanced_Operations/rest_api.md` and
`docs/Advanced_Operations/websocket.md` after this coordinated implementation
is selected for deployment. That repository is outside this task's write scope.

No target installation, service operation, reboot, GPIO change or RF testing was
performed. Actual systemd restoration and clock-disabled route switching remain
target validation, not established by these offline results.
