# Bluetooth - Accelerometer (BMA400) #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Mikroe-Accel%205%20Click%20board-green)](https://www.mikroe.com/accel-5-click)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-198.95%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-10.7%20KB-blue)
## Overview ##

This project aims to implement an accelerometer application using Silicon Labs development kits and external sensors integrated with the BLE wireless stack.

The following picture shows the system view of how it works.

![hardware_connect](image/hardware_connect.png)

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required ##

- 1x [Bluetooth Low Energy Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For example, [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- 1x [MikroE Accel 5 Click board](https://www.mikroe.com/accel-5-click)
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required ##

The mikroBUS Accel 5 Click board can be just "clicked" into its place. Be sure that the board's 45-degree corner matches the Explorer Kit's 45-degree white line.

![connection](image/connection.png)

Make sure that the click board is properly configured (I2C-mode or SPI-mode) by the resistors. Also, if using the I2C interface the application uses by default **I2C address 0x15** as it is the Accel 5 click default (the resistor labeled "I2C ADD" is on the "1". If setting "I2C ADD" resistor "0", the address will be 0x14).

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

**NOTE**:

- Make sure that the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is installed as part of the SiSDK and the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- SDK Extension must be enabled for the project to install the required components.

### Create a project based on an example project ###

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by "accelerometer".

2. Click **Create** button on the example:

    - **Bluetooth - Accelerometer (BMA400) - I2C** if using the I2C interface.  

    - **Bluetooth - Accelerometer (BMA400) - SPI** if using the SPI interface.

    Example project creation dialog pops up -> click Create and Finish and Project should be generated.
    ![Create_example](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing).

3. Import the GATT configuration:

    - Open the .slcp file in the project.

    - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

    - Find the Import button and import the attached `config/gatt_configuration.btconf` file.

    - Save the GATT configuration (ctrl-s).

4. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: *vcom*

    - [Services] → [Timers] → [Sleep Timer]

    - [Application] → [Utility] → [Log]

    - [Application] → [Utility] → [Assert]

    - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: *led0*

    - [Platform] → [Driver] → [GPIOINT]

    - If using the I2C interface: [Third Party Hardware Drivers] → [Sensors] → [BMA400 - Accel 5 Click (Mikroe) - I2C] → use default configuration

    - If using the SPI interface: [Third Party Hardware Drivers] → [Sensors] → [BMA400 - Accel 5 Click (Mikroe) - SPI] → use default configuration

5. Build and flash the project to your device.

**NOTE**:

- A bootloader needs to be flashed to your board if the project starts from the "Bluetooth - SoC Empty" project, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT changes were adding a new custom service using UUID ```03519cae-ce32-44be-ad30-e2c7d068da03``` that has a characteristic UUID ```27038e55-8e48-b5f1-6e20-84a124258810``` with Read and Indicate properties. The acceleration characteristic is 3 bytes containing 1 byte per X-, Y- and Z-axis accelerations. The axis values are absolute accelerations where 1 g is about value of 98. Typically "0 0 98" when the board is on a level plane like a table.

After resetting, the program will continuously query the interrupt from bma400. Once the interrupt from bma400 occurs, the application reads the current accelerations. If the notification was enabled, the client is notified about the updated values. The sl_bt_evt_gatt_server_characteristic_status_id-event is handling the indication enable/disable control.

## Testing ##

Follow the below steps to test the example:

1. Open the Simplicity Connect app on your smartphone and allow the permission requested the first time it is opened.

2. Find your device in the Bluetooth Browser, advertising as "Silabs Example", and tap Connect.

3. Find the unknown service, try to read the unknown characteristic and check the value.

4. Enable notify on the unknown characteristic. Try to move your kit in some direction and check the value.

    ![image](image/unknown_service.png)

5. You can launch the Console that is integrated in Simplicity Studio or can use a third-party terminal tool like TeraTerm to receive the logs from the virtual COM port.

    ![image](image/console.png)

*Note*: The LED blinks once if the accelerometer initialization is successful. If the LED stays on, the initialization has been failed. The reason is typically wrong sensor I2C address (see "I2C ADD" resistors) or wrongly configured Click board mode (SPI-mode instead I2C) or if using some own ways to connect the sensor.
