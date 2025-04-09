# Bluetooth - SoC Blinky - SparkFun Thing Plus Matter - MGM240P #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Sparkfun-Thing%20Plus%20Matter-green)](https://www.sparkfun.com/products/20270)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-203.84%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-10.64%20KB-blue)
## Overview ##

This example application is the "Hello World" of Bluetooth Low Energy (BLE). It will also provide a starting point for developers who are interested in developing BLE applications with the SparkFun Thing Plus Matter Board. It allows a BLE central device to control the LED on the mainboard via write operations from a GATT client. It also can receive button press notifications when the external button state changes (pressed or released). This example will not be supported directly by Silicon Labs' GSDK, but users will be able to download it from GitHub.

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required ##

- 1x [SparkFun Thing Plus Matter - MGM240P - BRD2704A](https://www.sparkfun.com/products/20270)
- 1x external button since the BRD2704A board has no the on-board button
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required ##

The following picture shows the system view of how it works.

![board](image/connection.png)

Listed below are the port and pin mappings for work with this example.

Board: **BRD2704A - SparkFun Thing Plus Matter - MGM240P Board**

| Button Pin | Connection | Pin function |
|:---:|:-------------:|:---------------|
| Pin 1 | PB04 | Button |
| Pin 2 | GND | GND |

**Note:**

- To connect the external button to the board and make the project more stable, you should use a ceramic capacitor (ex: Ceramic Capacitor 104) and a resistor to avoid the anti-vibration button used in the project as below.
![harware_connection](image/hardware_connection.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

**NOTE**:

- Make sure that the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

### Create a project based on an example project ###

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by "soc blinky".

2. Click **Create** button on the **Bluetooth - SoC Blinky - SparkFun Thing Plus Matter - MGM240P** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.
![board](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing).

3. Import the GATT configuration:

    - Open the .slcp file in the project.

    - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

    - Find the Import button and import the attached [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.

    - Save the GATT configuration (ctrl-s).

4. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: vcom

    - [Application] → [Utility] → [Log]

    - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: led0

    - [Platform] → [Driver] → [Button] → [Simple Button] → set instance name: btn0 → configure btn0 to select the "Selected Module" as PB04
    ![button_configuration](image/button_configuration.png)

5. Build and flash the project to your device.

**Note:**

- A bootloader needs to be flashed to your board if the project starts from the "Bluetooth - SoC Empty" project, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

This example implements a simple custom GATT service with two characteristics. One characteristic controls the state of the LED (ON/OFF) via write operations from a GATT client, and the second characteristic sends notifications to subscribed clients when the button state changes (pressed or released).

After launching the Simplicity Connect application, go to the demo view and select the Blinky demo. A pop-up will show all the devices around you that are running the SoC-Blinky firmware. Tap on the device to go into the demo view.

| ![Demo view](image/demo_app_list.jpg) | ![Pop up](image/device_selection.jpg) |
|-|-|

Tap the light on the mobile application to toggle the LED on the mainboard. When you press/release the external button, the state changes for the virtual button on the application as well.

| ![Blinky all off](image/demo_blinky_off_all.jpg) | ![Blinky all on](image/demo_blinky_on_all.jpg) |
|-|-|

The animation below showcases the demo running on a SparkFun Thing Plus Board (MGM240P - BRD2704A) with the mobile application running on an Android device.

![Demo](image/demo.GIF)
