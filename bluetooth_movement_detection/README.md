# Movement Detection application with BLE #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_movement_detection_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_movement_detection_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_movement_detection_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_movement_detection_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_movement_detection_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_movement_detection_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_movement_detection_common.json&label=RAM&query=ram&color=blue)

## Overview ##

This project aims to implement a motion detection example application, the motion detection algorithm and its software module will be used as a building block for the Asset Tracking application.

The block diagram of this application is shown in the image below:

![system overview](images/system_overview.png)

This code example referred to the following code examples. More detailed information can be found here:

- [BMA400 accelerometer driver](https://github.com/SiliconLabs/platform_hardware_drivers/tree/master/bma400_accelerometer)
- [Bluetooth security feature](https://github.com/SiliconLabs/bluetooth_stack_features_staging/tree/master/security)

## Gecko SDK Suite version ##

GSDK v4.0.2

## Hardware Required ##

- [BGM220 Explorer Kit board](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)

- [**MikroE Accel 5 click** ultra-low power triaxial accelerometer sensor](https://www.mikroe.com/accel-5-click)

## Connections Required ##

The mikroBUS Accel 5 Click board can be just "clicked" into it place. Be sure that the boards 45-degree corner matches the Explorer Kit's 45-degree white line. The board also has 4.7k I2C-bus pull-ups. Just be sure that the click board is configured into I2C-mode by the resistors and not into SPI-mode. Also the application uses by default **I2C address 0x15** as it is the Accel 5 click default (the resistor labeled "I2C ADD" is on the "1". If setting "I2C ADD" resistor "0", the address will be 0x14).

![hardware connection](images/hardware_connection.png)

## Setup ##

To test this application, you can either import the provided `bluetooth_movement_detection.sls` project file or start with an empty example project as the following:

1. Create a **Bluetooth - SoC Empty** project for the **BGM220 Explorer Kit Board** using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing app.c).

3. Import the GATT configuration:

   - Open the .slcp file in the project.

   - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

   - Find the Import button and import the attached [gatt_configuration.btconf](config/gatt_configuration.btconf) file.

   - Save the GATT configuration (ctrl-s).

4. Open the .slcp file again. Install and configure the following components:

    - [Services] →  [Sleep Timer]
    - [Bluetooth] → [NVM] → NVM Support
    - [Services] → [NVM3] → NVM3 Core
    - [Services] → [NVM3] → NVM3 Default Instance
    - [Services] → [IO Stream] → [IO Stream: USART] → Default instance name: **vcom**
    - [Application] → [Utility] → Log
    - [Platform] → [Driver] → [I2C] → [I2CSPM] → Default instance name: **mikroe**
    - [Platform] → [Driver] → [Button] → [Simple Button] → Default instance name: **btn0**
    - [Platform] → [Driver] → [LED] → [Simple LED] → Default instance name: **led0**

5. Build and flash the project to your device.

Note: You need to create the bootloader project and flash it to the device before flashing the application. When flash the application image to the device, use the .hex or .s37 output file. Flashing the .bin files may overwrite (erase) the bootloader.

## How it Works ##

### Application Overview ###

![app overview](images/overview.png)

### GATT Configurator ###

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT changes were adding a new custom service (Movement Detection) using UUID `134c8216-77a5-40c8-a640-bca22c27fc77` which are 4 characteristics:

- [Service] **Movement Detection** - `134c8216-77a5-40c8-a640-bca22c27fc77`
  - [Char] **Movement Threshold** - `4addc963-47f9-44cb-8c6c-f42390b7fc07`
    - [R] Get the movement threshold value
    - [W] Set the movement threshold value
  - [Char] **Wake-Up Time Period** - `a1740841-0136-4f40-9d82-ad0993d25c9c`
    - [R] Get the upper threshold value
    - [W] Set the lower threshold value
  - [Char] **Notification Time** - `0370cdec-eea4-40fd-975b-1a7fe3f0222d`
    - [R] Get the threshold mode
    - [W] Set the threshold mode
  - [Char] **Notification Break Time** - `75896182-8482-4626-9b2c-073afa537156`
    - [R] Get configured buzzer volume
    - [W] Set buzzer volume

Where R = Readable, W = Writeable with response.

### Movement Detection implementation ###

#### Initialization ####

Application initialization function is invoked from the *app_init()* function at startup. The initialization can be divided into phases as follows:

- Initialization Phase 1 - General

  ![General Mode](images/general_mode.png)

- Initialization Phase 2/A - Normal Mode

  ![Normal Mode](images/normal_mode.png)

- Initialization Phase 2/B - Configuration Mode

  ![Configuration Mode](images/configuration_mode.png)

#### Application Logic ####

![Application Logic](images/application_logic.png)

#### Movement Detection algorithm ####

In general the device and the sensor is in sleep mode, when the accelerometer detects that the device is moving, it will wake-up the host MCU triggering an external interrupt via a GPIO pin. When the device is woke-up the movement detection algorithm is active and if the movement is above the threshold, then the the host MCU starts blinking the LED on the development board.

![Movement Detection](images/movement_detection.png)

### Testing ###

Upon reset, the device starts in **Normal Mode**. In this state, the device runs the movement detection algorithm without the Bluetooth. Try to move the device in some direction and check the logs on the terminal.

  ![logs](images/logs_1.png)

To switch to **Configuaration Mode**, button PB0 should be pressed during startup (power-on or reset).

Follow the below steps to test the example with the EFR Connect app when the device is in **Configuaration Mode**:

1. Open the EFR Connect app on your iOS/Android device.

2. Find your device in the Bluetooth Browser, advertising as **Movement Detection**, and tap Connect. For iOS devices, enter the passkey (passkey default as **123456**) to confirm authentication for the pairing process for the first time. For Android devices, the user must accept a pairing request first and do as above. After that, wait for the connection to be established and the GATT database to be loaded.

   **Note**: The pairing process on Android and iOS devices is different. For more information, refer to [bluetooth security](https://github.com/SiliconLabs/bluetooth_stack_features/tree/master/security).

3. Find the unknown service at the above of the OTA service.

4. Tap on the main service to see the available characteristics. Try to read, write, re-read the characteristics, and check the value. Values for the characteristics are handled by the application as ASCII strings. You should expect a similar output to the one below.

    ![logs](images/logs_2.png)

## .sls Projects Used ##

- [bluetooth_movement_detection.sls](SimplicityStudio/bluetooth_movement_detection.sls)
