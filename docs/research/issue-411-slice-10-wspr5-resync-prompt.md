# Issue 411 Slice 10 execution prompt: wspr5 source resynchronization

## Objective

Safely fast-forward the clean primary WsprryPi checkout on `wspr5` from the
pre-integration commit to current `origin/devel`, verify that the monorepo
component conversion remains intact, and run the repository's normal
non-hardware validation on the Raspberry Pi without installing or operating
the application.

## Verified starting point

- Local and GitHub `devel` are synchronized at
  `6eadbc17d2771d885125f9a5e41029fffed3ffec`.
- That tip includes the complete Issue 411 chain through Slice 9 plus the two
  subsequent portable build-parallelism commits.
- `/home/pi/WsprryPi` on `wspr5` is clean on `devel` at
  `00f093c8523d2068740d0371526d2340d8d99379`.
- Its locally cached `origin/devel` is also at that old commit and therefore
  must be fetched before comparing or fast-forwarding.
- `.gitmodules` is absent, no nested `.git` files were found below the component
  paths, `wsprrypi.service` is inactive, and no process holds an I2C, gpiochip,
  or RP1 device handle.
- Issue 411 is open and must remain unchanged.

## Scope

1. Persist this prompt on a Slice 10 branch based on current `devel`.
2. Recheck the local and remote repository state before changing `wspr5`.
3. On `wspr5`, fetch `origin`, require a clean primary `devel` checkout, and
   prove that it can fast-forward to the expected GitHub `devel` commit.
4. Fast-forward only `/home/pi/WsprryPi` to that exact commit.
5. Verify these ten former submodule paths are ordinary tracked directories:
   `WsprryPi-UI`, `src/INI-Handler`, `src/LCBLog`, `src/Mailbox`,
   `src/MonitorFile`, `src/PPM-Manager`, `src/Signal-Handler`, `src/Singleton`,
   `src/WSPR-Transmitter`, and `src/WSPR-Reference`.
6. Inspect the relevant Make targets, then run the normal non-hardware
   validation from `src` with hardware access disabled and no sudo wrapper.
7. Recheck checkout cleanliness, service inactivity, and absence of open
   I2C/GPIO/RP1 device handles.
8. Persist a bounded result record, review it, commit it, and push only the
   Slice 10 branch if a documentation commit is warranted.

## Required invariants

- The source update is a strict fast-forward to the expected commit; no merge,
  rebase, reset, checkout overwrite, stash, clean, or history rewrite is used.
- Existing untracked or modified content stops the update rather than being
  removed or overwritten.
- Every former component path is parent-repository content with no gitlink or
  nested repository registration.
- Validation runs with `WSPRRYPI_DISABLE_HARDWARE_ACCESS=1` and `SUDO=`.
- No test may require a real I2C, GPIO, mailbox, MMIO, DMA, or RP1 device.
- No application binary is installed or substituted into the running system.

## Constraints and non-goals

- Do not run an installer or removal target.
- Do not start, stop, restart, enable, disable, or modify a service.
- Do not change system configuration, packages, permissions, or device state.
- Do not probe or open a physical I2C/GPIO/RP1 device.
- Do not generate a tone, start a transmission, or produce RF.
- Do not modify application, component, UI, installer, service, or operator
  documentation behavior in this slice; only prompt/result research records
  may be added locally.
- Do not update any secondary or qualification worktree on `wspr5`.
- Do not open a pull request or mutate Issue 411.
- Do not fast-forward GitHub `devel`; it is the source of truth for this slice.

## Adversarial review

Attempt to disprove that the Pi checkout is clean, that the update is a pure
fast-forward to the expected tip, that all ten component paths are ordinary
tracked directories, that the selected validation is genuinely hardware-free,
and that no build artifact remains in the checkout. Treat any unexpected dirty
state, branch movement, device access, service activity, or test dependency on
hardware as a stop condition. Correct only findings within this slice and
repeat affected checks.

## Exit criteria

- `/home/pi/WsprryPi` is clean on `devel` at the exact current
  `origin/devel` commit.
- All ten former submodule paths are verified as ordinary tracked directories.
- The normal non-hardware validation passes on `wspr5`, or any environment
  limitation is precisely recorded without weakening a test.
- The service remains inactive and no I2C/GPIO/RP1 device handle is open.
- No installation, runtime operation, hardware access, or RF occurs.
- The Slice 10 prompt/result record is committed and pushed on its matching
  branch if needed.
- Issue 411 remains open and otherwise unchanged.
