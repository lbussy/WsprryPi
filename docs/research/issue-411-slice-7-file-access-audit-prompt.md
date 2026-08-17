# Issue 411 Slice 7 execution prompt: strict-profile file-access audit

## Objective

Close the remaining software-only validation gap in the Issue 411 research
record by proving that a non-root strict Si5351 executable attempts only its
explicitly selected Linux I2C device and does not attempt Raspberry Pi GPIO,
mailbox, MMIO, or RP1 device paths.

## Verified starting point

- Start from synchronized Slice 6 commit
  `fb62a9f46fcdf77add2e84327c7bf521b87028b5`.
- `make BACKENDS=si5351 ANCILLARY_GPIO=0` excludes both Raspberry Pi
  transmission implementations, Mailbox, libgpiod-backed ancillary controls,
  and their link dependencies.
- The strict executable permits non-root Si5351 operation and relies on normal
  kernel permissions for `/dev/i2c-N`.
- The research record still lists a file-access audit as unperformed.

## Required behavior

1. Add a focused Linux syscall-trace regression for the strict executable.
2. Run the executable as a non-root account in an isolated CI container with
   no device passthrough.
3. Select a deliberately nonexistent, distinctive non-negative I2C bus number
   so the process can only attempt an open and cannot reach an adapter, slave,
   or transmitter.
4. Assert that the trace contains the exact selected `/dev/i2c-N` path and no
   other I2C device path.
5. Assert that the trace contains no `/dev/mem`, `/dev/gpiomem`, `/dev/vcio`,
   `/dev/gpiochip*`, `/dev/rp1-gpclk*`, or RP1 platform resource path.
6. Assert that the process fails with its normal selected-I2C-path diagnostic,
   not the legacy root requirement.
7. Integrate the audit only into the strict Ubuntu non-hardware job and install
   only the tracing dependency needed there.
8. Update the durable research record to distinguish an isolated path audit
   from actual device, ioctl, electrical, timing, frequency, or RF validation.

## Constraints and non-goals

- Do not pass any host or physical device into the container.
- Do not create, mount, emulate, chmod, chown, or otherwise modify a device
  node.
- Do not run an I2C ioctl, read, or write; the nonexistent path must fail at
  `open(2)`.
- Do not access GPIO, MMIO, mailbox, DMA, RP1, transmitter hardware, services,
  installation, or RF.
- Do not change backend selection, privilege policy, device implementation,
  configuration semantics, UI, packaging, or operator documentation.
- Do not add udev rules, group policy, capabilities, or privilege management.
- Do not open a pull request or change Issue 411 state.

## Validation and evidence

- Parse the modified workflow YAML.
- Build `BACKENDS=si5351 ANCILLARY_GPIO=0` without libgpiod development files.
- Run the existing strict-profile tests as a non-root account.
- Run the new syscall audit as the same non-root account.
- Run the audit script's shell syntax check and focused failure-path
  self-review.
- Run capability generator/Make regressions and final diff whitespace checks.
- Preserve the trace only as ephemeral CI/test evidence; do not commit runtime
  artifacts.

## Adversarial review

Attempt to disprove each claim:

- Could the chosen bus accidentally exist or be passed through?
- Could a loose grep accept another I2C bus or miss a forbidden path?
- Could the test pass without an exact `O_RDWR|O_CLOEXEC` open failing with
  `ENOENT`?
- Could root execution hide a privilege-policy regression?
- Could the invocation proceed beyond `open(2)` to an ioctl or data transfer?
- Does process/library startup noise get confused with transmitter access?
- Does the documentation overstate what a failed-open trace qualifies?

Correct every actionable finding and rerun the focused checks.

## Exit criteria

- The strict Ubuntu job automatically proves the exact attempted I2C path and
  absence of the enumerated physical GPIO/mailbox/MMIO/RP1 paths.
- The invocation is non-root, isolated, and fails at the nonexistent device
  open without hardware access.
- The research record marks only software file-access audit item 8 complete;
  physical adapter/Si5351 and RF qualification remain explicitly outstanding.
- Only attributable changes are committed and the current branch is pushed to
  its matching origin branch with neutral `Related to #411` wording.
