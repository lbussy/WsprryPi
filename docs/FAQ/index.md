<!-- Grammar and spelling checked -->
# FAQ and Known Errors

## Install Error ``bash: line 1: syntax error near unexpected token `<'``
If this happens, the DNS redirect (vanity URL) I use to make the install command shorter and easier to type may have broken.

**Explanation:** The installation command line uses an application called `curl` to download the target URL.  The pipe operator (`|`) redirects that to whatever follows, in this case, `sudo` (run as root) and `bash` (the command interpreter) to make the bash script run as soon as it downloads.  If the redirect breaks somehow, a regular HTML page will be sent instead of the bash script.  Bash doesn't know what to do with HTML (the `<` in the first position of the first line), so it simply refuses to do anything.

You may use the following longer and more challenging to type, command instead (one line):

```bash
curl -L https://raw.githubusercontent.com/lbussy/WsprryPi/main/scripts/install.sh | sudo bash
```

## WSPR-15 Support

I have removed WSR-15 support in version 2.x.

WSPR-15 (“Weak Signal Propagation Reporter” with a 15-minute transmit/receive cycle) was introduced in January 2013 as part of the experimental WSPR-X software suite.  By stretching the standard 2-minute cycle to 15 minutes, WSPR-15 lowers the symbol rate to about 0.183 Hz tone spacing (versus 1.4648 Hz in standard WSPR), narrowing the occupied bandwidth to ≈ 0.7 Hz.  This yields roughly a 9 dB sensitivity improvement, making it particularly attractive for very low-frequency (LF, MF) beacon work where Doppler shifts are minimal.

Sources:

- https://de.wikipedia.org/wiki/Weak_Signal_Propagation_Reporter
- https://www.scribd.com/document/358388839/WSPR-X-Users-Guide

Current Support and Viability

- Software support is scarce.  The only decoders that handle WSPR-15 are the legacy WSPRX program and the now-unmaintained WSPR-X client.  Mainstream WSJT-X releases (e.g., WSJT-X 2.7) implement only the standard 2-minute WSPR protocol and offer no WSPR-15 option
- Network integration is limited.  The central WSPRnet database and most reporting services expect the standard WSPR2 format; uploading or mapping WSPR-15 spots generally requires unofficial workarounds (for example, using wsprdaemon’s mode-designator code “2” for WSPR15) and sees only niche community use.  Source: https://wsprdaemon.org/wspr-field-names.html
- Modern alternatives outperform it.  Within WSJT-X, the FST4W family (e.g., FST4W-120) achieves similar or better sensitivity than standard WSPR—with FST4W-120 about 1.4 dB more sensitive—and is fully supported, actively developed, and widely adopted on LF/MF bands.  The WSJT-X author explicitly recommends migrating LF/MF propagation tests from JT9/WSPR to FST4/FST4W for sensitivity and ease of use.  Source: https://wsjt.sourceforge.io/FST4_Quick_Start.pdf

Bottom line: While WSPR-15 still “works” if you can run WSPRX or WSPR-X and manage manual uploads, it remains a niche, legacy protocol with minimal software and network support.  For new or ongoing weak-signal experiments—especially on LF/MF bands—you’ll find greater viability and community backing in the FST4W modes (or stick with the standard 2-minute WSPR2 on HF).
