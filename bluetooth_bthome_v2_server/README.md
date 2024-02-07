# Bluetooth - BTHome v2 - Server Example #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_server_gateway_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_server_gateway_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_server_gateway_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_server_gateway_common.json&label=SDK&query=sdk&color=green)
[![Required board](https://img.shields.io/badge/Sparkfun-Thing%20Plus%20Matter-green)](https://www.sparkfun.com/products/20270)
[![Required board](https://img.shields.io/badge/Adafruit-HXD8357D-green)](https://www.adafruit.com/product/2050)
[![Required board](https://img.shields.io/badge/Sparkfun-Thing%20Plus%20Matter-green)](https://www.sparkfun.com/products/20270)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_server_gateway_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_server_gateway_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_server_gateway_common.json&label=RAM&query=ram&color=blue)
## Overview ##

The example showcases the implementation of BTHome support for Silabs development kits.

This project aims to implement a BTHome v2 server device. Integrated with the Adafruit HXD8357D - 3.5" TFT LCD with Touchscreen, the server/gateway device is able to show information on the screen and interact with users via touch directly on the screen. The example also demonstrates the scanning process, which allows configuring a device name and an encryption key as well as removing devices.

This example uses the source files which are generated from **[SquareLine Studio](https://squareline.io/)**. These files are provided in the `app_ui` folder. **SquareLine Studio** is a professional and easy-to-use UI editor software to speed up and simplify UI development. Visiting this page for more information

## Gecko SDK version ##

- GSDK 4.4.0

- [Third Party Hardware Drivers v1.11.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Hardware Required ##

- [SparkFun Thing Plus Matter - MGM240P](https://www.sparkfun.com/products/20270)

- [Adafruit HXD8357D - 3.5" TFT LCD with Touchscreen](https://www.adafruit.com/product/2050)

**NOTE:**
Tested boards for working with this example:

| Board ID | Description  |
| -------- | ------ |
| BRD2703A | [EFR32xG24 Explorer Kit - xG24-EK2703A](https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit?tab=overview)    |
| BRD2704A | [SparkFun Thing Plus Matter - MGM240P](https://www.sparkfun.com/products/20270)  |

## Connections Required ##

The following picture shows the connection for this application:

![overview](image/overview.png)

## Hardware Connection ##

To connect the Adafruit 3.5" TFT LCD (with Touchscreen) with the boards, you can see the pins mapping table below.

**SparkFun Thing Plus Matter - MGM240P**

| Pin | Connection | Pin function |
|:---:|:-------------:|:---------------|
| PC0 | D/C | GPIO |
| PC1 | CS | SPI CS |
| PC2 | CLK | SPI SCK |
| PC6 | MISO | SPI MISO |
| PC3 | MOSI | SPI MOSI |
| PD2 | XP(X+) | AN |
| PA0 | YP(Y+) | AN |
| PD1 | YM(Y-) | AN |
| PD0 | XM(X-) | AN |

**EFR32xG24 Explorer Kit - xG24-EK2703A**

| Pin | Connection | Pin function |
|:---:|:-------------:|:---------------|
| PC8 | D/C | GPIO |
| PC0 | CS | SPI CS |
| PC1 | CLK | SPI SCK |
| PC2 | MISO | SPI MISO |
| PC3 | MOSI | SPI MOSI |
| PD5 | XP(X+) | AN |
| PD4 | YP(Y+) | AN |
| PB1 | YM(Y-) | AN |
| PA0 | XM(X-) | AN |

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

### Create a project based on an example project ###

1. From the Launcher Home, add your product name to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with the filter **"bthome"** and **"server"**.

2. Click the **Create** button on **Bluetooth - BTHome v2 - Server/Gateway** example. Example project creation dialog pops up -> click Create and Finish and the project should be generated.
![create_project](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all the files in `app_ui` folder and the app.c file in the `src` folder into the project root folder (overwriting the existing files).

3. Install the software components:

    - Open the .slcp file in the project

    - Select the SOFTWARE COMPONENTS tab

    - Install the following components:

      - [Services] → [IO Stream] → [Driver] → [IO Stream: EUSART] → default instance name: vcom.

      - [Application] → [Utility] → [Log]

      - [Third-Party Hardware Drivers] → [Display & LED] → [HXD8357D - TFT LCD Display (Adafruit) - SPI with DMA]

      - [Third-Party Hardware Drivers] → [Services] → [BTHome v2 - Server] → use the configuration as below:
      ![bthome_server_configuraion](image/bthome_server_configuration.png)
  
      - [Third-Party Hardware Drivers] → [Services] → [LVGL - Graphics Library → You should configure **Two buffers** mode.
      ![lgvl_config](image/lgvl_config.png)

      - [Third-Party Hardware Drivers] → [Human Machine Interface] → [Touch Screen (Analog)] → Using settings as below:
      ![touch_screen_config](image/touch_screen_config.png)

4. Build and flash the project to your device.

**Note:**

- Make sure the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is added to the required SDK: [Preferences > Simplicity Studio > SDKs](https://github.com/SiliconLabs/third_party_hw_drivers_extension/blob/master/README.md#how-to-add-to-simplicity-studio-ide).

- SDK Extension must be enabled for the project to install third-party components.

- Do not forget to flash a bootloader to your board, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

The example provides a simple user interface, which allows interaction with the device by touching directly on the 3.5" TFT LCD with Touchscreen. It is designed with 4 pages. 

![screen_page](image/screen_pages.png)

The content and function of each page are described below:

  - Home page : shows the information of client devices.

  - Setting page : contains two buttons, which can help the user add new devices or remove existing devices.

  - Scanning page : list all BTHome compatible devices with the MAC address. The user can add the new device by touching the device address.

  - Device configuration page : The user can configure a device name and an encryption key for the new device. 

To verify how the example works, you should follow the guide below:

1. Because the example could support other examples, which relevant to the BTHome v2 format. You could prepare three or four client devices. These devices need to be flashed the existing BTHome v2 examples in this repository, which are available in this repository, including :

     - Bluetooth - BTHome v2 - Switch

     - Bluetooth - BTHome v2 - Light

     - Bluetooth - BTHome v2 - Internal Temperature Monitor

     - Bluetooth - BTHome v2 - Humidity and Temperature Monitor (SHTC3)

    **Note:**

    - If you use BTHome switch and light devices, you need to configure the switch device as a client device and the light device to act as a server device.

2. You can add 6 client devices using **Add new devices** button on the screen. After that, the gateway device will list all the client devices, which are scanned. The example only supports 6 scanned devices. Then, you have to register an encryption key and the device name (optional). The following picture explains the adding process.
![adding_process](image/adding_process.png)

1. You can go back to the home screen, using the **exit button** on the bottom of each screen. After adding new client devices, the gateway device can decrypt the advertising packets, and get the information. On the home screen, the device shows the name, MAC address and device information of each client device. If there is no data updated, the screen will show the last time when the client device data was updated.

    ![home_screen](image/home_screen.png)

2. Moreover, the example allows users can remove the client devices. The removal process is illustrated in the following picture.

    ![removal_process](image/removal_process.png)