# TODO

- [ ] Documentation needs updates from/related to changes to command line parsing (including INI vs option precedence)
- [ ] See if we can use C++ 20 and .contains() (in arg parsing)
- [ ] Replace manual trimming – Use std::erase_if() (C++20) instead of manually erasing whitespace.
- [ ] Consider an external file for band to frequency lookups
- [ ] Note in docs that they may see:
    `2025-02-20 23:10:00 UTC [INFO ] Transmission started.`
    ... this is a floating point rounding issue.
- [ ] Add configurable transmission pin to transmission logic: ini.get_int_value("Common", "Transmit Pin"));
- [ ] Figure out how to add in DMA stuff for transmission
- [ ] Also change priority for transmissions
- [ ] Add wp_server command processing
- [ ] MA notes at: `https://github.com/fandahao17/Raspberry-Pi-DMA-Tutorial`
