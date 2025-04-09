# Bluetooth - Door Lock

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Mikroe-BUZZ%202%20click-green)](https://www.mikroe.com/buzz-2-click)
[![Required board](https://img.shields.io/badge/Mikroe-Cap%20Touch%202%20Click-green)](https://www.mikroe.com/cap-touch-2-click)
[![Required board](https://img.shields.io/badge/Sparkfun-Micro%20OLED%20Breakout%20(Qwiic)-green)](https://www.sparkfun.com/products/14532)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-211.89%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-11.32%20KB-blue)

## Overview

This project demonstrates a Bluetooth door lock application using Silicon Labs development kits, [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532), [Cap Touch 2 Click MikroE board](https://www.mikroe.com/cap-touch-2-click), and [BUZZ 2 click Mikroe board](https://www.mikroe.com/buzz-2-click).

In this example, we use some security features of BLE such as pairing, bonding and setting up the attribute permissions for higher reliability.

The block diagram of this application is shown in the image below:

![block_diagram](image/block_diagram.png)

More detailed information can be found in the section [How it works](#how-it-works)

This code example referred to the following code examples. More detailed information can be found here:

- [OLED SSD1306 driver](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/silabs/micro_oled_ssd1306)
- [Cap Touch 2 Click driver](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/mikroe/captouch2_cap1166)
- [Bluetooth security feature](https://github.com/SiliconLabs/bluetooth_stack_features/tree/master/security)

## SDK version

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required

- 1x [Bluetooth Low Energy Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For example, [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- 1x [SparkFun Micro OLED Breakout (Qwiic)](https://www.sparkfun.com/products/14532)
- 1x [Cap Touch 2 Click](https://www.mikroe.com/cap-touch-2-click)
- 1x [BUZZ 2 click](https://www.mikroe.com/buzz-2-click)
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required

The hardware connection is shown in the image below:

![hardware connection](image/hardware_connection.png)

The SparkFun Micro OLED Breakout (Qwiic) board can be easily connected by using a Qwiic cable. Use jumper wires to connect the Explorer kit with the Cap touch board and Buzzer board.

The pin connection is shown in the image below:

![pinout connection](image/pinout_connection.png)

## Setup

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware. You should connect the BMG220 Explorer Kit Board to the PC using a MicroUSB cable.

**NOTE**:

- Make sure that the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is installed as part of the SiSDK and the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- SDK Extension must be enabled for the project to install the required components.

### Create a project based on an example project

1. From the Launcher Home, add the BRD4314A  to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by 'door lock'.

   ![create_demo](image/creat_demo.png "Create a project based on an example project")

2. Click **Create** button on the **Bluetooth - Door Lock** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy the .c files 'src/app.c' to the following directory of the project root folder (overwriting the existing files).

3. Install the software components:

    - Open the .slcp file in the project.

    - Select the SOFTWARE COMPONENTS tab.

    - Install the following components:

        - [Services] → [Timers] → [Sleep Timer]
        - [Bluetooth] → [Bluetooth Host (Stack)] → [Additional Features] → [NVM Support]
        - [Services] → [IO Stream] → [IO Stream: EUSART] → default instance name: **vcom**
        - [Application] → [Utility] → [Log]
        - [Platform] → [Driver] → [GPIOINT]
        - [Platform] → [Driver] → [I2C] → [I2CSPM] → default instance name: **qwiic**
        - [Platform] → [Driver] → [PWM] → [PWM] → default instance name: **mikroe**
        - [Platform] → [Driver] → [SPI] → [SPIDRV] → default instance name: **mikroe**
        - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: **led0**
        - [Third Party Hardware Drivers] → [Audio & Voice] → [CMT_8540S_SMT - Buzz 2 Click (Mikroe)]
        - [Third Party Hardware Drivers] → [Display & LED] → [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C]
        - [Third Party Hardware Drivers] → [Services] → [GLIB - OLED Graphics Library]
        - [Third Party Hardware Drivers] → [Human Machine Interface] → [CAP1166 - Capacitive Touch 2 Click (Mikroe)]
        - [Bluetooth] → [OTA] → [In-Place OTA DFU] → uninstall
        - [Platform] → [Bootloader Application Interface] → uninstall.

4. Import the GATT configuration:

    - Open the .slcp file in the project again.
    - Select the CONFIGURATION TOOLS tab and open the "Bluetooth GATT Configurator".
    - Find the Import button and import the [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.
    - Save the GATT configuration (ctrl-s).

5. Build and flash this project to the board.

## How it Works

### Unlock Door

The following diagram shows the main flowchart of this demo:

![workflow](image/work_flow.png)

Once the password is entered, the buzzer plays a sound, and the OLED displays the password with the '\*' character. The user should enter the password to unlock the door from the capacitive sensor. If the password is correct, the LED will turn on, the OLED will show **Unlock** for a few seconds. If it is wrong, it will display **Lock** and allow you to re-enter it from the beginning. Users can change the door unlock password over BLE, see the [Change the door unlock password](#change-the-door-unlock-password-and-the-passkey).

### GATT Configurator

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT changes were adding a new custom service using UUID `02AE647B-83B7-45AA-AA59-D3DDFEE325E9` that has a characteristic UUID `8AF6DEB2-B277-49F9-A4F2-14210C6766CF`. The **new_password** characteristic only has a written property with permission level **encrypted** and **bonded**, which means that the characteristic can be written only on encrypted and bonded connections. This protects against password alteration by a third party. The **new_password** characteristic has a **user data** type and 6-bytes length.

The GATT should contain the following services and characteristics:

| Name      | UUID                                 |
| --------- | ------------------------------------ |
| door_lock | 02AE647B-83B7-45AA-AA59-D3DDFEE325E9 |

Characteristic properties:

| Name         | Value Type | UUID                                 | Security                     | Properties |        Comment         |
| ------------ | ---------- | ------------------------------------ | ---------------------------- | ---------- | :--------------------: |
| new_password | User data  | 8AF6DEB2-B277-49F9-A4F2-14210C6766CF | Pairing and bonding required | Write      | Write with a response. |

### Use Simplicity Connect Mobile Application

#### Connect to the door

Follow the below steps to test the example with the Simplicity Connect application:

- Open the Simplicity Connect application on your smartphone and **allow the permission request** when opened for the first time.
- Click [Develop] -> [Browser] and you will see a list of nearby devices which are sending Bluetooth advertisements.
- Find the one named **Door Lock** and click the connect button on the right side. For iOS devices, enter the passkey `(passkey default as 123456)` to confirm authentication for the pairing process for the first time. For Android devices, the user must accept a pairing request first and do as above. After that, wait for the connection to be established and the GATT database to be loaded.

_Note_: The pairing process on Android and iOS devices is different. For more information, refer to [Bluetooth security](https://github.com/SiliconLabs/bluetooth_stack_features/tree/master/security).

![efr_connect_app](image/efr_connect_app.png "Simplicity Connect app")

#### Change the door unlock password and the passkey

The door unlock password and the passkey to connect with the mobile device will both be changed and will be the same as each other when updating the **new_password** characteristic.

To change both of them, the user must write a new password which has 6 bytes to the **Ascii string** field in the **new_password** characteristic. If the length of the new password is not 6 bytes then, an `INVALID ATTRIBUTE VALUE LENGTH` error will be thrown out. If the user writes to the new_password characteristic successfully, a new password will be updated in non-volatile memory. By contrast, the password will not be updated.

![password_service](image/password_service.png "Password")
