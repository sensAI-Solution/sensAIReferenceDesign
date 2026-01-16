# LSC219 Camera Driver
This directory contains source code for enabling the IMX219 RPi V2 camera sensor attached to the LSCC SOM.

## Components
Enabling the IMX219 attached to the SOM needs:
- Modified kernel driver (imx219.c)
    + This is derived from the inbox imx219 camera driver from Raspberry Pi Linux kernel source.
    + All i2c related callouts are stubbed and diverted to local (static) dummy functions.
    + This driver keeps its original name (imx219.ko) to make sure that the kernel loads the V4L2 machinery as is.
    + The original driver (/lib/modules/<KERNEL_VERSION>>/kernel/drivers/media/i2c/imx219.ko.xz) needs to be disabled to prevent it from loading.
    + Finally, this modified driver needs to be loaded before starting any camera application via an insmod.
    + These tasks are handled by the script:
    ```
    scripts/enable_lsc219_cam.sh
    ```

- Device tree overlay (lsc219_overlay.dtsi)
    + This device tree overlay uses non i2c base target absolute path in the main device-tree.
    + We remove all i2c related port bindings.
    + Device tree fragment uses random numbering to differentiate the sections in the .dtsi overlay file.
    + Sensore@51 is a dummy number used as an indication for Gard FW I2C address number.
    + Remove the I2C client driver binding and use the platform device.
    + The modified imx219 driver does not use I2C Port and address binding. This leaves the 0x10 slave address free for use by user-space applications to configure the LSC219 (if needed).
    + To enable the LSC219 camera, the following line needs to be added to /boot/firmware/config.txt:
    ```
    dtoverlay=lsc219,cam0
    ```
    + The dtbo file generated from this dtsi (lsc219.dtbo) needs to be copied to /boot/overlays. This is handled by the script:
    ```
    scripts/enable_lsc219_cam.sh
    ```

## HUB developer mode
HUB developers can enable and test the LSC219 camera using the make target:
```
make enable_lsc219_cam
```

## HUB package mode
HUB debian package when installed on the system gives access to LSC219 camera enablement via the script:
```
/opt/hub/bin/enable_lsc219_cam.sh
```

# Usage
Run the script "enable_lsc219_cam" (either in dev mode or package mode) **twice** to enable the LSC219 camera.
Enter "2" or "4" as per desired lane configuration for LSC219 camera.
There should be a POWER CYCLE between these 2 runs of the script.

*INFO: The first run copies the relevant files and device-tree overlay blobs.
The second run (post POWER-CYCLE) loads the custom drivers and configures the camera for action.*
