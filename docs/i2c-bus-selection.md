# I2C bus selection

The Si5351 I2C bus selector lists adapters exposed by the running host through
`/sys/class/i2c-dev/i2c-N` with a corresponding character device `/dev/i2c-N`.
Entries are sorted by bus number and labeled with the adapter name when readable.
This bus inventory reads filesystem metadata only. Address discovery is a
separate, bounded operation on the selected bus: WsprryPi checks only addresses
`0x60` through `0x6F` using the same non-configuring register read used by the
Si5351 readiness check. It does not write clock configuration or enable an
output. A responding device is described as register-compatible because the
Si5351 does not provide a definitive silicon-identification register; operators
must still verify the attached hardware.

The public configuration's read-only `Platform` section includes `I2C Buses`
(an array of objects with integer `Number` and string `Name`) and
`I2C Bus Discovery Error` (empty on successful discovery). These values are
transient host information, not persisted settings or client authority.
For the configured bus, `Si5351 Address Bus`, `Si5351 Addresses`, and
`Si5351 Address Discovery Error` publish the corresponding transient address
inventory. The private `GET /config/si5351-addresses?bus=N` route refreshes that
inventory when the operator selects another listed bus.

The selector refreshes when configuration is loaded. Reload after enabling,
adding, or removing adapters. A saved bus that disappears remains visible as
unavailable; it is never silently replaced. An empty inventory disables the
selector and explains that no buses are available. Discovery failure and missing
inventory information have separate messages. A replacement requires an explicit
selection. The existing integer `Si5351.I2C Bus` format remains unchanged.

The address control lists only responding addresses within `0x60` through
`0x6F`. It preserves the configured address only while that address is present
on the selected bus. A missing configured address is explained but is not added
as a selectable option, and no address is selected automatically as a fallback.
Loading, no-device, stale-response, controller-error, and unsupported-build
states leave the control unavailable. Address values continue to persist as
uppercase `0x`-prefixed strings.

New bus numbers submitted through the web configuration API, including changes
to an inactive Si5351 setting, are checked against fresh host metadata. Switching
to Si5351 also checks its bus. An unchanged unavailable bus does not prevent
recovery to a different backend. Existing startup and transmission readiness
checks still govern actual device use, including CLI and INI configuration.
Changed address selections and switches to the Si5351 backend are checked
against a fresh inventory for the selected bus. The server accepts Si5351
addresses only from `0x60` through `0x6F`; it does not trust the browser's
transient inventory. An unchanged unavailable address does not prevent recovery
by switching to another backend. A listed adapter or register-compatible
response is not proof of external header routing, exact silicon identity, or
safe RF operation.

## Hardware-free validation

From `src`, run `make i2c-bus-inventory-test SUDO=` for temporary filesystem
fixtures. The `i2c-bus-selection-test` target exercises rejection of a forged
inventory and nonexistent selection without changing configuration. Both targets
are included in the full and portable semantics profiles. Choose the host's
profile as documented in `AGENTS.md`.

From `WsprryPi-UI`, run `npm test` and
`node tests/conditional_transmit_gpio_integration_test.js`. The browser suite
checks available, missing, empty, failed, and absent inventory states; explicit
replacement; preservation in payloads; dependent address loading, empty,
failure, and stale states; removal of the old fallback; and backend-toggle
behavior.

Operator-documentation follow-up belongs in the sibling documentation
repository's `docs/User_Interface/Setup/Transmitter/index.md`, under **I2C Bus**.
Its fixed bus-1 guidance should describe detected choices, unavailable saved
values, reload behavior, address choices from `0x60` through `0x6F`, and the
distinction between a compatible register response and verified device identity.
That repository is a separate change boundary.
