# Bluetooth - Man-In-The-Middle #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_man_in_the_middle_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_man_in_the_middle_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_man_in_the_middle_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_man_in_the_middle_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_man_in_the_middle_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_man_in_the_middle_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_man_in_the_middle_common.json&label=RAM&query=ram&color=blue)
## Overview ##

This project shows the implementation of Man-In-The-Middle with BLE. It demonstrates how to avoid this vulnerability. It will show how we can prevent the risk in the MITM attack with BLE configuration. This project makes an MITM example in the case of using the thermometer server, a smartphone and the MITM device.

- MITM device: This device acts as the attacker. It will try to interrupt the connection between the server and the smartphone, then it fakes the server to connect with the smartphone, then send malicious data to it.

- Thermometer server: This device acts as the server that connects with the smartphone at first. This runs two projects, one is the Bluetooth - SoC Thermometer example to demonstrate the unsecured server and another project is The Bluetooth - SoC Thermometer Authenticated Server to demonstrate the secure server.

The following picture shows the system view of how it works.

![board](images/connection.png)

## Gecko SDK Suite version ##

- GSDK v4.4.0

## Hardware Required ##

**Attacker:**
- [EFR32MG12 2.4 GHz 19 dBm Radio Board Brd4161a](https://www.silabs.com/documents/public/reference-manuals/brd4161a-rm.pdf)

- [BRD4001A A01 Wireless Starter Kit Mainboard](https://www.silabs.com/documents/public/schematic-files/BRD4001A-A01-schematic.pdf)

**Server:**
- [EFR32MG12 2.4 GHz 19 dBm Radio Board Brd4162a](https://www.silabs.com/documents/public/reference-manuals/brd4162a-rm.pdf)

- [BRD4001A A01 Wireless Starter Kit Mainboard](https://www.silabs.com/documents/public/schematic-files/BRD4001A-A01-schematic.pdf)

**NOTE:**
Tested boards for working with this example:

**Attacker:** bt_man_in_the_middle.slcp
| Board ID | Description  |
| ---------------------- | ------ |
| BRD4161A | [EFR32MG12 2.4 GHz 19 dBm Radio Board](https://www.silabs.com/documents/public/reference-manuals/brd4161a-rm.pdf)    |
| BRD4162A | [EFR32MG12 2.4 GHz 19 dBm Radio Board](https://www.silabs.com/documents/public/reference-manuals/brd4162a-rm.pdf)    |
| BRD2704A | [SparkFun Thing Plus Matter - MGM240P](https://www.sparkfun.com/products/20270)                                      |
| BRD2703A | [EFR32xG24 Explorer Kit](https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit?tab=overview)                                                               |
| BRD2601B | [EFR32xG24 Dev Kit](https://www.silabs.com/development-tools/wireless/efr32xg24-dev-kit?tab=overview)                                                               |
| BRD4108A | [BG22 Bluetooth SoC Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth/bg22-explorer-kit?tab=overview)    |
| BRD4314A | [BGM220 Bluetooth Module Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit?tab=overview)

**Server:** bt_thermometer_auth.slcp
| Board ID | Description  |
| ---------------------- | ------ |
| BRD4161A | [EFR32MG12 2.4 GHz 19 dBm Radio Board Brd4161a](https://www.silabs.com/documents/public/reference-manuals/brd4161a-rm.pdf)    |
| BRD4162A | [EFR32MG12 2.4 GHz 19 dBm Radio Board Brd4162a](https://www.silabs.com/documents/public/reference-manuals/brd4162a-rm.pdf)    |
| BRD2601B | [EFR32xG24 Dev Kit](https://www.silabs.com/development-tools/wireless/efr32xg24-dev-kit?tab=overview)

## Connections Required ##

The following picture shows the hardware for the MITM device.

![hardware_connect](images/hardware_connect.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

### Create a project based on an example project ###

1. From the Launcher Home, add your product name to MyProducts, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with filter "mitm".

2. Click **Create** button on the **Bluetooth - Man In The Middle Device Example** and **Bluetooth - SoC Thermometer Authenticator Server** example. These example projects creation dialog pops up -> click Create and Finish and Projects should be generated.
![board](images/create_project.png)

3. Build and flash this example to your boards.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src/**/app.c* folders into the project root folder (overwriting existing):
   - With **Attacker** (MITM device): *src/mitm_device/app.c*
   - With **Server** device: *src/thermometer_auth/app.c*

3. Import the GATT configuration:

    - Open the .slcp file in the project.

    - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

    - Find the Import button and import the attached gatt_configuration.btconf file.

        - With **Attacker** device: *config/mitm_device/gatt_configuration.btconf*
        - With **Server** device: *config/thermometer_auth/gatt_configuration.btconf*

    - Save the GATT configuration (ctrl-s).

4. Open the .slcp file. Select the **SOFTWARE COMPONENTS** tab and install the software components:

    - For **Attacker** device:
        - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom**
        - [Application] → [Utility] → [Log]
        - [Application] → [Utility] → [Assert]
        - [Bluetooth] → [Feature]: uninstall [Scanner for legacy advertisements]
        - [Bluetooth] → [Feature] → [Scanner Base Feature]
        - [Platform] → [Board] → [Board Control] → enable *Virtual COM UART*

    - For **Server** device:
        - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom**
        - [Application] → [Utility] → [Log]
        - [Application] → [Utility] → [Assert]
        - [Platform] → [Driver] → [I2C] → [I2CSPM] → default instance name: **sensor**
        - [Bluetooth] → [GATT] → [Health Thermometer API]
        - [Application] → [Sensor] → [Relative Humidity and Temperature sensor]
        - [Application] → [Services] → [Simple timer service]
        - [Platform] → [Board] → [Board Control] → enable *Virtual COM UART*

5. Install printf float

    - Open Properties of the project.

    - Select C/C++ Build → Settings → Tool Settings → GNU ARM C Linker → General. Check Printf float.
![float](images/float.png)

6. Build and flash these projects to your boards.

**Note:**

- Make sure that this repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).
- Do not forget to flash a bootloader to your board, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How it Works ##

In this example, we follow the MITM (Man-In-The-Middle) scenario, which means:

1. We have three devices:
    - Attacker (MITM).
    - Real health thermometer server. It is an EFR32MG12 radio board + BRD4001A WSTK mainboard which runs the Bluetooth - SoC Thermometer project(an example project already included in Simplicity)
    - Smartphone.

2. At first, the smartphone connects to the real health thermometer server via its advertisement and transfers data to this.
3. Then a connection issue is visible and the smartphone is temporarily disconnected.
4. The MITM device connects to the real health thermometer server and advertises itself as the real server.
5. A smartphone connects to the MITM device.
6. The MITM gets the real data from the real health thermometer server, then alters the signs (i.e. changing the value to negative) and sends the wrong data to the smartphone.

The below steps show the operation of the following scenario:

Firstly:

- Open your EFR app on your smartphone:
- Search device with filter: 'Thermometer'
- Press the 'Connect' button to establish the connection between devices.
- It's ready to transfer data.

    ![connect_efr](images/connect_efr.png)

- In the log console, you will see the status of the server:

    ![connected_server](images/connected_server.png)

To simulate some problems with the connection, temporarily disconnect the health thermometer server in the EFR app from your smartphone.
Then turn on the attacker device; the attacker connects to the real health thermometer server and advertises itself as the real server with the same name.

![attacker_connect](images/attacker_connect.png)
That smartphone is connected to the attacker and gets the malicious data.

![malicious_data](images/malicious_data.png)

This is the Man-In-The-Middle scenario. To prevent the MITM attack, another project shows the initialization and configuration with the Bluetooth Security Manager API to prevent MITM. The bt_thermometer_auth project shows how to initialize the security manager in the system_boot_id event. The temperature measurement characteristic is configured with authenticated notification. Replace the Bluetooth - SoC Health Thermometer project with the Bluetooth - SoC Thermometer Authenticated Server project and try this scenario again.

When **Bluetooth - SoC Thermometer Authenticated Server** is used, every time the Thermometer Authenticated server connects to another device, it will require bonding and increasing security of the connection.

![connect_auth](images/connect_auth.png)

A passkey is generated and transferred.

![passkey](images/passkey.png)

If the passkey matches, the connection will be established.

![auth_success](images/auth_success.png)

When the attacker tries to connect to the Thermometer Authenticated server, it has no passkey for bonding. The Thermometer Authenticated server rejects the connection.

![auth_fail](images/auth_fail.png)
