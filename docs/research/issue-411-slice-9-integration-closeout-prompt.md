# Issue 411 Slice 9 execution prompt: integration and closeout validation

## Objective

Integrate the complete, linear Issue 411 implementation chain into `devel`
after one final adversarial review and cross-platform non-hardware validation.
Publish the resulting `devel` commit without changing Issue 411 state or
claiming unperformed RF qualification.

## Verified starting point

- Start from synchronized Slice 8 commit
  `b7e326769bb785f1e3e3351bd079f503bc3f1462`.
- The Slice 8 tip is exactly eight commits ahead of `origin/devel` and has no
  divergent commits.
- The chain contains the fortified-write portability fix, backend capability
  foundation, profile activation, capability reporting, strict I2C profile,
  GPIO-free privilege policy, isolated file-access audit, and bounded `wspr5`
  startup-quiescence evidence.
- `wspr5` primary `devel` has been reconciled through the submodule-to-monorepo
  migration and is clean and synchronized with the pre-integration
  `origin/devel`.
- Issue 411 is open and must remain open.

## Scope

1. Persist this execution prompt on a final Slice 9 branch based on Slice 8.
2. Review the complete `origin/devel..Slice 9` commit series and diff.
3. Confirm every changed file is attributable to Issue 411 and no UI,
   installation, service, or unrelated component behavior was added.
4. Run the strict Ubuntu 24.04 x86_64 GCC 13 build without libgpiod development
   packages, its factory/profile regressions, non-root privilege check, and
   syscall file-access audit.
5. Run capability generator and Make integration regressions.
6. Run the default-profile factory and representative semantics suite in an
   isolated Debian environment with hardware access disabled.
7. Parse workflow YAML and run final staged/unstaged whitespace checks.
8. Confirm `wspr5` remains clean, service-inactive, and free of I2C/GPIO/RP1
   device handles. Do not rerun live hardware qualification.
9. Commit and push the final prompt/result record on Slice 9.
10. Fast-forward local `devel` to the validated Slice 9 tip and push only
    `origin/devel`.

## Required invariants

- The default build remains the complete four-backend executable with
  ancillary GPIO enabled.
- `BACKENDS=si5351 ANCILLARY_GPIO=0` remains the canonical strict profile.
- Omitted backend selection fails closed with no automatic fallback.
- A GPIO-capable executable retains its pre-parse root requirement.
- A GPIO-free Si5351 executable may rely on ordinary `/dev/i2c-N` permissions.
- Simulation remains explicitly selected, hardware-free, and non-RF.
- The strict executable links no libgpiod and contains no GPIO, mailbox, MMIO,
  DMA, or RP1 implementation path.
- No validation invocation accesses a real device or generates RF.

## Constraints and non-goals

- Do not modify application, component, UI, installer, service, or operator
  documentation behavior during closeout. Only prompt/result documentation may
  be added if the implementation diff is already complete.
- Do not install, restart, enable, disable, or replace anything on `wspr5`.
- Do not access GPIO, MMIO, mailbox, DMA, RP1 GPCLK, I2C hardware, transmitter
  hardware, or RF.
- Do not rebase, squash, amend, force-push, or rewrite the reviewed chain.
- Do not open a pull request.
- Do not close, comment on, label, assign, or otherwise mutate Issue 411.

## Adversarial review

Attempt to disprove that the branch is a clean linear descendant of current
`origin/devel`; that every reduced profile fails closed; that strict builds
exclude all GPIO capability and dependency; that mixed/default builds retain
the root boundary; that the non-root audit cannot touch a real device; that the
`wspr5` evidence is accurately bounded; and that no generated artifact or
unrelated change is staged. Correct every actionable finding and repeat the
affected checks before integration.

## Exit criteria

- All required cross-platform and hardware-disabled validation passes.
- The final complete diff is reviewed and attributable.
- Slice 9 is committed and pushed to its matching origin branch.
- `devel` is fast-forwarded without conflict to the exact validated Slice 9
  commit and pushed to `origin/devel`.
- Local `devel`, `origin/devel`, and the validated Slice 9 tip are identical.
- `wspr5` remains on the pre-integration `devel` until separately resynchronized;
  no installation or runtime change is implied by repository integration.
- Issue 411 remains open and otherwise unchanged.
