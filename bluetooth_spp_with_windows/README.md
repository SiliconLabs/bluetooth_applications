# BLE SPP with Windows #

## Summary ##

This project uses Windows BLE API to communicate with the [SPP-over-BLE KBA](https://www.silabs.com/community/wireless/bluetooth/knowledge-base.entry.html/2017/04/13/spp-over-ble_c_examp-mnoe) example. A custom Windows Console App was created that acts like TeraTerm and PuTTY but uses BLE on the back end.

## Hardware Required ##

- One radio board which can run the SPP-over-BLE example
- One BRD4001A WSTK board
- A Windows PC

## Setup ##

Program the radio board with the SPP-over-BLE example code provided with the KBA. Connect the BLE device to the computer and open a COM port in the terminal application of your choice. Launch the provided executable and type in the name of the BLE device. The device should connect to the Windows machine, and they will be able to communicate. Typing in either console should send the data to the other.

## How It Works ##

This project was inspired by a community post which the user asked if there were examples of a BLE device communicating with the computer wirelessly. I thought substituting the VCOM USB communication to a computer terminal (PuTTY) with a Bluetooth wireless communication to a computer terminal would help debug if the device is not accessible. The application shows how printf is retargeted to use BLE and how to get keyboard inputs from the console on the device.

Since there is no official Windows Console Apps that have this functionality, a custom console app was created using Visual Studio. An executable file of the project allows any user to use this without having to download the Visual Studio IDE.

1. User types in name of BLE device
2. Console App finds and connects to the BLE device
3. Console App searches and saves the BLE characteristics related to SPP
4. Console App subscribes to SPP_data notifications and adds a callback to print the text whenever device notifies Windows
5. Whenever user types on the keyboard, the keystroke is saved into the SPP_data characteristic
