# TODO

- [ ] Documentation needs updates from/related to changes to command line parsing (including INI vs option precedence)
- [ ] Note in docs that they may see:
    `2025-02-20 23:10:00 UTC [INFO ] Transmission started.`
    ... this is a floating point rounding issue.
- [ ] Incorporate new WsprTransmitter class
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
      "chrony"
      "gpiod"
      "libgpiod-dev"
    )
    ```

- [ ] Added chrony `sudo apt install chrony -y`
- [ ] Review if we do or can do WSPR-15
- [ ] Create new REST-based web page
- [ ] Add websockets to web page
