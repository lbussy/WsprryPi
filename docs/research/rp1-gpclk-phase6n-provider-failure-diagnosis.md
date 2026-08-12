# Phase 6N: immediate provider termination and false-success diagnosis

## Outcome

Both Phase 6M failures have confirmed and separate causes:

1. After reboot, `wspr5` loaded a stale boot-installed RP1 provider module that
   implements the legacy 48-byte submit ioctl. Current userspace submits the
   304-byte version 1 frame program, so the kernel rejects `SUBMIT` immediately
   with `ENOTTY`.
2. The RP1 execution backend returns that submission error as `ok=false` and
   `faulted=false`. The WSPR execution path checks only
   `!execute_result.ok && execute_result.faulted`; it therefore ignores the
   failed result, transitions to `COMPLETE`, emits a successful completion
   callback, and lets the direct CLI process exit zero.

No provider DMA transfer began, no live output was enabled, and no RF was
transmitted.

## Source and runtime identity

- Parent: `ad25d2a2744fc8984d7a919928213cfa12319a3c` on
  `codex/issue-399-rp1-gpclk`.
- Transmitter: `7c234796cf523657ea3c7d1806d3c6f70ee84ef2` on
  `codex/issue-399-rp1-gpclk-divider-planner`.
- Kernel: `6.18.44-v8-16k+ #3`.
- Installed provider source version: `C09AA574CBFF079D4B5A6FA`.
- Retained current provider build source version: `E2B807F9BED056FF0867C9B`.
- Patch and overlay hashes on `wspr5` matched the Mac repository.
- The retained kernel-tree provider source matched the committed provider
  source byte for byte.

## Provider reproduction

A temporary clock-disabled harness performed the provider operations directly
after a clean `live_output=N` module reload:

```text
elapsed_ns=9297 operation=open rc=0 errno=0 error=none
elapsed_ns=48370 operation=ACQUIRE rc=0 errno=0 error=none
elapsed_ns=51130 operation=SUBMIT rc=-1 errno=25 error=Inappropriate ioctl for device
elapsed_ns=56426 operation=RELEASE rc=0 errno=0 error=none
elapsed_ns=59370 operation=close rc=0 errno=0 error=none
```

The current userspace command is `0x4130b701`, encoding a `0x130` (304-byte)
program. Disassembly of the module loaded from `/lib/modules` showed that it
compares against `0x4030b701`, encoding the obsolete `0x30` (48-byte) program.
Disassembly of the retained current build at
`/home/pi/rpi-linux-phase6g/drivers/clk/rp1-gpclk-provider.ko` showed the
correct `0x4130b701` comparison.

The installed and retained modules also have different SHA-256 hashes and
source-version identifiers. Phase 6L ran the current temporarily loaded build,
but that build was not installed as the module selected at boot. The reboot
therefore restored the older UAPI implementation.

The failure is at ioctl command dispatch, before program validation, DMA
preparation, DMA submission, callback timing, cadence enforcement, or final
divider verification. There is no evidence that generation state, the DMA
channel, or clock-disabled tick pacing caused this reproduction.

## Scheduler reproduction

The scheduler was reproduced under `strace` with `live_output=N`:

```text
openat(..., "/dev/rp1-gpclk0", O_RDWR|O_CLOEXEC) = 6
ioctl(6, _IOC(_IOC_WRITE, 0xb7, 0, 0x10), ...) = 0
ioctl(6, _IOC(_IOC_WRITE, 0xb7, 0x1, 0x130), ...) = -1 ENOTTY
```

The application nevertheless reported:

```text
Started transmission: 14.097126 MHz.
Completed transmission: 0.011444 seconds.
scheduler_rc=0
```

`WsprRp1GpclkBackend::execute()` returns a default `ExecutionResult` with
`ok=false` and `faulted=false` when `prepare()` or `emitFrame()` fails. It
retains the provider error string but does not classify the result as faulted.
The WSPR controller then rejects only results for which both `ok` is false and
`faulted` is true. It consequently continues through its normal completion
state and callback for this submission failure.

This is not limited to `ENOTTY`: any non-faulted execution failure returned by
the RP1 backend at preparation or submission can be mislabeled as a successful
transmission.

## Smallest proposed corrections

### Provider deployment

Build the provider and KUnit modules from the committed Phase 6L kernel tree,
install the resulting current modules into the boot kernel's module tree, run
the normal module dependency update, and reboot. Verify after reboot that the
loaded module has the current source version and recognizes submit command
`0x4130b701`. Add a deployment check that compares the built artifact with the
module that will actually be loaded after reboot; a successful temporary
`insmod` is not persistent-install evidence.

### Scheduler failure propagation

Treat every `ExecutionResult` with `ok=false` as an execution failure,
regardless of the `faulted` classification. Preserve `faulted` as additional
severity/recovery information rather than using it as permission to ignore an
error. Propagate the error through a controlled transmitter failure state so
the scheduler does not emit `COMPLETE`, the direct CLI exits nonzero, and
managed service operation reports an actionable backend failure without an
uncaught transmit-thread exception.

## Required regression coverage

- RP1 backend preparation failure produces a failed execution result with its
  original error.
- RP1 provider `SUBMIT` failure, including `ENOTTY`, cannot produce a
  transmission-complete callback.
- A non-faulted `ok=false` execution result follows the controlled failure
  path.
- Direct CLI returns nonzero for an RP1 submission failure.
- Managed/service mode reports the failure and remains safely inhibited or
  idle according to its existing runtime contract.
- Cleanup after every failure releases the provider and restores GPIO4 and
  GPCLK counts.
- Post-install and post-reboot UAPI identity checks prove that userspace and
  the boot-selected module encode the same submit command and structure size.
- One clock-disabled production frame reaches `COMPLETE` inside the enforced
  cadence window before Phase 6M is attempted again.

## Cleanup and compatibility

All temporary source, binaries, and decompressed module artifacts were removed.
The run finished with `live_output=N`, GPIO4 input under the provider safe
2 mA state, GPCLK0 prepare and enable counts zero, and both
`wsprrypi.service` and `soapyremote-server.service` active. The parent and
transmitter worktrees on `wspr5` are clean and synchronized with origin.

The diagnosis changes no Pi 4-and-earlier GPIO behavior, Si5351 behavior, CW,
power selection, web UI, operator configuration, or operator documentation.

## Documentation impact

No operator documentation was changed. This phase produced engineering
diagnosis only. Developer documentation should later record the persistent
kernel-module installation and post-reboot UAPI identity check. Operator
documentation remains deferred until Pi 5 GPIO WSPR qualification passes.

Pi-side raw evidence and its SHA-256 manifest are retained at
`/home/pi/phase6n-evidence`.
