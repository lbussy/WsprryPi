# Compile-time transmission backend selection

## Status

Research and design record only. No compile-time backend selection described in
this document is implemented.

This investigation was prompted by an Ubuntu 24.04 x86 user who wanted to use
the Si5351 backend through a regular Linux `/dev/i2c-N` adapter. The user did not
select the hardware-free simulated backend, but simulation would not have met
the intended purpose because it deliberately performs no I2C access.

## Conclusion

An x86 Linux build that includes the Si5351 backend and excludes Raspberry Pi
clock/GPIO transmission is technically feasible. The Si5351 device layer uses
the standard Linux userspace I2C interface and does not inherently depend on a
Raspberry Pi processor, GPIO clock, mailbox, MMIO, PWM, or DMA.

The current executable cannot be reduced to Si5351 merely by omitting one GPIO
source file. Its Makefile discovers and links nearly all production sources,
its backend factory contains unconditional constructors for every backend, and
`WsprTransmitter` initially constructs the legacy Raspberry Pi GPIO backend
before configuration selects the requested backend. A maintainable solution
therefore requires an explicit compile-time backend capability model.

## Reported Ubuntu failure

The first reported error was:

```text
main.cpp:97:22: error: ignoring return value of ‘ssize_t write(int, const void*, size_t)’
declared with attribute ‘warn_unused_result’ [-Werror=unused-result]
```

The relevant common shutdown code is:

```cpp
(void)::write(g_async_shutdown_pipe[1], &wake, sizeof(wake));
```

This error is unrelated to GPIO and is not proof of an x86 incompatibility.
Ubuntu's fortified C library declaration marks `write()` with
`warn_unused_result`, while the project promotes all warnings to errors through
the common `-Werror` build flag. GCC does not reliably treat a cast to `void` as
use of a value returned by a function carrying that attribute.

The production correction should consume the return value without adding an
unsafe operation to the signal-notification path. Assigning it to a local
`ssize_t` and intentionally ignoring that local value is sufficient for this
best-effort nonblocking wakeup. The signal path must not log, allocate, throw,
or perform a blocking retry. Temporarily removing `-Werror` or using
`-Wno-error=unused-result` can expose subsequent portability errors during
diagnosis, but neither is the desired source fix.

Primary references:

- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [glibc source fortification](https://sourceware.org/glibc/manual/latest/html_node/Source-Fortification.html)

## Confirmed Linux I2C compatibility

`Si5351Device` already uses a Linux adapter that:

1. opens `/dev/i2c-<configured bus>` with `O_RDWR`;
2. selects the configured slave address with `ioctl(I2C_SLAVE, ...)`;
3. performs checked `read()` and `write()` calls; and
4. closes the device through the same adapter boundary.

That is the standard Linux `i2c-dev` userspace contract. A registered native,
USB, PCIe, or other Linux I2C adapter can provide `/dev/i2c-N`; the Si5351 layer
does not need to know the transport beneath that device. Adapter numbers are
assigned dynamically and can change, so the operator must continue to select
the correct bus rather than assuming `/dev/i2c-1`.

Primary reference:

- [Linux userspace I2C device interface](https://www.kernel.org/doc/html/latest/i2c/dev-interface.html)

This establishes software-interface compatibility only. It does not prove that
a particular adapter supports the required transactions, voltage levels,
pull-ups, bus rate, Si5351 board, or transmitter hardware.

## Current compile and link coupling

The current parent Makefile:

- includes the `Mailbox` and complete `WSPR-Transmitter` source directories;
- recursively discovers nearly every C++ production source in those
  directories;
- always includes libgpiod detection, compile flags, and link libraries; and
- links the complete discovered object set into one application executable.

The runtime factory in `WsprTransmitter::createBackend()` contains constructors
for legacy Raspberry Pi clock/GPIO, RP1 GPCLK, Si5351, and simulated backends.
Those references require the corresponding implementations to be linked unless
the factory is also made compile-time aware.

In addition, the `WsprTransmitter` constructor currently selects
`RPI_CLOCK_GPIO` before CLI or INI parsing. Although this initial construction
does not by itself start RF or access hardware, it creates an unnecessary link
dependency and makes a physically unavailable backend the implicit initial
state.

## Proposed build capability model

Define independent build-time capabilities, for example:

```text
WSPRRYPI_BACKEND_RPI_GPIO
WSPRRYPI_BACKEND_RP1_GPCLK
WSPRRYPI_BACKEND_SI5351
WSPRRYPI_BACKEND_SIMULATED
```

A user-facing Make variable could map a concise selection to those generated
definitions and source groups:

```sh
make BACKENDS=si5351
make BACKENDS=si5351,simulated
```

The exact spelling is an implementation decision. The important contract is
that one authoritative build configuration controls source selection, factory
availability, validation, help/version reporting, and tests.

### Source selection

Replace backend source discovery with explicit source groups. An Si5351-only
profile should omit:

- the legacy Raspberry Pi clock/GPIO backend;
- the Raspberry Pi mailbox implementation;
- the RP1 GPCLK transmitter backend and Linux provider; and
- the simulated backend when it was not requested.

Shared request, planning, scheduling, controller, Si5351, logging, web, and
configuration sources remain present as needed by the application.

### Factory and initial state

Conditionally compile each backend factory arm. Selecting a backend that was
not compiled must fail closed with an explicit diagnostic, such as:

```text
Backend "gpio" is unavailable in this build. Compiled backends: si5351
```

There must be no automatic fallback to Si5351, simulated, or any other backend.

Prefer constructing `WsprTransmitter` without an active backend and selecting
one only after the CLI or configuration candidate has been parsed and
validated. This removes the implicit GPIO dependency and better represents the
application lifecycle. If an empty initial state proves too invasive, a
generated compiled default could be used, but it must remain explicit and
testable.

### Capability reporting

The compiled backend set should be visible through:

- `--help` and version output;
- a machine-readable status/version field used by integrations; and
- preferably a `--list-backends` command.

Configuration validation must distinguish an invalid backend name from a valid
backend that is unavailable in this build. Existing configuration that selects
an omitted backend must be rejected, not rewritten or normalized to an
available backend.

### Transmission GPIO versus ancillary GPIO

Excluding Raspberry Pi clock/GPIO transmission is not identical to removing all
GPIO support. The application also uses libgpiod for TX LED, amplifier control,
shutdown-button input, and band-selection outputs.

At least two useful profiles exist:

1. **No GPIO transmission backend:** omit Raspberry Pi clock/DMA and RP1 GPCLK,
   but retain optional ancillary libgpiod controls.
2. **Strict I2C-only:** omit transmission GPIO and all ancillary libgpiod
   features, dependencies, code, and configuration acceptance.

The strict profile most directly matches an ordinary x86 host whose only
physical transmitter interface is `/dev/i2c-N`. Ancillary GPIO configuration
in that profile must fail clearly rather than being silently ignored.

## Runtime privilege boundary

The application currently requires root for every physical backend and exempts
only explicit simulation. Linux deployments commonly grant access to a
specific `/dev/i2c-N` through device ownership, groups, or udev rules, so a
future Si5351-only deployment might not technically need full root privilege.

That is a separate security and operator-workflow decision. Compile-time
backend selection should not silently remove the current privilege gate. A
later change could replace it with backend-specific access checks and clear
diagnostics while leaving physical GPIO protections intact.

## Safety and non-goals

An Si5351-only build is still a physical transmitter build. It must retain:

- explicit backend selection;
- Si5351 startup quiescence;
- output parking and cleanup;
- cancellation and shutdown handling;
- configuration and frequency validation; and
- failure inhibition when the I2C device cannot be opened or controlled.

Compile-time exclusion does not qualify I2C electrical behavior, Si5351 output,
frequency accuracy, RF output, filtering, amplifier operation, installation,
service lifecycle, or a transmitter chain. The simulated backend remains the
canonical hardware-free development path and must continue to perform no I2C
access.

Dynamic backend plugins, automatic hardware discovery, and automatic fallback
are not required to solve this problem. Static build capabilities fit the
existing `ITransmissionBackend` architecture and give the linker a verifiable
boundary.

## Proposed validation

Implementation should be accepted only after the following evidence exists:

1. Ubuntu 24.04 x86 compilation with GCC 13 and the normal `-Werror` policy.
2. Builds for all supported backend combinations, including an intentional
   failure when no backend is enabled.
3. Link or symbol audits proving that a strict Si5351 executable contains no
   Raspberry Pi GPIO, RP1 provider, mailbox, MMIO, PWM, or DMA implementation.
4. CLI and configuration tests proving that omitted backend and ancillary GPIO
   selections fail closed without fallback or persistence changes.
5. Existing Si5351 fake-I2C planner, transition, startup-quiescence, controller,
   cancellation, and cleanup tests.
6. Existing simulated and Raspberry Pi backend regression suites in builds
   where those backends are enabled.
7. A file-access audit proving that an authorized Si5351-only run accesses the
   intended `/dev/i2c-N` and does not access `/dev/mem`, `/dev/vcio`, GPIO chip,
   RP1 GPCLK, mailbox, MMIO, PWM, or DMA paths.
8. Separately authorized validation of the intended adapter and Si5351 hardware.
9. Separately authorized RF and frequency qualification before making any
   physical-output claim.

## Documentation impact for a future implementation

This research record is not operator documentation. If backend-selectable
builds are implemented, the repository will need developer/build documentation
covering available profiles, dependencies, commands, capability reporting, and
test matrices. Operator documentation in the separate `Wsprry_Pi_Docs`
repository will require review if distributed packages can expose different
backend or ancillary-GPIO capabilities.
