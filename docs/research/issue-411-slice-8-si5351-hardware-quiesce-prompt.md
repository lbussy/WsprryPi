# Issue 411 Slice 8 execution prompt: wspr5 Si5351 hardware quiescence

## Objective

Validate the strict Si5351 profile on `wspr5` against its attached Si5351 using
only the purpose-built startup-quiescence operation. Establish non-root Linux
I2C access, exact device/address selection, repeatable output-disable behavior,
and terminal closure without programming a frequency or initiating RF.

## Verified target and starting point

- Target: `wspr5`, Raspberry Pi 5, AArch64 Debian.
- Resolve the current address rather than relying on a saved address.
- Begin by fetching remote refs and attempting a fast-forward-only
  synchronization of the existing checkout.
- Preserve every tracked, ignored, and untracked target file. If the existing
  checkout cannot fast-forward without overwriting files, stop that update and
  create a separate clean worktree at
  `origin/codex/issue-411-slice-7-file-access-audit`.
- Expected I2C device: `/dev/i2c-1`.
- Expected Si5351 address: `0x60`.
- Retained reference configuration: 27 MHz external TCXO. This qualification
  does not program or validate that reference.
- User `pi` must be a member of the device's owning `i2c` group.

## Preconditions

1. Confirm the WsprryPi service is inactive and do not change its state.
2. Confirm no process has an open `/dev/i2c-*` descriptor.
3. Confirm `/dev/i2c-1` exists and record its kernel adapter identity.
4. Build and run the fake startup-quiescence qualification test before the
   live executable.
5. Use only the checked-in purpose-built qualification executable and its
   mandatory live-hardware acknowledgement.

## Authorized live operation

Run exactly one bounded qualification invocation:

```sh
./build/bin/si5351_startup_quiesce_qualification \
  --device /dev/i2c-1 \
  --address 0x60 \
  --count 2 \
  --i-understand-this-accesses-live-si5351-hardware
```

The tool may:

- open and close `/dev/i2c-1`;
- select slave address `0x60`;
- read Si5351 register 3 before and after each operation; and
- perform exactly two register-3 writes of `0xFF`, disabling all outputs.

It may not write any other register or value.

## Required validation

- Both quiescence calls succeed.
- Register 3 reads `0xFF` after each call.
- The audited operation trace contains exactly the expected open, select,
  register-3 read/write, and close sequence.
- Build `BACKENDS=si5351 ANCILLARY_GPIO=0` on `wspr5` and run the strict
  profile/factory checks without live device invocation.
- Afterward, confirm the service remains inactive, no I2C descriptor remains
  open, the synchronized source worktree has no tracked changes, and no
  installation occurred.

## Constraints and non-goals

- Do not generate a test tone, carrier, WSPR frame, QRSS, FSKCW, or DFCW.
- Do not program PLL, multisynth, clock-control, frequency, drive-strength, or
  reference registers.
- Do not start, stop, enable, disable, or restart a service.
- Do not install or replace the running application.
- Do not change I2C, udev, group, ownership, permission, boot, or system
  configuration.
- Do not access GPIO, MMIO, mailbox, DMA, or RP1 GPCLK.
- Do not claim frequency accuracy, spectral quality, RF output, adapter
  portability beyond this host, or qualification of the original x86 user's
  adapter.
- Do not change Issue 411 state or open a pull request.

## Adversarial review

Verify that the live trace contains no operation beyond register-3 reads and
two `03 ff` writes; that the result was obtained as non-root user `pi`; that no
service or installed binary was involved; that output-disable was already the
initial state and remained the terminal state; and that documentation does not
conflate this result with RF or frequency qualification. Correct every
actionable documentation or evidence issue before publication.

## Exit criteria

- The bounded hardware-access and startup-quiescence contract passes on
  `wspr5`.
- The strict ARM64 profile builds and its non-hardware checks pass.
- The target remains inactive, output-disabled, and free of open I2C handles.
- Evidence and limitations are persisted in the repository, committed on the
  Slice 8 branch with `Related to #411`, and pushed to its matching origin
  branch while Issue 411 remains open.
