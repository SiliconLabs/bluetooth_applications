# Bluetooth - Door Lock Fingerprint (A-172-MRQ) #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_door_lock_fingerprint_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_door_lock_fingerprint_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_door_lock_fingerprint_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_door_lock_fingerprint_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_door_lock_fingerprint_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_door_lock_fingerprint_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_door_lock_fingerprint_common.json&label=RAM&query=ram&color=blue)
## Overview ##

This example is used for a BLE-based device lock system using one Silicon Labs EFR32xG24 Explorer kit and various hardware components such as MikroE Fingerprint Click 2 (A-172-MRQ) and SparkFun Micro OLED Breakout (Qwiic). The system is using BLE for wireless communication. This example also is the foundation to develop a secure and convenient device system using biometric authentication (fingerprint) and LED display indication. The system will use BLE for wireless communication between the Silicon Labs board and the EFR connect mobile application.

## Gecko SDK Suite version ##

- GSDK v4.2.1

## Hardware Required ##

- [EFR32xG24-EK2703A Explorer Kit Board](https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit?tab=overview)

- [SSD1306 OLED display (SparkFun OLED Display)](https://www.sparkfun.com/products/14532)

- [A-172-MRQ - Fingerprint 2 Click](https://www.mikroe.com/fingerprint-2-click)

**NOTE:**
Tested boards for working with this example:

| Board ID | Description  |
| ---------------------- | ------ |
| BRD2703A | [EFR32xG24 Explorer Kit - XG24-EK2703A ](https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit?tab=overview)    |
| BRD4314A | [BGM220 Bluetooth Module Explorer Kit - BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit?tab=overview)  |
| BRD4108A | [BG22 Bluetooth SoC Explorer Kit - BG22-EK4108A](https://www.silabs.com/development-tools/wireless/bluetooth/bg22-explorer-kit?tab=overview)  |

## Connections Required ##

The following picture shows the system view of how it works.

![connection_required](images/connection_required.png)

The SparkFun OLED Display board can be easily connected to the EFR32 xG24 Explorer Kit by using a Qwiic cable. The Fingerprint 2 Click connects to the EFR32xG24 Explorer Kit using USART via MikroE connection.

## Set up ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

### Create a project based on an example project ###

1. From the Launcher Home, add the your hardware to **My Products**, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with filter "door lock fingerprint".

2. Click **Create** button on the **Bluetooth - Door Lock Fingerprint (A-172-MRQ)** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

![create_project](images/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing).

3. Import the GATT configuration:

    - Open the .slcp file in the project.

    - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.
    
    - Find the Import button and import the attached [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.

    - Save the GATT configuration (ctrl-s).

4. Open the .slcp file. Select the **SOFTWARE COMPONENTS** tab and install the software components:

    - [Services] → [IO Stream] → [IO Stream: EUSART] → default instance name: vcom

    - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: mikroe

    - [Application] → [Utility] → [Log]

    - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: lock. Then, configure pin as bellow:

    ![led_lock_config](images/led_lock_config.png)

    - [Third Party Hardware Drivers] → [SparkFun] → [Human Machine Interface] → [A-172-MRQ - Fingerprint 2 Click (Mikroe)]

    - [Third Party Hardware Drivers] → [SparkFun] → [Display & LED] → [SSD1306 - Micro OLED Breakout (SparkFun) - I2C]

    - [Third Party Hardware Drivers] → [Services] → [GLIB Graphics Library]

    

5. Build and flash the project to your device.

**Note:**

- Make sure the [SDK extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension/blob/master/README.md) already be installed and this repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- Do not forget to flash a bootloader to your board, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information

## How It Works ##

### Application Overview ###

![application_overview](images/application_overview.png)

### Application Logic ###

![application_logic](images/application_logic.png)

### GATT Database ###

Advertisement Packet Device name: **FINGERPRINT DOORLOCK**

GATT Database

- Device name: **FINGERPRINT DOORLOCK**

- **[Service] Fingerprint Door Lock**

    - **[Char] Mode**

        - [R] Get mode value.
        - [W] Set operation mode (0 - normal, 1 - register, remove, 2 - show).
    
    - **[Char] Remove Authorized Fingerprint**

        - [W] Provide authorized fingerprint id to remove it from the authorized fingerprints.

    - **[Char] Get Fingerprint ID**

        - [W] Select the index.

        - [R] Provide the status of the index (**EMPTY** or **EXISTS**).
    
    - **[Char] Open Lock**
    
        - [W] Write 1 to open lock.

### Application initialization ###

![application_init](images/application_init.png)

### Runtime - Normal Mode ###

![normal_mode](images/normal_mode.png)

### Runtime - Configuration Mode 1 - Register/Remove Fingerprint ###

![configuration_mode](images/configuration_mode.png)

### Runtime - Configuration Mode 2 - Show Authorized Fingerprints ###

![show_mode](images/show_mode.png)

### Display ###

In normal mode, the OLED displays the BLE connection status (CONNECTED or DISCONNECTED) as well as the door status (LOCKED or UNLOCKED). If the door is unlocked by a user's fingerprint, the screen will display the index of this fingerprint.

![normal_mode_display](images/normal_mode_display.png)

In configuration mode 1, the OLED will show "CONFIG" on the screen and the number of fingerprints that have already been authorized. There are two methods in this mode:

- When the user adds the new fingerprint, the screen will show the "ADDED" label and the index of this fingerprint.

- Otherwise, when the fingerprint is removed by the user, the OLDE will show the "REMOVED" label and the index of the removed fingerprint, which was stored on the Fingerprint 2 Click board and NVM memory.

![config_mode_display](images/config_mode_display.png)

In configuration mode 2, if the fingerprints were stored in NVM memory and on the Fingerprint 2 Click board, the OLED would show the index of each fingerprint in 4 seconds. If do not have any authorized fingerprints, the OLED will show "-" and "NO FPs".

![show_mode_display](images/show_mode_display.png)

### Testing ###

You can use a smartphone app, such as the EFR Connect application on your phone, to connect to the board. Please, following some steps as below:

- Open the EFR Connect app.

- Open the Bluetooth Browser.

- Find the device advertising as **FINGERPRINT DOORLOCK**.

- Click on **Connect** button.

![efr_connect_app](images/efr_connect_app.png)

Upon reset, the application will display the Silicon Labs logo on the OLED screen for a few seconds. After the firmware is configured, the device starts in normal mode. In this state, it starts advertising devices. Open your terminal emulator and connect to your client device over its serial port. Set baudrate to 115200. A similar output as below:

![termial_start](images/terminal_start.png)

#### Normal Mode ####
When you place your finger on the sensor in normal mode, the system compares your fingerprint and returns the result. Please see the terminal output as follows: 

![terminal_normal](images/terminal_normal.png)

#### Configuration Mode 1 ####
In configuration mode 1, you can get the index of authorized fingerprints by reading **Get Fingerprint ID** characteristic. After that, you can delete the fingerprint that you no longer use. The terminal output is as follows:

![terminal_remove](images/terminal_remove.png)

Moreover, in this mode, when you place the new finger, that is not authorized. The system will then assist you in registering this fingerprint if it has enough storage space. The terminal output is as follows:

![terminal_add](images/terminal_add.png)

#### Configuration Mode 2 ####

The OLED screen will show the index of the authorized fingerprints. Please take a look at the picture in the paragraph marked **Display**.


