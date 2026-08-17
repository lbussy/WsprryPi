# Issue 411 Slice 6 execution prompt: GPIO-free Si5351 privilege policy

## Objective

Implement the next bounded slice of Issue 411 by allowing an explicitly
selected Si5351 backend to run without an unconditional root requirement when
the executable contains no physical GPIO capability. Let the operating
system's ordinary permissions on the selected `/dev/i2c-N` device remain the
authoritative access control.

## Verified starting point

- Start from the clean, synchronized
  `codex/issue-411-slice-5-strict-i2c` result.
- `make BACKENDS=si5351 ANCILLARY_GPIO=0` produces the canonical strict I2C
  executable without Raspberry Pi transmission backends or ancillary GPIO.
- The application currently rejects every non-root physical-backend process
  before full command-line and configuration parsing.
- Explicit simulation is already exempt from the root requirement.
- Linux can grant a user access to a particular I2C character device through
  ordinary device ownership, group membership, or administrator-managed udev
  policy.

## Required behavior

1. Define one testable privilege-policy function over the authoritative
   selected transmission backend and compile-time capabilities.
2. Continue to require root for `gpio` and for any executable that contains a
   Raspberry Pi transmission backend or ancillary GPIO capability.
3. Continue to allow explicit `simulated` operation without root.
4. Allow `si5351` without root only in a GPIO-free executable: neither
   Raspberry Pi transmission backend is compiled and ancillary GPIO is
   disabled. The simulated backend may also be present.
5. Retain a pre-parse root safeguard for every GPIO-capable executable so
   parsing cannot reach physical-backend validation first. Only a GPIO-free
   executable may defer the physical-backend decision until full argument and
   configuration parsing, where the final selected backend controls it.
6. Do not preflight an I2C path with `access(2)`. The real open/ioctl path and
   kernel device permissions remain authoritative, avoiding a time-of-check to
   time-of-use split.
7. Preserve clear failures: GPIO-capable physical builds retain the root/sudo
   diagnostic, while a GPIO-free Si5351 build proceeds to normal configuration
   validation and later reports its actual I2C open/ioctl error if access is
   unavailable.
8. Add focused non-hardware regression coverage for every policy branch and
   for the strict-profile command-line ordering.
9. Update the durable research record to distinguish implemented policy from
   administrator-managed device permissions and unperformed hardware
   qualification.

## Constraints and non-goals

- Do not change Issue 411 status.
- Do not add or edit udev rules, groups, ownership, modes, capabilities,
  privilege escalation/drop, `sudo`, installation, or service configuration.
- Do not weaken the root requirement of any executable capable of physical
  GPIO.
- Do not auto-select a backend and do not fall back after a backend or device
  failure.
- Do not access `/dev/i2c-N`, GPIO, MMIO, mailbox, DMA, transmitter hardware,
  services, or RF during validation.
- Do not broaden this slice into operator documentation or UI work.

## Validation and evidence

- Run whitespace checks on staged and unstaged changes.
- Build and run the backend-profile factory test for the default profile and
  for `BACKENDS=si5351 ANCILLARY_GPIO=0`.
- Run the strict I2C profile regression without an I2C-valid invocation; use
  an ancillary-GPIO rejection to prove a non-root process passed the former
  early root gate without reaching device access.
- Run the established capability regression tests and a representative
  default-profile non-hardware regression set.
- Inspect binaries only where needed to preserve the strict no-libgpiod
  contract.
- Record that source and simulated evidence does not qualify physical I2C,
  Si5351 output, timing, installation, services, GPIO, or RF.

## Adversarial review

After implementation, independently inspect the complete diff and ask:

- Can a GPIO-capable executable reach backend validation or avoid the root
  gate by selecting Si5351?
- Can an omitted or malformed backend accidentally receive the exemption?
- Does configuration-file selection use the same final policy as direct CLI
  selection?
- Did policy evaluation introduce any I2C or GPIO side effect?
- Do tests prove compile-time capability combinations rather than only the
  current developer build?
- Did any change weaken explicit selection, unavailable-backend diagnostics,
  or the strict ancillary-GPIO rejection?

Correct every actionable finding and repeat the focused checks.

## Exit criteria

- The bounded privilege policy and focused tests are implemented.
- GPIO-capable builds demonstrably retain the root requirement.
- A GPIO-free Si5351 build is no longer rejected solely for being non-root.
- The research record accurately states the administrator and hardware
  qualification boundaries.
- Only attributable files are committed and the current branch is pushed to
  its matching origin branch; no pull request is opened and Issue 411 remains
  open and otherwise untouched.
