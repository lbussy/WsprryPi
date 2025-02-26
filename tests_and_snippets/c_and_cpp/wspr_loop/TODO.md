# TODO

- [ ] Note in docs that they may see:
    `2025-02-20 23:10:00 UTC [INFO ] Transmission started.`
    ... this is a floating point rounding issue.
- [ ] [INFO ] TX will stop after: 1 iterations <- This functionality not yet implemented
- [ ] Add configurable transmission pin to transmission logic: ini.get_int_value("Common", "Transmit Pin"));
- [ ] Add in WSPR-Message
- [ ] Figure out how to add in DMA stuff for transmission
- [ ] Also change priority for transmissions
- [ ] Decide on transmission set vs iterations question
- [ ] Loop through frequencies at each period
- [ ] 