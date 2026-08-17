# Issue 411 Slice 12 execution prompt: candidate publication

## Objective

Integrate the validated strict Si5351 install-path candidate into `devel`, prove
the published commit with the repository's actual GitHub Actions workflow, and
post exact Ubuntu x86 build/install instructions to Issue 411 without closing
or otherwise changing the issue's lifecycle.

## Verified starting point

- Slice 11 commit `e138a71e1e5de368b1641e18ecb06b3558467d69` is clean,
  pushed, and exactly one commit ahead of `origin/devel`.
- Slice 11 passed Ubuntu 24.04 x86/GCC 13 and native Debian non-hardware
  validation, including its binary-only disposable-prefix install regression.
- `origin/devel` is at Slice 10 commit
  `dd041d8eb48bced8a49cdb9f8c62283fe4057a97`.
- Issue 411 is open. It says a candidate is forthcoming but does not yet contain
  the final strict-profile build/install instructions.
- The original tester does not yet have the CP2112 adapter; hardware validation
  must remain pending.

## Scope

1. Persist this prompt on a Slice 12 branch based on Slice 11.
2. Recheck repository cleanliness, exact ancestry, remote synchronization, and
   Issue 411 state.
3. Commit and push the Slice 12 prompt branch.
4. Fast-forward local `devel` to the exact Slice 12 prompt commit and push only
   `origin/devel`.
5. Locate the resulting `Debian non-hardware validation` GitHub Actions run for
   that exact `devel` commit and wait for completion.
6. Treat every failing or cancelled required job as a blocker. Inspect logs,
   correct actionable product findings on the Slice 12 branch, revalidate, and
   repeat publication if necessary.
7. After CI passes, post one concise Issue 411 comment containing the integrated
   commit, dependency command, source synchronization, strict build,
   binary-only install, capability checks, and the remaining hardware boundary.
8. Confirm the issue remains open and the posted comment contains no closing
   keyword or unsupported qualification claim.
9. Persist a Slice 12 result record with exact commits, CI evidence, issue
   comment URL, and remaining work; commit and push it, fast-forward `devel`,
   and verify the documentation-only follow-up workflow.

## Tester instructions contract

The Issue 411 comment must provide an equivalent of this Ubuntu workflow,
updated with the actual integrated commit:

```sh
sudo apt-get update
sudo apt-get install -y \
  build-essential git libssl-dev libsystemd-dev pkg-config

git clone --branch devel https://github.com/WsprryPi/WsprryPi.git
cd WsprryPi/src

make -j"$(nproc)" release \
  BACKENDS=si5351 ANCILLARY_GPIO=0 SUDO=
make install-binary \
  BACKENDS=si5351 ANCILLARY_GPIO=0 PREFIX=/usr/local/bin

/usr/local/bin/wsprrypi --list-backends
/usr/local/bin/wsprrypi --version
```

For an existing checkout, instruct the tester to preserve local changes and use
`git switch devel` plus `git pull --ff-only`; do not recommend reset, clean,
stash, or overwriting local work.

Expected capability evidence is compiled backends `si5351` with ancillary GPIO
disabled. The comment must explicitly say that installation does not start a
service and that live adapter/Si5351/RF commands will follow only after the
tester has the hardware and confirms the exact bus, address, wiring, output
path, duration, and stopping procedure.

## Required invariants

- Integration is a strict fast-forward with no merge commit, rebase, squash,
  amend, force-push, or history rewrite.
- `devel`, `origin/devel`, and the published Slice 12 tip are identical after
  each publication step.
- The existing full Raspberry Pi installer remains the normal product path;
  the Issue 411 instructions describe the explicit experimental Ubuntu
  binary-only path.
- CI evidence must belong to the exact published commit, not an earlier branch
  or local run.
- Issue 411 remains open and otherwise unchanged except for the one authorized
  instructional comment.

## Constraints and non-goals

- Do not install anything on the Mac, a Raspberry Pi, or the tester's host.
- Do not operate services, applications, I2C, GPIO, mailbox, MMIO, DMA, RP1,
  Si5351 hardware, a test tone, a transmission, or RF.
- Do not update `wspr5`; it is not needed for candidate publication.
- Do not open a pull request.
- Do not close, label, assign, milestone, edit, or otherwise mutate Issue 411.
- Do not claim CP2112 driver, USB adapter, electrical, frequency, or RF
  qualification.
- Do not modify the separate operator-documentation repository in this slice.

## Adversarial review

Attempt to disprove that the candidate is the exact validated descendant of
current `devel`; that Actions ran against the published commit; that every
required job passed; that the tester commands select and install only the
strict profile; that no command starts a service; and that the issue comment
cannot be read as hardware qualification or issue closure. Correct actionable
findings and repeat affected checks.

## Exit criteria

- Slice 11 and the Slice 12 records are fast-forwarded into `devel` and pushed.
- The exact final `devel` commit has a successful `Debian non-hardware
  validation` workflow run.
- Issue 411 contains the verified build/install instructions and remains open.
- The issue comment clearly defers CP2112 and physical Si5351 validation until
  the adapter arrives.
- Local and remote branch state is clean and synchronized.
- No installation, service, hardware, or RF operation occurred.
