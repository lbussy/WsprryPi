# Switching to 4KB Pages on RPi5

The Raspberry Pi 5 defaults to a 16KB memory page size to enhance performance. However, some software may require the traditional 4KB page size for compatibility. To switch the Raspberry Pi 5 to use a 4KB page size kernel, follow these steps:

1. Edit the Boot Configuration File:
    * Open the terminal on your Raspberry Pi.
    * Execute the following command to edit the config.txt file:

        ``` bash
        sudo nano /boot/firmware/config.txt
        ```

2. Specify the 4KB Page Size Kernel:
    * In the config.txt file, add the following line:

        ``` bash
        kernel=kernel8.img
        ```

    * This directive instructs the Raspberry Pi to load the `kernel8.img` which operates with a 4KB page size.

3. Save and Exit:
    * Press `Ctrl + X` to exit the editor.
    * Press `Y` to confirm saving the changes.
    * Press `Enter` to write the changes to the file.

4. Reboot the Raspberry Pi:
    * Apply the changes by rebooting your device:

        ``` bash
        sudo reboot
        ```

5. Verify the Page Size:
    * After the system restarts, confirm the page size by running:

        ``` bash
        getconf PAGESIZE
        ```

    * The output should display `4096`, indicating a 4KB page size.

