# Issue #414 Slice 48 — Closeout Audit

Date: 20 August 2026

Status: **Not ready to close — two product closeout blockers and two test-harness
findings remain.**

## Acceptance reconciliation

| Acceptance area | Evidence | Verdict |
|---|---|---|
| Readable local creation and saving | Candidate lifecycle, browser download, exact archive digest, CLI compatibility, and Slice 45 real candidate | Met |
| Privacy inventory and review | Manifest categories, review steps, explicit exact-byte consent, exclusions/recollection path, and responsive UI | Met |
| Exact reviewed-byte encryption | Finalization size/digest binding, age round trip, production metadata, and Slice 45 receipt | Met |
| Separate encryption and signing keys | Distinct provisioners, public metadata, production ceremony, and private backup/recovery records | Met |
| Signed dynamic intake policy | Retrieval, Ed25519 exact-byte validation, state durability, endpoint, active/disabled/upgrade mapping, and production manifest | Met |
| Fail-closed availability and local fallback | Invalid/expired/disabled/incompatible/retrieval/rollback tests and retained local workflow | Met |
| Rotation without application upgrade | Lifecycle rotate/disable/renew tooling and signed generation transition tests | Met |
| Minimum-version upgrade gating | SemVer policy, durable higher-generation replay defense, limited release guidance, and UI states | Met |
| Existing/new/no-GitHub correlation | Strict private context, prepared safe public text, fixed GitHub destinations, non-GitHub description/contact requirement | Met |
| Dropbox disclosure and truthful state | Explicit metadata disclosure/consent, browser handoff, user-reported versus maintainer-confirmed distinction | Met |
| Anonymous signed-out upload | Slice 45 Chrome-incognito upload to the production Dropbox File Request | Met |
| Maintainer receipt and retention | Production-key inspection, canonical promotion, active-case record, audit and one-case deletion tooling | Met |
| External maintainer confirmation | Slice 47 exact public Issue #414 correlation comment | Met |
| Abuse response | Close/replace/signed-generation contract and maintainer lifecycle/publication tooling | Met, operational exercise deferred until needed |
| Existing CLI behavior | Collector regression and preserved command-line entry point | Met |

## Closeout blockers

### P1 — Operator documentation describes the superseded workflow

`Wsprry_Pi_Docs/docs/User_Interface/Maintenance/support_bundle.md`, its
Maintenance index entry, and Support guidance still describe downloading a
readable `.tar.gz` and sharing or attaching it directly. They do not document
issue/no-GitHub context, exact-byte review consent, local age encryption,
receipt preservation, Dropbox metadata and signed-out handoff, truthful upload
reporting, safe GitHub continuation, upgrade/disabled states, or maintainer
processing and retention. Shipping the feature with these instructions would
direct operators around its privacy boundary.

**Required next action:** an explicitly authorized cross-repository operator
documentation slice, following `Wsprry_Pi_Docs/AGENTS.md`, with rendered desktop
and mobile documentation review. Existing screenshots should be replaced only
where materially inaccurate.

### P2 — Clipboard fallback loses focus in current Chromium

The existing-issue continuation correctly selects the safe public comment when
Clipboard API writing fails, but current Chromium leaves `document.activeElement`
on the document body rather than the readonly comment textarea. This weakens
keyboard recovery: a user may need to navigate back to the field before copying.
The official browser test fails its focus assertion with an empty active-element
ID; after bypassing only that recorded assertion in an isolated fixture, the
remaining scenarios and visual-state captures pass.

**Required next action:** use `/impeccable harden` for the clipboard failure
path, preserve the selection and explicitly restore textarea focus, then rerun
the unmodified DOM-capable test in current Chromium.

## Test-harness findings

- **P2:** the browser harness can throw `ENOTEMPTY` while removing Chromium's
  profile directory in `finally`, masking the actual earlier assertion. Cleanup
  must wait/retry without replacing the primary result.
- **P2:** screenshot capture does not suppress the unrelated UI-version refresh
  modal, so generated workflow screenshots can be obscured. The harness should
  establish a matching version identity or deliberately dismiss the modal before
  capture without changing product behavior.

These are evidence-integrity defects, not observed end-user workflow failures.

## Impeccable UI audit

Implementation Integrity verdict: **Pass.** The workflow is product-specific,
uses semantic fieldsets, labels, status regions, explicit consent, safe fixed
destinations, truthful state language, design tokens, and coherent light/dark
responsive panels. The mechanical detector returned no findings. Fresh 1440 by
1100 and 390 by 844 renders showed no horizontal overflow; content remains
readable and actions wrap at mobile width.

| Dimension | Score | Key finding |
|---|---:|---|
| Accessibility | 3/4 | Clipboard failure does not retain focus on the selected textarea |
| Performance | 4/4 | Lean event-driven JavaScript; no layout-thrashing or animation issue found |
| Responsive design | 4/4 | Desktop/mobile actions and long disclosure text wrap without horizontal overflow |
| Theming | 4/4 | Bootstrap/design tokens and light/dark states remain coherent |
| Implementation integrity | 4/4 | Detector clean; workflow is precise, guarded, and product-specific |
| **Total** | **19/20** | **Excellent; one accessibility hardening fix remains** |

Positive findings include native form semantics, keyboard-visible controls,
bounded helper/error regions, consent gates before encryption and Dropbox,
plain-language privacy disclosure, and a public continuation that never includes
private description/contact or the signed transfer capability.

## Automated and runtime evidence

The complete focused Issue #414 Make target set was invoked. Loopback/process
tests were rerun outside the restricted sandbox, and retrieval, HTTP, and
maintainer inspection passed independently after an earlier broad parallel run
caused resource-contention timeouts. The runtime installer fixture uses GNU
`stat` and is intentionally Linux-specific: it fails on macOS and passed in the
isolated `wspr4` fixture. The remaining focused targets passed or reported their
documented platform skip.

The current UI browser fixture passed its complete remaining scenario/assertion
set after bypassing only the recorded focus assertion for diagnostic continuation. Unobscured active,
reported, upgrade, desktop, mobile, light, and dark screenshots were then
rendered and inspected. This qualified evidence does not count the original
unmodified browser suite as passing.

No installation, service change, reboot, production state change, hardware,
GPIO, transmitter, RF, Dropbox deletion, retention deletion, diagnostic access,
merge, release, or issue closure occurred.

## Impeccable and collateral follow-up

Installed Impeccable is v4.0.3 and v4.0.4 is available. `PRODUCT.md` predates
the current schema, and `.impeccable/design.json` is stale relative to
`DESIGN.md`. The previously requested closeout work remains:

1. update Impeccable on every applicable host;
2. run `init` deliberately to refresh the product record through confirmed
   answers rather than inference; and
3. run `document` to regenerate the design sidecar while preserving the design
   source and update the product collateral/design metadata.

These updates were not performed as an audit side effect.

## Documentation Impact

This prompt and audit record provide the repository closeout evidence. The
separate operator documentation was inspected read-only and is a blocking
follow-up. No operator-doc files or UI sources were changed in this slice.

## Required sequence before closure

1. Fix and qualify the clipboard-focus and browser-harness findings.
2. Update and render the separate operator documentation.
3. Update Impeccable across hosts and reconcile its product/design collateral.
4. Rerun the closeout audit against the resulting exact commits.
5. Obtain separate approval for integration/merge, release handling if any, and
   closing Issue #414.
