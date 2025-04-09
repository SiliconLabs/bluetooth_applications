# Bluetooth - BTHome v2 - Switch #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-190.98%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-10.48%20KB-blue)
## Overview ##

The example showcases the implementation of BTHome support for Silicon Labs development kits.

This project aims to implement a BTHome v2 compatible switch. The device is in sleep mode to optimize power consumption. It wakes up once the button 0 on the board is pressed. The application supports press, double press, triple press, and long press events.

Raspberry Pi 4 runs a Home Assistant OS that scans for and detects BTHome v2 devices. You can use the Home Assistant application on your smartphone to communicate with the Raspberry Pi 4 to get advertisement packets from the BTHome v2 switch device, parse those packets, and recognize the button event.

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Home Assistant OS](https://www.home-assistant.io/)

## Hardware Required ##

- 1x [Bluetooth Low Energy Development Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For simplicity, Silicon Labs recommends the [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- 1x Raspberry Pi 4 running Home Assistant OS
- 1x smartphone running Home Assistant application

## Connections Required ##

The following picture shows the connection for this application:

![overview](image/overview.png)

**Note:**

- If you use **SparkFun Thing Plus Matter - MGM240P** to run this application, you have to set up an external button, because it has no integrated button. Please, connect this button to **PB0** pin on the SparkFun Thing Plus Matter board.

- To connect the external button to the board and make the project more stable, you should use a ceramic capacitor (ex: Ceramic Capacitor 104) and a resistor to avoid the anti-vibration button used in the project as below.
  
![external_button](image/external_button.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC iBeacon" project based on your hardware.

**NOTE**:

- Make sure that the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is installed as part of the SiSDK and the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- SDK Extension must be enabled for the project to install the required components.

### Create a project based on an example project ###

1. From the Launcher Home, add your product name to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by **"bthome"** and **"switch"**.

2. Click the **Create** button on **Bluetooth - BTHome v2 - Switch** example. Example project creation dialog pops up -> click Create and Finish and the project should be generated.
![create_project](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC iBeacon" project ###

1. Create a **Bluetooth - SoC iBeacon** project for your hardware using Simplicity Studio 5.

2. Copy the `src/app.c` file into the project root folder (overwriting the existing file).
3. Install the software components:

    - Open the .slcp file in the project

    - Select the SOFTWARE COMPONENTS tab

    - Install the following components:

      - [Services] → [IO Stream] → [Driver] → [IO Stream: USART] → default instance name: vcom.

      - [Application] → [Utility] → [Log]
  
      - [Platform] → [Driver] → [Button] → [Simple Button] → default instance name: btn0.
  
      - [Third-Party Hardware Drivers] → [Services] → [BTHome v2]
  
4. Build and flash the project to your device.

## How It Works ##

### Application Initialization ###

![application_initialization](image/application_init.png)

### External Interrupt ###

![external_interrupt](image/external_event.png)

## Testing ##

To test this example, you should follow some steps below:

1. Power on Raspberry Pi 4 and BTHome switch device. After that, the BThome switch device sends an advertisement packet with `none` event. After 5 seconds, the device goes into sleep mode to reduce power consumption. It can be woken up by pressing BTN0 on the board. Then the switch device will send a button event to the server. This example supports many kinds of button events, including single press, double press, triple press and long press events. If there is no button event sent, the device will send a `none` event after 2 seconds. Then go again sleep mode in 5 seconds.

    You can launch Console that's integrated into Simplicity Studio or use a third-party terminal tool like TeraTerm to receive the data from the USB. A screenshot of the console output is shown in the figure below.

    ![console_log](image/console_log.png)

2. Open the Home Assistant application on your smartphone, select [Settings] → [Devices and Services] → [Add Integration]
![add_device](image/add_device.png)

3. **Add Integration** with the name **'BTHome'**. You can see the list of devices, which are advertising in BTHome format. Choose your device with the correct name, e.g. **BTSwitch 79B4** for this example, and submit the bindkey, which is defined in the firmware of the BTHome switch device.

    - The device's name is `BTSwitch`.

    - The bindkey is  `11112222333344445555666677778888`.

    **Note:** To be able to find your switch device with the Home Assistant application, you need to use the same network on both Raspberry Pi 4 and your smartphone.
    ![configure_device](image/configure_device.png)

4. After adding your switch device successfully with the Bindkey, select a suitable area where your device is located.
![device_information](image/device_information.png)

5. Now you can see your switch device is already added to the Home Assistant system. You can track the button events in the **Logbook** section.
