# Issue #414 Slice 49 — Closeout Re-audit

Date: 20 August 2026

Status: **Ready for integration.** No Issue #414 implementation, UI,
documentation-content, privacy, or qualification blocker remains on the
audited revisions. The feature is not yet closed because the application and
operator-documentation branches have not been integrated into their respective
`devel` branches.

## Audited revisions

- Application integration: `d4fd9335ff3c95cd945e395a36c1a28494e1811f`
  on `codex/issue-414-slice-30-runtime-activation-adapter`.
- Impeccable collateral: `96ef06096efd1f876d2979e1f9992c05d40aff52`
  is an ancestor of the integration tip through merge commit `d4fd933`.
- Operator documentation: `26f506493a4a6651bece09dab90c2454554ef8d7`
  on `codex/issue-414-support-bundle-docs`, synchronized with its remote.
- Refreshed GitHub Issue #414 remained open with `enhancement` and `In Devel`
  labels and retained the privacy-safe maintainer confirmation for case
  `KPXV-ZKYQ-8P7J`.

## Acceptance reconciliation

Every acceptance criterion in the refreshed issue remains met:

- readable local creation, inspection, exclusion/recollection, explicit
  exact-byte approval, and preservation without upload;
- exact reviewed-byte age encryption with no plaintext fallback;
- separate public bundle-encryption and intake-signing identities;
- strict signed intake retrieval, verification, expiry, disable, rotation,
  rollback, protocol, and minimum-version behavior;
- fail-closed upload availability while local collection remains available;
- existing/new/no-GitHub correlation with encrypted private context and fixed,
  user-driven public continuations;
- explicit Dropbox metadata disclosure and truthful separation of browser
  handoff, user-reported completion, and maintainer confirmation;
- signed-out Dropbox upload evidence, production-key maintainer inspection,
  safe promotion, correlation, retention audit, and bounded deletion tooling;
- abuse recovery through signed request replacement; and
- preserved command-line bundle creation.

No public issue content, source, log, screenshot, or audit output discloses a
diagnostic bundle, private contact value, transfer capability, or private key.

## Corrections since Slice 48

The four Slice 48 findings are closed:

1. The clipboard fallback restores focus to the selected readonly public
   comment textarea and the unmodified Chromium regression passes.
2. Chromium profile cleanup retries without masking the primary browser result.
3. Screenshot setup suppresses the unrelated version-refresh modal without
   changing product behavior.
4. The separate operator guide now documents local review, age encryption,
   receipt preservation, Dropbox File Request disclosure and handoff, truthful
   upload reporting, GitHub/no-GitHub continuation, upgrade/disabled states,
   and the prohibition against public bundle or transfer-link attachment.

The refreshed `PRODUCT.md`, `DESIGN.md`, and Impeccable design sidecar are now
part of the Issue #414 integration ancestry.

## Automated qualification

The complete focused support-bundle target set was invoked sequentially.
All applicable macOS targets passed. The HTTP and HTTPS-retrieval tests passed
outside the restricted sandbox because they require loopback/process-group
facilities. The checksum descriptor-leak fixture now tries Linux
`/proc/self/fd` and falls back to macOS `/dev/fd`; it passed locally under the
strict `BACKENDS=si5351 ANCILLARY_GPIO=0` profile. Job-directory removal and
startup cleanup continue to exercise Linux descriptor-relative behavior and
passed on `wspr4` from a temporary clone of application revision `d4fd933`; no
installation or service action occurred. The checksum fixture will be rerun on
Linux at the final re-audit commit before integration.

The UI component unit suite passed, including support workflow source/state,
browser-harness cleanup, logs, spots, responsive-shell, GPIO-menu, and frequency
correlation coverage. The unmodified support-intake Chromium suite passed and
captured all requested workflow states without the version modal obscuring
evidence. The separate operator-documentation Sphinx build passed without
warnings at revision `26f5064`.

The broader macOS `semantics-test` was first invoked with the default profile,
then correctly retried with `BACKENDS=si5351 ANCILLARY_GPIO=0` and the documented
Clang warning flags. Its test source directly includes `gpio_output.hpp`, so it
still requires unavailable `gpiod.hpp` despite the strict profile. This is not
an Issue #414 failure and does not invalidate the focused product matrix or
exact Linux qualification.

## Impeccable UI audit

Implementation Integrity verdict: **Pass.** The support workflow remains a
coherent bench-instrument surface with explicit state, native form semantics,
visible consent gates, fixed public destinations, privacy-specific language,
and no deceptive confirmation.

| Dimension | Score | Key finding |
|---|---:|---|
| Accessibility | 4/4 | Clipboard recovery, focus, labels, status regions, and keyboard paths pass |
| Performance | 4/4 | Event-driven workflow has no observed layout-thrashing or blocking animation |
| Responsive design | 4/4 | Desktop and 390-pixel layouts preserve actions and distinctions without overflow |
| Theming | 4/4 | Light and dark workflow states remain readable and coherent |
| Implementation integrity | 4/4 | Product-specific privacy and state contract is preserved end to end |
| **Total** | **20/20** | **Excellent** |

Fresh active desktop, handoff mobile, reported-upload desktop/mobile, dark
reported-upload, upgrade desktop, loading mobile, and unavailable mobile renders
were generated. Representative light, dark, desktop, and mobile captures were
visually inspected and showed no obstruction, clipping, or horizontal overflow.

The detector reported six advisory design-system differences: one Zephyr hover
shade in the generated sidecar and five incumbent shell values introduced before
Issue #414 (one mixed alert color, three compact radii, and one mobile kicker
size). They are deliberate or pre-existing shell details, do not affect the
support workflow, and are not WCAG, task-completion, or closeout blockers.

## Remaining closure sequence

1. Integrate the application branch into WsprryPi `devel` under explicit merge
   authorization.
2. Integrate the operator-documentation branch into `Wsprry_Pi_Docs` `devel`
   under its separate repository boundary.
3. Verify both resulting remote `devel` tips and required CI.
4. Close Issue #414 only after those integrations are confirmed.

## Documentation Impact

This re-audit supersedes the Slice 48 blocker status. Operator documentation is
implemented and validated on its dedicated synchronized branch; only its
separate merge remains. No operator documentation, application UI behavior,
hardware configuration, service state, or external provider state was changed
by this re-audit.
