# Phase 6S: RP1 GPCLK implementation and deployment closeout

## Outcome

Issue 399 is implementation-complete for its qualified Phase 6R cell: Raspberry
Pi 5, RP1 GPCLK0, GPIO4, 20 m WSPR, and 2 mA drive. Phase 6S found no production
source defect requiring correction. It completed the missing developer
deployment contract and passed the applicable non-hardware regression and
build gates.

No GPIO state, GPCLK state, module, service, boot configuration, installed
binary, or RF output was changed in Phase 6S.

## Verified starting state

- Parent: `ac1eed9f87737715cce370e339ba2892251842bb` on
  `codex/issue-399-rp1-gpclk`.
- Transmitter: `d6eb8bb6568d612483b48c6ccf7181449cfaa06e` on
  `codex/issue-399-rp1-gpclk-divider-planner`.
- Mac and `wspr5.local` parent and transmitter branches were clean and matched
  their origin tracking branches.
- All recorded submodules were initialized and clean.

Before synchronization, retained Pi working files were proved byte-for-byte
identical to the committed Issue 399 files. Only those verified redundant files
were restored before both Pi branches were fast-forwarded.

## Acceptance audit

Implemented and source-tested:

- automatic RP1 selection for the Pi 5 GPIO route while preserving legacy Pi
  GPIO and Si5351 selection;
- canonical request, plan, and backend execution;
- pure 16.16 divider planning and bounded dithering;
- versioned provider UAPI with no userspace divider address;
- common-clock exclusive lease and provider-owned DMA target;
- exact 162-symbol finite submission;
- startup quiescence, cancellation, terminal cleanup, and repeat execution;
- lease-scoped generation reset and monotonicity;
- fail-closed provider errors and scheduler `FAILED` propagation; and
- GPIO band-policy enforcement.

Hardware-qualified before Phase 6S:

- common-clock and pinctrl ownership;
- clock-disabled provider, UAPI, DMA, STOP/drain, conflict, and cleanup paths;
- provider-owned minimum-drive output and restoration;
- full-frame cadence; and
- three independent Phase 6R WSPR decodes for the qualified cell.

Intentionally deferred or unqualified:

- GPIO20, other bands, and drive values above 2 mA;
- CW through the RP1 backend;
- an operator-facing RP1 drive selection workflow;
- absolute frequency or power calibration;
- regulatory spectral compliance;
- operator documentation; and
- Issue 400 simulation work.

The original Phase 5 delayed-work duration failure was superseded by the finite
provider/DMA lifecycle and later full-frame cadence evidence. The earlier SDR
offset remains a relative receiver/readback observation; Phase 6R used relative
translation and made no absolute calibration claim.

## Deployment contract

`tools/rp1_gpclk_provider/DEVELOPMENT.md` now records:

- supported BCM2712 Raspberry Pi OS 64-bit kernel scope;
- exact reference source revision and 16 KiB configuration requirements;
- complete developer dependencies;
- the Pi 5 full-processor build exception;
- source and static validation;
- side-by-side image, module, and overlay installation requirements;
- safe `live_output=N` default;
- post-reboot kernel, module, `srcversion`, device, and boot-selection checks;
- mismatch recovery and packaged-kernel rollback;
- lease-scoped generation and repeat-run semantics; and
- the exact RF qualification boundary.

Read-only inspection of `wspr5.local` found:

- kernel `6.18.44-v8-16k+`;
- four processors;
- provider `srcversion` `00435149E8EC6D24857F8C1`;
- KUnit `srcversion` `272C4EBBF19BBC0181ABB63`;
- provider and KUnit installed under the matching module tree;
- `live_output=N`;
- `/dev/rp1-gpclk0` mode `0600`, owned by root;
- persistent engineered kernel and overlay selection; and
- the documented Flex, Bison, M4, libfl-dev, GCC, and binutils dependencies.

## Validation

All compilation on `wspr5.local` used `make -j4`, the explicitly permitted
Pi 5 `nproc` value.

Passed:

- portable provider-core test;
- kernel static-contract test;
- RP1 planner, lifecycle, transition, production-backend, Linux-provider, and
  scheduler-backend tests;
- Si5351 planner and fake-I2C transition tests;
- startup-quiesce and fake GPIO/Si5351 qualification tests;
- Si5351 minimum-drive fake qualification test;
- GPIO band-policy test;
- parent startup-quiesce test;
- QRSS/WSPR execution regression;
- WSPR tone regression;
- complete parent semantics suite, including scheduler false-success and
  managed provider-failure coverage;
- parent debug build; and
- parent release build.

The standalone transmitter debug target was inspected but is not a supported
independent full build in this checkout: it requires the parent-provided
`ini_file.hpp`. Its focused transmitter tests passed, and the transmitter was
compiled successfully through both parent debug and release builds.

An initial orchestration mistake left two duplicate parent `make -j4` commands
running against the same build tree. Only those agent-started processes were
stopped. One controlled build/test batch was then run and exited zero. This did
not change source, services, modules, GPIO, or RF state.

## Compatibility and documentation impact

No production or transmitter-submodule source changed in Phase 6S. Existing Pi
4-and-earlier GPIO behavior, Si5351 behavior, configuration compatibility,
power semantics, and fail-closed GPIO band policy remain unchanged. RP1 does not
silently fall back when its provider is unavailable or incompatible.

Core developer documentation was updated. The separate operator-documentation
repository was not inspected or modified. Operator documentation may later
describe only the specifically qualified Phase 6R cell after its installation,
configuration, power-selection, and support workflow are separately reviewed.

## Remaining work

CW implementation/qualification and the operator RP1 power-selection workflow
remain separate future work. No Phase 6S evidence supports expanding the
qualified cell or making absolute RF, power, or regulatory claims.
