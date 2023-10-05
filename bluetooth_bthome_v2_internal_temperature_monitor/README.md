# Bluetooth - BTHome v2 - Internal Temperature Monitor #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_internal_temperature_monitor_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_internal_temperature_monitor_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_internal_temperature_monitor_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_internal_temperature_monitor_common.json&label=SDK&query=sdk&color=green)
[![Required board](https://img.shields.io/badge/Sparkfun-Thing%20Plus%20MGM240P-green)](https://www.sparkfun.com/products/20270)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_internal_temperature_monitor_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_internal_temperature_monitor_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_bthome_v2_internal_temperature_monitor_common.json&label=RAM&query=ram&color=blue)
## Overview ##

The example showcases the implementation of BTHome support for Silabs development kits.

The example application illustrates how BTHome can be effectively utilized with Silabs development kits to communicate internal temperature sensor values to a HomeAssistant setup running on a Raspberry Pi 4. This demonstration will provide developers with insights into integrating BTHome with Silabs hardware for IoT applications.

![overview](images/overview.png)

The BTHOME v2 sensor device is a BGM220 Explorer Kit that reads the internal temperature sensor and sends BLE advertisement packets in the BTHome v2 format.

Raspberry Pi 4 runs a Home Assistant OS that scans for and detects BTHome v2 sensor device. Users use the Home Assistant application on the smartphone to communicate with Raspberry Pi 4 to get the advertisement packet of the BTHome v2 sensor device, parse and display it on a smartphone.

## Gecko SDK Version ##

- GSDK v4.3.1
- [Third Party Hardware Drivers v1.8.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Hardware Required ##

- [**BGM220-EK4314A** BGM220 Bluetooth Module Explorer Kit - BRD4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)

**NOTE:**
Tested boards for working with this example:

| Board ID | Description  |
| -------- | ------ |
| BRD2601B | [EFR32xG24 Dev Kit](https://www.silabs.com/development-tools/wireless/efr32xg24-dev-kit?tab=overview)  |
| BRD2703A | [EFR32xG24 Explorer Kit - XG24-EK2703A ](https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit?tab=overview)    |
| BRD4314A | [BGM220 Bluetooth Module Explorer Kit - BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit?tab=overview)  |
| BRD4108A | [EFR32BG22 Explorer Kit Board](https://www.silabs.com/development-tools/wireless/bluetooth/bg22-explorer-kit?tab=overview)  |
| BRD2704A | [Sparkfun Thing Plus MGM240P](https://www.sparkfun.com/products/20270)  |

## Connections Required ##

The following picture shows the connection for this application:

![connection](images/connection.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

### Create a project based on an example project ###

1. From the Launcher Home, add your product name to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with the filter "bthome v2".

2. Click **Create** button on **Bluetooth - BTHome v2 - Internal Temperature Monitor** example. Example project creation dialog pops up -> click Create and Finish and the project should be generated.

    ![Create project from example](images/create_project_from_example.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all the `src/app.c` file into the project root folder (overwriting the existing file).

3. Install the software components:

    - Open the .slcp file in the project

    - Select the SOFTWARE COMPONENTS tab

    - Install the following components:
    
      - [Platform] → [Driver] → [TEMPDRV]
      - [Third-Party Hardware Drivers] → [Services] → [BTHome v2]

4. Build and flash the project to your board.

**Note:**

- Make sure the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is added to the required SDK: [Preferences > Simplicity Studio > SDKs](https://github.com/SiliconLabs/third_party_hw_drivers_extension/blob/master/README.md#how-to-add-to-simplicity-studio-ide).

  ![add Third Party Hardware Drivers extension](images/sdk_extension.png)

- SDK Extension must be enabled for the project to install the [BThome v2] component.

- Do not forget to flash a bootloader to your board, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

### Application Initialization ###

![application initialization](images/app_init.png)

### BLE System Boot Event ###

![System boot event flow](images/system_boot_event.png)

### Periodic Timer Callback ###

![Timer callback handle](images/timer_handle.png)

## Testing ##

To test this application and monitor the temperature, you need the following materials:
  - Raspberry Pi 4 runs Home Assistant OS.
  - Home Assistant application on a smartphone.
  - BTHOME sensor device, which is BGM220 Explorer Kit runs the **Bluetooth - BTHome v2 - Internal Temperature Monitor** example.

*Note*: To utilize the BTHOME sensor device, you have two options. You can use Silabs development kits and run either the "BTHome v2 - Internal Temperature Monitor" example or create your own project by following the instructions provided in the "Setup" section above.

1. Power on Raspberry Pi 4 and BTHOME sensor device. After powering on, the BTHOME sensor device sends the advertisement packet every 10 seconds that contains the temperature value.

    ![Sensor initialization](images/sensor_initialize_log.png)

2. Open the Home Assistant application on the smartphone, select [Settings] → [Devices and Services] → [Add Integration]

    ![Setting on the home assistant application](images/app_setting.png)

3. Add Integration with the name 'BTHome'. You can see the list of device, which is advertising in BTHome format. Choose your device with the correct name (it is 'Temp 79B4' in this example) and submit the Bindkey, which is defined in the firmware of the BTHOME sensor device. *Note: To be able to find your sensor device with the home assistant application, you need to use the same network on both Raspberry Pi 4 and the smartphone.*

    ![Add a device with Name and Bindkey](images/add_device.png)

    The name and Bindkey in the firmware of the sensor device.

    ![Check name and Bindkey in the firmware of sensor device](images/bindkey_sensor_device.png)

4. After adding your sensor device successfully with the Bindkey, select a suitable area where your sensor device is located.

    ![Set area for sensor device and track temperature](images/track_sensor_value.png)

5. Now you can see your sensor is already added to the Home Assistant system. You can track the temperature value from your sensor by selecting your device in BTHome.
