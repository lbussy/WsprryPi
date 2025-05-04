# Issue: Kernel PLL Frequency Stuck in WSPR Transmitter

tl;dr here:  I removed NTP and began using [chrony](https://chrony-project.org/) with much more success.

## Problem Summary

When running the WSPR transmitter, the frequency correction value (`ntx.freq`) retrieved from `ntp_adjtime()` remained stuck at `-662571`. This issue persisted even after rebooting the system, preventing accurate transmission timing.

## Symptoms

- The program logs the following message repeatedly:

  ``` text
  Debug: Raw ntx.freq from ntp_adjtime = -662571
  Warning: Invalid ntx.freq. Retrying in 2 seconds...
  ```

- The output of `sudo adjtimex -p` shows:

  ``` text
  frequency: -662571
  ```

- Rebooting **does not** reset the frequency correction.
- The WSPR transmitter starts with a **clamped** PPM value.

## Root Cause

The kernel PLL (Phase-Locked Loop) retained an incorrect frequency correction value, preventing updates. This was likely due to a combination of:

1. NTP not fully resetting its correction values.
2. Kernel timekeeping parameters being stuck across reboots.
3. Drift files or incorrect `adjtimex` settings being persistently applied.

## Fixing the Issue

### Step 1: Stop and Disable NTP

First, ensure that NTP is completely stopped to prevent interference.

```bash
sudo systemctl stop ntp
sudo systemctl disable ntp
```

### Step 2: Manually Reset Kernel PLL Settings

Force the system to reset all timekeeping adjustments:

```bash
sudo adjtimex --tick 10000 --frequency 0 --offset 0 --status 0
```

This will:

- Set the tick value to the default.
- Clear any stored frequency corrections.
- Reset any residual offset values.
- Disable PLL-based time adjustments.

### Step 3: Manually Set System Time

To prevent NTP from applying an old correction:

```bash
sudo date -s "$(date -u +'%Y-%m-%d %H:%M:%S')"
```

### Step 4: Reboot the System

```bash
sudo reboot
```

Rebooting ensures that the kernel starts fresh with no retained drift correction.

### Step 5: Verify the Reset Worked

After rebooting, check if the frequency correction has been cleared:

```bash
sudo adjtimex -p
```

- The `frequency` value should now be `0` or close to it.
- If it is still `-662571`, something is persisting the bad correction.

### Step 6: Re-enable and Restart NTP

Once `adjtimex -p` shows a correct frequency:

```bash
sudo systemctl enable ntp
sudo systemctl start ntp
sleep 5
sudo adjtimex -p
```

This allows NTP to reapply a fresh drift correction instead of using old values.

### Step 7: Run the WSPR Transmitter

Once NTP is stable, restart the program:

```bash
sudo ./wsprrypi
```

- If the frequency correction is now within range, the issue is resolved.
- If `ntx.freq` is still stuck, check `/var/lib/ntp/ntp.drift` and any custom NTP settings.

## Preventing This Issue in the Future

1. Monitor NTP regularly using:

   ```bash
   ntpq -p
   adjtimex -p
   ```

2. Ensure that NTP is not applying bad drift corrections** by checking `/var/lib/ntp/ntp.drift`.
3. Perform a full reset if `ntx.freq` is outside the normal range.

By following these steps, the WSPR transmitter should operate with accurate frequency calibration and stable PPM values.
