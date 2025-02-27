# TODO

- [ ] Add DEBUG flag in config?
- [ ] Note in docs that they may see:
    `2025-02-20 23:10:00 UTC [INFO ] Transmission started.`
    ... this is a floating point rounding issue.
- [ ] Add configurable transmission pin to transmission logic: ini.get_int_value("Common", "Transmit Pin"));
- [ ] Figure out how to add in DMA stuff for transmission
- [ ] Also change priority for transmissions
- [ ] Need a singleton
- [ ] Maybe skip early if next/last transmission is a 0.0
