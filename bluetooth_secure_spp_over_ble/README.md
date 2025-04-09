# Bluetooth - Secure SPP (Serial Port Profile) over BLE

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-206.33%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-10.83%20KB-blue)

## Background

This code example has a related User's Guide, here: [Using Bluetooth Security Features](https://docs.silabs.com/bluetooth/latest/general/security/using-bluetooth-security-features-in-silicon-labs-bluetooth-sdk).

## Description

This code example demonstrates how to build a secure Bluetooth connection between two EFR32 devices and how to implement secure serial communication between them. The example relies on the [Bluetooth - Serial Port Profile (SPP)](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_serial_port_profile) example which implements the serial communication between the two devices. See the SPP example first to understand how it works. This example puts emphasis on the security side.

Just like with [Bluetooth - Serial Port Profile (SPP)](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_serial_port_profile), the client and server roles are combined into one code. Upload the same code to both devices that you want to connect. The roles are configured by holding down either pushbutton at boot time. Holding down the button makes that device a central (client), which is scanning for other devices that implements the SPP service. If none of the buttons is held down during reset, the device becomes a peripherial (server) and starts advertising.

When the connection is built and services and characteristics are discovered, the devices have to go through an authentication phase before starting secure data transfer. In this example, I/O capabilities are defined to DisplayYesNo, which ultimately results in a Numeric Comparison pairing method. For details, see the description of [Pairing Processes](https://docs.silabs.com/bluetooth/latest/general/security/pairing-processes).

During the authentication phase, the devices have to confirm that they have successfully built a secure connection by showing passkeys on both devices. If the passkeys match and both devices confirm it, they are bonded (paired). If the devices were already bonded when they connected, the authentication is not needed again and the secure connection is automatically built using the long term key stored in the flash.

Bonding information including the long term key and other information persists over reset. To test the authentication repeatedly, implement a call to **sm_delete_bondings** with a trigger of your choice. Another option is to use the Simplicity Commander tool to erase the devices' flash.

## SDK version

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)

## Software Required

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)

## Hardware Required

- 2x [Bluetooth Low Energy Development Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For simplicity, Silicon Labs recommends the [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)

## Connections Required

- Connect the Bluetooth Development Kits to the PC through a compatible-cable. For example, a micro USB cable for the BGM220 Bluetooth Module Explorer Kit.

## Setup

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

**NOTE**:

- Make sure that the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

### Create a project based on an example project

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by "secure".

2. Click **Create** button on the **Bluetooth - Secure SPP over BLE** examples. Example project creation dialog pops up -> click Create and Finish and Project should be generated.
![create_project](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in inc and src folders into the project root folder (overwriting existing file).

3. Install the software components to use the **VCOM** port (UART) for logging:
   - Open the .slcp file in the project.

   - Select the SOFTWARE COMPONENTS tab.

   - Install **IO Stream: USART** component with the default instance name: **vcom**  
    ![install_usart](image/install_usart.png)

   - Install the **Log** component (found under Bluetooth > Utility group)
   ![log](image/log.png)

   - Create the first button by installing the **Simple Button** component with the default instance name: **btn0**
   ![btn0](image/btn0.png)

4. Import the GATT configuration:
    - Open the **Bluetooth GATT Configurator** under the **CONFIGURATION TOOLS** tab.
    - Find the Import button and import the attached **gatt_configuration.btconf** file.
    ![btconf](image/btconf.png)
    - Save the GATT configuration (Ctrl+S).

5. Build and flash the project to each device.

**NOTE:**

- A bootloader needs to be flashed to your board if the project starts from the "Bluetooth - SoC Empty" project, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works

1. Open a terminal program (e.g., TeraTerm) and connect to both devices to see the logs.

2. Press PB0 or PB1 on one of the devices while resetting it to put it into client mode.

3. Simply reset the other device to put it into server mode.

4. After the first boot, the authentication phase requires confirmation that the passkeys displayed on the two devices match. To confirm, send a 'y' or 'Y' character with the terminal. To decline send 'n' or 'N'.

5. After the authentication phase, use the application to send characters from one device to the other just like with the [Bluetooth - Serial Port Profile (SPP)](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_serial_port_profile) example.
![terminal_output](image/terminal_output.png)

6. Reboot the devices and check if the secure connection is built automatically.

7. Open the Network Analyzer, connect to one of the devices, and check its traffic. You should see encrypted content.

Note: to get a proper log, Network Analyzer needs to observe the connection from the very beginning. Hence, after connecting to your device, reset it, and let the Network Analyzer observe the connection establishment process.

![network_analyzer](image/network_analyzer.png)
