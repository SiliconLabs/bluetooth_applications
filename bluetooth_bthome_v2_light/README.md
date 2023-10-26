# Bluetooth - BTHome v2 - Light

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_light_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_light_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_light_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_light_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_light_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_light_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_light_common.json&label=RAM&query=ram&color=blue)
## Overview

![logo](images/logo.png)

BTHome is an energy-efficient, but flexible BT format for devices to broadcast their sensor data and button presses.

Devices can run over a year on a single battery. It allows data encryption and is supported by popular home automation platforms, like Home Assistant, out of the box.

For more information, please visit [BThome](https://bthome.io/).

This project aims to implement a BTHome v2 compatible light. The application provides a CLI to configure switches to control the onboard LED0, it supports the press event only.

One or more Switches can control a Light. Each Switch acts as a BTHome client which advertises button press event to control Light. This device should use the *BTHome v2 - Switch* example for testing in this project.

The Light device acts as a BTHome v2 Client and Server (be implemented in this project with name: *BTHome v2 - Light*):
- As a server, it gathers the Button events from the registered devices to control Light.
- As a client, it reports the status of the Light to Home Assistant.

![connection](images/connection2.png)

## Gecko SDK version

- GSDK v4.3.1
- [Third Party Hardware Drivers v1.8.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Hardware Required

- Two [**BGM220-EK4314A** BGM220 Bluetooth Module Explorer Kit (BRD4314A BGM220 Explorer Kit Board)](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
  - One BGM220 run *BTHome v2 - Light*
  - The other BGM220 run *BTHome v2 - Switch*
- Raspberry Pi 4 running Home Assistant. For install process follow [here](https://www.home-assistant.io/installation/raspberrypi)
- Smartphone install Home Assistant app

## Connections Required

The hardware connection is shown in the image below:

![connection](images/connection1.png)

## Setup

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

### Create a project based on an example project

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with the filter "bthome".

2. Click **Create** button on **Bluetooth - BTHome v2 - Light** examples. Example project creation dialog pops up -> click Create and Finish and Project should be generated.
![create_project](images/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all the .h and .c files to the project root folder (overwriting the existing file).

3. Install the software components:

    - Open the .slcp file in the project

    - Select the SOFTWARE COMPONENTS tab

    - Install the following components:

      - [Third-Party Hardware Drivers] → [Services] → [BTHome v2]
      - [Third-Party Hardware Drivers] → [Services] → [BTHome v2 - Server]
      - [Services] → [Command Line Interface] → [CLI Instance(s)] → default instance name: inst
      - [Platform] → [Driver] → [Button] → [Simple Button] → default instance name: btn0.
      - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: led0.
      - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: vcom
      - [Application] → [Utility] → [Log]
4. Build and flash the project to your device.

**Note:**

- Make sure that the SDK extension is already installed. If not please follow [this documentation](https://github.com/SiliconLabs/third_party_hw_drivers_extension/blob/master/README.md#how-to-add-to-simplicity-studio-ide).

- Third-party Drivers Extension must be enabled for the project to install "BTHome v2" and "BTHome v2 - Server" component.

## How It Works

You have one or more switches controlling one light. You would use in a hallway, where you have a switch at both ends of the hallway that controls the hallway light. You are able to switch the light on and off from either end of the hallway.

**Application Initialization**

![application_initialization](images/application_init.png)

**BTHome v2 Events**

![bthome_v2_events](images/bthome_v2_events.png)

The example implements a CLI Interface that provide some features:

- Scan BLE network and list BTHome devices. List them with the following parameters:
  - MAC - Encryption (Yes/No) - Encryption Key Available (Yes/No)
- List registered devices
  - MAC - Encryption Key
- Register key by MAC
- Remove key by MAC
- Add device to the interested devices by MAC
- Remove device from the interested devices by MAC

## Testing

For testing, there should be at least 2 Silabs boards. One acts as a Switch device which use the "BTHome v2 - Switch" example. One acts as a Light device.

### Silicon Labs Devices

1. Open a console or a terminal program (e.g., TeraTerm) and connect to the device to see the logs

2. Type `help` to see the supported commands:

    ![help](images/help.png)

3. Start scanning to find BThome v2 devices. A continuous scan will update the newest data on devices. You can check if the device found is *BTHome v2 - Switch*

    ![scan_start](images/scan_start.png)

4. You can register the *BTHome v2 - Switch* device using the `key_register` command

    ![key](images/key.png)

5. Now you can control the LED using a button from the *BTHome v2 - Switch* device

    ![light_system](images/light_system.png)

6. You can also add another button by following the same process.

### App Home Assistant

1. The **Home Assistant** application utilizes the Bluetooth adapter on your phone/tablet to scan BLE devices.

    ![app1](images/app1.png)

2. Open the *Home Assistant* application on your smartphone. Click [Settings] → [Devices & Services] → [BTHome], and you will see a list of nearby devices, which are sending BTHome advertisements. Find the one named "BTLight" and click the *ADD ENTRY*. Enter the BindKey then submit, add device to your home.

    ![app2](images/app2.png)

3. Automations in Home Assistant allow you to automatically respond to things that happen. You can check whenever the light is on or off. For this example, we will create two simple automations to receive signals from devices and show them on the dashboard.

    ![app3](images/app3.png)

## Report Bugs & Get Support

To report bugs in the Application Examples projects, please create a new "Issue" in the "Issues" section of [third_party_hw_drivers_extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) repo. Please reference the board, project, and source files associated with the bug, and reference line numbers. If you are proposing a fix, also include information on the proposed fix. Since these examples are provided as-is, there is no guarantee that these examples will be updated to fix these issues.

Questions and comments related to these examples should be made by creating a new "Issue" in the "Issues" section of [third_party_hw_drivers_extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) repo.