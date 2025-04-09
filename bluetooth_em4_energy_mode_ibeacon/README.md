# Bluetooth - EM4 Energy Mode in iBeacon Application #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-168.66%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-9.14%20KB-blue)
## Overview ##

This code example shows the use of sleep mode EM4 in a Bluetooth iBeacon Application. This example uses a Bluetooth iBeacon device, which operates in deep sleep mode EM4. When either the user interrupts the operation via a button, or the BURTC counter reaches the top value (15000 = 15 sec), the device exits from EM4 and advertises for 5 seconds before going back to EM4 again. BURTC counter resets every time the device exits from EM4.

Note that wake-up from EM4 is performed through a reset. Thus, no data is retained from the previous state, and the stack is reinitialized.

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)

## Hardware Required ##

- 1x [Bluetooth Low Energy Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For example, [EFR32xG24 Dev Kit](https://www.silabs.com/development-tools/wireless/efr32xg24-dev-kit?tab=overview)

**Note:**

- Users can use the Silicon Labs Explorer Kit or Development Kit instead of the WSTK. However, it requires an STK/WSTK Debug Adapter and there must be a mini Simplicity connector on this board.
- This project is tested only with the [EFR32xG24 Dev Kit](https://www.silabs.com/development-tools/wireless/efr32xg24-dev-kit?tab=overview)

## Connections Required ##

- Connect the EFR32xG24 Dev Kit to the PC through a micro USB cable.

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC iBeacon" project based on your hardware.

**NOTE**:

- Make sure that the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

### Create a project based on an example project ###

1. From the Launcher Home, add your hardware to "My Products", click on it, and click on the EXAMPLE PROJECTS & DEMOS tab. Find the example project filtering by "EM4".

2. Click **Create** button on both **Bluetooth - Using EM4 Energy Mode in Bluetooth iBeacon App** example. Example project creation dialog pops up -> click Create and Finish and the projects will be generated.

    ![create example](image/create_example.png)

3. Build and flash the examples to the board.

### Start with a "Bluetooth - SoC iBeacon" project ###

1. Create **Bluetooth - SoC iBeacon** projects for your hardware using Simplicity Studio 5
2. Copy the attached src/app.c file into your project (overwriting existing).
3. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    - [Platform] → [Driver] → [Button] → [Simple Button] → instance name: **em4wu** → Configure pin as below:

       | Board | BRD2703A | BRD4314A | BRD4108A | BRD2601B |
       |:---:|:----:|:----:|:----:|:----:|
       | EM4WU GPIO | PB3 | PC7 | PC7 | PB3 |

4. Build and flash the project to your device.

## How it Works ##

To initialize a device for EM4:

```C
EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;
EMU_EM4Init(&em4Init);
```

To wake-up from EM4 using a specific module, the EM4WUEN register of that module must be enabled. In this example code, this is done via the call to  `BURTC_Init()` and `GPIO_EM4EnablePinWakeup()` for the BURTC and GPIO wakeup pins, respectively.

### Testing ###

In this example, we use the Energy Profiler to see how the energy changes between the advertising time and when the board is in EM4 mode by monitoring the current.

If users do not have the compatible WSTK, they need to use a Silicon Labs motherboard and a debug adapter to open the Energy Profier because the Energy Profier does not support the Explorer and the Development Kit. The connection should be the same as below.

![debug adapter](image/debug_adapter.png)

After connecting all the required hardwares, users need to go back to the LAUNCHER HOME. In the debug adapter window, right-click on your connected motherboard and choose **device configuration** then choose **Adapter configuration** tab. Select the **Debug Mode** as **OUT** mode.

Finally, Select the debug interface as JTAG to detect the target part. Open the Energy Profiler to view the result.

![result](image/result.png)
