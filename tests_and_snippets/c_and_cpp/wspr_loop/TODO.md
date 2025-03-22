# TODO

- [ ] Documentation needs updates from/related to changes to command line parsing (including INI vs option precedence)
- [ ] See if we can use C++ 20 and .contains() (in arg parsing)
- [ ] Replace manual trimming – Use std::erase_if() (C++20) instead of manually erasing whitespace.
- [ ] Note in docs that they may see:
    `2025-02-20 23:10:00 UTC [INFO ] Transmission started.`
    ... this is a floating point rounding issue.
- [ ] Figure out how to add in DMA stuff for transmission
- [ ] Also change priority for transmissions
- [ ] DMA notes at: `https://github.com/fandahao17/Raspberry-Pi-DMA-Tutorial`
- [ ] May not need:
  - jq
  - libraspberrypi-dev
  - raspberrypi-kernel-headers
  - ntpd

    ``` bash
    readonly APT_PACKAGES=(
      "jq"
      "git"
      "apache2"
      "php"
      "libraspberrypi-dev"
      "raspberrypi-kernel-headers"
      "ntp"
      "gpiod"
      "libgpiod-dev"
    )
    ```

- [ ] Aded chrony `sudo apt install chrony -y`
- [ ] Make threads run as background priority
- [ ] Review if we do or can do WSPR-15
