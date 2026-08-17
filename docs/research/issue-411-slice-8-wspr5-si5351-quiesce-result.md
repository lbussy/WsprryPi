# Issue 411 Slice 8: wspr5 Si5351 startup-quiescence result

## Result

Passed on 2026-08-17. `wspr5` provided non-root access to its attached Si5351
through `/dev/i2c-1`, address `0x60`. The bounded qualification confirmed that
the device was already output-disabled and that two repeated startup-quiescence
operations left register 3 at `0xFF`.

This qualifies Linux device access and the Si5351 startup output-disable
operation on this host and attached device only. It does not qualify frequency
programming, timing, spectral behavior, RF output, the complete transmitter
chain, or the original end user's x86 I2C adapter.

## Target state

- Host: `wspr5`
- Kernel: `6.18.34+rpt-rpi-2712`, AArch64
- User: `pi`, non-root, member of group `i2c`
- Device: `/dev/i2c-1`, owned `root:i2c`, mode `0660`
- Adapter identity: `Synopsys DesignWare I2C adapter`
- Si5351 address: `0x60`
- Retained configuration: 27 MHz external TCXO
- WsprryPi service: inactive before and after the qualification
- Open I2C descriptors: none before and after the qualification

The existing `/home/pi/WsprryPi` checkout was clean as reported by Git but
contained preserved untracked UI content that blocked its 43-commit
fast-forward. Nothing was deleted, moved, stashed, or overwritten. Remote refs
were fetched, and qualification used a separate clean detached worktree at
`/home/pi/WsprryPi-issue411`, synchronized to Slice 7 commit
`a18099cfd2e307a3c970ec7f0c0dc3ae89235677`.

## Preconditions and build evidence

The purpose-built fake qualification test passed before live access:

```text
si5351_startup_quiesce_qualification_test passed
```

The live executable was built without running it as part of the Make target:

```text
Built live qualification executable; it is not run by this target.
```

The parent strict ARM64 profile then built successfully with:

```sh
make -j2 release backend-profile-factory-test \
  BACKENDS=si5351 ANCILLARY_GPIO=0 SUDO=
```

The existing strict-profile regression passed and reported only `si5351` as
compiled, with ancillary GPIO disabled and no libgpiod link/symbol dependency.

## Live invocation and evidence

The only live command was:

```sh
./build/bin/si5351_startup_quiesce_qualification \
  --device /dev/i2c-1 \
  --address 0x60 \
  --count 2 \
  --i-understand-this-accesses-live-si5351-hardware
```

Observed register state:

```text
register3-before=0xff register3-after-first=0xff register3-after-second=0xff
first-quiesce=success
second-quiesce=success
Qualification passed.
```

Audited device-operation trace:

```text
open /dev/i2c-1
select 0x60
select-register 03
read 03
close
open /dev/i2c-1
select 0x60
write 03 ff
close
open /dev/i2c-1
select 0x60
select-register 03
read 03
close
open /dev/i2c-1
select 0x60
write 03 ff
close
open /dev/i2c-1
select 0x60
select-register 03
read 03
close
```

No frequency, PLL, multisynth, clock-control, reference, or drive-strength
register was written. No carrier or transmission mode was requested. No
service, installed binary, configuration, device permission, GPIO, MMIO,
mailbox, DMA, or RP1 GPCLK operation was performed.

## Remaining qualification

Frequency accuracy and RF behavior remain separately authorized work. Testing
the original Ubuntu x86 scenario also still requires the end user's particular
adapter, driver, wiring, and Si5351 hardware; the `wspr5` result cannot qualify
that external adapter.
