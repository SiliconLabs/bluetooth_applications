# Bluetooth - SPP with Windows #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_spp_with_windows_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_spp_with_windows_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_spp_with_windows_common.json&label=License&query=license&color=green)

## Summary ##

This project uses Windows BLE API to communicate with the [Bluetooth - Serial Port Profile (SPP)](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_serial_port_profile) example. A custom Windows Console App was created that acts like TeraTerm and PuTTY but uses BLE on the back end.

## Hardware Required ##

- 1x [Bluetooth Low Energy Development Kit](https://www.silabs.com/development-tools/wireless/bluetooth) running the [Bluetooth - Serial Port Profile (SPP)](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_serial_port_profile) example. For simplicity, Silicon Labs recommends the [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- A Windows PC running the [ble-console-spp](executables/ble-console-spp.exe)

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)

## Connections Required ##

- Connect the Bluetooth Development Kits to the PC through a compatible-cable. For example, a micro USB cable for the BGM220 Bluetooth Module Explorer Kit.

## Setup ##

Create the [Bluetooth - Serial Port Profile (SPP)](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_serial_port_profile) example based on your hardware, then build and flash the image to your board. Connect your board to the PC and open a COM port in the terminal application of your choice. Launch the provided executable and type in the name of the BLE device. The device should connect to the Windows machine, and they will be able to communicate. Typing in either console should send the data to the other.

## How It Works ##

This project was inspired by a community post which the user asked if there were examples of a BLE device communicating with the computer wirelessly. I thought substituting the VCOM USB communication to a computer terminal (PuTTY) with a Bluetooth wireless communication to a computer terminal would help debug if the device is not accessible. The application shows how printf is retargeted to use BLE and how to get keyboard inputs from the console on the device.

Since there is no official Windows Console Apps that have this functionality, a custom console app was created using Visual Studio. An executable file of the project allows any user to use this without having to download the Visual Studio IDE.

1. User types in name of BLE device
2. Console App finds and connects to the BLE device
3. Console App searches and saves the BLE characteristics related to SPP
4. Console App subscribes to SPP_data notifications and adds a callback to print the text whenever device notifies Windows
5. Whenever user types on the keyboard, the keystroke is saved into the SPP_data characteristic
