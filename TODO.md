# TODO

## C++ Code

- [ ] Right-size logging

## Web UI

- [ ] Implement freq_test

## Orchestration

- [ ] Inline the apache_tool into install.
- [ ] Inline the old version cleanup into install.
- [ ] Update `make install` to call installer
- [ ] Update `make uninstall` to call uninstaller
- [ ] Create release script orchestration
- [ ] Uninstall:
        [INFO ] Ensuring destination directory does not exist: '/home/pi/WsprryPi'
        [INFO ] Destination directory already exists: '/home/pi/WsprryPi'


## Documentation

- [ ] Documentation needs updates from/related to changes to command line parsing (including INI vs option precedence)
- [ ] Note in docs that they may see: `2025-02-20 23:10:00 UTC [INFO ] Transmission started.` ... this is a floating point rounding issue.
- [ ] DMA notes at: `https://github.com/fandahao17/Raspberry-Pi-DMA-Tutorial`
- [ ] Remove WSPR-15 support

## Shit to Remember

- [ ] No need to disable_sound() on Pi 5 and up (install and uninstall)

## WSPR15 Notes:

Overview of WSPR-15

WSPR-15 (“Weak Signal Propagation Reporter” with a 15 minute transmit/receive cycle) was introduced in January 2013 as part of the experimental WSPR-X software suite. By stretching the standard 2 minute cycle to 15 minutes, WSPR-15 lowers the symbol rate to about 0.183 Hz tone spacing (versus 1.4648 Hz in standard WSPR), narrowing the occupied bandwidth to ≈ 0.7 Hz.  This yields roughly a 9 dB sensitivity improvement, making it particularly attractive for very low-frequency (LF, MF) beacon work where Doppler shifts are minimal
https://de.wikipedia.org/wiki/Weak_Signal_Propagation_Reporter?utm_source=chatgpt.com
https://www.scribd.com/document/358388839/WSPR-X-Users-Guide?utm_source=chatgpt.com

Current Support and Viability
	•	Software support is scarce.  The only decoders known to handle WSPR-15 are the legacy WSPRX program and the now-unmaintained WSPR-X client.  Mainstream WSJT-X releases (e.g., WSJT-X 2.7) implement only the standard 2 minute WSPR protocol and offer no WSPR-15 option
        https://wsjt.sourceforge.io/wsjtx-doc/wsjtx-main-2.6.1.html?utm_source=chatgpt.com
        https://wsprdaemon.org/wspr-field-names.html?utm_source=chatgpt.com
	•	Network integration is limited.  The central WSPRnet database and most reporting services expect the standard WSPR2 format; uploading or mapping WSPR-15 spots generally requires unofficial workarounds (for example, using wsprdaemon’s mode-designator code “2” for WSPR15) and sees only niche community use
        https://wsprdaemon.org/wspr-field-names.html?utm_source=chatgpt.com
	•	Modern alternatives outperform it.  Within WSJT-X, the FST4W family (e.g., FST4W-120) achieves similar or better sensitivity than standard WSPR—with FST4W-120 about 1.4 dB more sensitive—and is fully supported, actively developed, and widely adopted on LF/MF bands.  The WSJT-X author explicitly recommends migrating LF/MF propagation tests from JT9/WSPR to FST4/FST4W for both sensitivity and ease of use
        https://wsjt.sourceforge.io/FST4_Quick_Start.pdf?utm_source=chatgpt.com

Bottom line: While WSPR-15 still “works” if you can run WSPRX or WSPR-X and manage manual uploads, it remains a niche, legacy protocol with minimal software and network support.  For new or ongoing weak-signal experiments—especially on LF/MF bands—you’ll find greater viability and community backing in the FST4W modes (or stick with the standard 2 minute WSPR2 on HF).
