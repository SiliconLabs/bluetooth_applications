# Bluetooth - Data Logger SD Card

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Mikroe-Silabs%20Click%20Shield-green)](https://www.mikroe.com/silabs-click-shield)
[![Required board](https://img.shields.io/badge/Mikroe-microSD%20Click-green)](https://www.mikroe.com/microsd-click)
[![Required board](https://img.shields.io/badge/Sparkfun-Micro%20OLED%20Breakout%20(Qwiic)%20board-green)](https://www.sparkfun.com/products/14532)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-235.16%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-14.64%20KB-blue)

## Overview

This project aims to implement a data logger application using Silicon Labs development kits integrated with the BLE wireless stack and humidity & temperature sensors (si72xx).

The block diagram of this application is shown in the image below:

![overview](image/overview.png)

More detailed information can be found in the section [How it works](#how-it-works).

This code example referred to the following code examples. More detailed information can be found here:

- [SSD1306 - SparkFun Micro OLED Breakout](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/silabs/micro_oled_ssd1306)
- [External Storage - microSD Click (Mikroe)](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/mikroe/microsd)

## SDK version

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required

- 1x [xG24-DK2601B](https://www.silabs.com/development-tools/wireless/efr32xg24-dev-kit?tab=overview) EFR32xG24 Dev Kit
- 1x [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532)
- 1x [microSD Click](https://www.mikroe.com/microsd-click)
- 1x [Silabs Click Shield](https://www.mikroe.com/silabs-click-shield)
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required

The hardware connection is shown in the image below:

![hardware connection](image/hardware_connection.png)

- The **xG24 Dev Kit** and the **microSD Click** can be connected to the [Mikroe Silabs Click Shield](https://www.mikroe.com/silabs-click-shield) via the Thunderboard socket and the mikroBus respectively, or we can use some **female-to-female jumper wire** to connect them directly as shown in the table below

  | xG24 Dev Kit | microSD Click |
  | --- | --- |
  | EXP10 - SPI_CS - PA7 (pin 10) | CS |
  | EXP8 - SPI_SCLK - PC1 (pin 8) | SCK |
  | EXP6 - SPI_MISO - PC2 (pin 6) | SDO |
  | EXP4 - SPI_MOSI - PC3 (pin 4) | SDI |
  | - | CD |
  | GND - EXP1 (pin 1) | GND |
  | EXP2 - VMCU (pin 2) | +3V3 |

  The xG24 Dev Kit  and microSD Click pinout diagram is shown below:

  | xG24 Dev Kit | microSD Click |
  | --- | --- |
  | ![xG24 Dev Kit diagram](image/xg24_dev_kit.png) | ![microSD Click](image/microsd_click.png) |

- **Micro OLED** can be easily connected via qwiic to the xG24 Dev Kit

  Mikroe Silabs Click Shield EXP header pinout diagram is shown below:

  ![Mikroe Silabs Click Shield EXP header](image/silabs_click_shield.png)

  Micro OLED 2.5mm Header pinout diagram is shown below:

  ![Micro OLED 2.5mm Header printout](image/micro_oled_qwiic_header.png)

## Setup

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware. You should connect the xG24 Dev Kit to the PC using a MicroUSB cable.

**NOTE**:

- Make sure that the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is installed as part of the SiSDK and the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- SDK Extension must be enabled for the project to install the required components.

### Create a project based on an example project

1. From the Launcher Home, add the BRD2601B  to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by 'data logger'.

   ![create_demo](image/create_project.png "Create a project based on an example project")

2. Click **Create** button on the **Bluetooth - Data Logger SD Card** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project

1. Create a **Bluetooth - SoC Empty** project for the **xG24 Dev Kit** using Simplicity Studio 5.

2. Copy all attached files in the "inc" and the "src" folders into the project root folder (overwriting existing files).

3. Import the GATT configuration:

   - Open the .slcp file in the project.

   - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

   - Find the Import button and import the attached [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.

   - Save the GATT configuration (ctrl-s).

4. Install the software components:

   - Open the .slcp file in the project.

   - Select the SOFTWARE COMPONENTS tab.

   - Install the following components:

      - [Services] → [Timers] → [Sleep Timer] → Set "Enable wallclock functionality"
      - [Bluetooth] → [Bluetooth Host (Stack)] → [Additional Features] → [NVM Support]
      - [Bluetooth] → [Application] → [Miscellaneous] → [Relative Humidity and Temperature sensor]
      - [Services] → [IO Stream] → [IO Stream: USART] → vcom
      - [Application] → [Utility] → [Log]
      - [Platform] → [Driver] → [I2C] → [I2CSPM] → qwiic
      - [Platform] → [Driver] → [I2C] → [I2CSPM] → sensor
      - [Platform] → [Driver] → [SPI] → [SPIDRV] → exp
      - [Platform] → [Driver] → [Button] → [Simple Button] → btn0
      - [Platform] → [Board Driver] → [Si70xx - Temperature/Humidity Sensor]
      - [Platform] → [Board] → [Board Control] → Enable Relative Humidity and Temperature sensor
      - [Third Party Hardware Drivers] → [Storage] → microSD - microSD Click (Mikroe)
      - [Third Party Hardware Drivers] → [Display & LED] → [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C]
      - [Third Party Hardware Drivers] → [Services] → [GLIB - OLED Graphics Library]
      - [Third Party Hardware Drivers] → [Services] → [FatFS - Generic FAT Filesystem] → Set "Enable f_mkfs() function"
      - [Bluetooth] → [OTA] → [In-Place OTA DFU] → uninstall
      - [Platform] → [Bootloader Application Interface] → uninstall.

5. Build and flash the project to your device.

    *Note*: Flash the application image to the device by using the .hex or .s37 output file.

## How it Works

### Application Overview

![Application Overview](image/application_overview.png)

### GATT Configurator

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT re-uses the [SPP over BLE](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_secure_spp_over_ble) custom service:

- [Service] **SPP Service**: UUID `4880c12c-fdcb-4077-8920-a450d7f9b907`
  - [Char] **SPP Data**: UUID `fec26ec4-6d71-4442-9f81-55bc21d658d6`
    - [**Notifiable**] - Get notification of log data

### Data logger Implementation

#### Application initialization

![Application init](image/application_init.png)  

#### Device connect event

![Device connect event](image/device_connect_event.png)  

#### Periodic timer event

![Periodic timer event](image/priodic_timer_event.png)

#### Application Workflows

1. Initialize the peripherals, the Bluetooth stack

2. Get the logger to enable configuration from nvm

3. Initialize the OLED display

4. Mount file system on the SD card

    - If failure to mount the file system then re-format it (make a new file system on the SD card)

5. Initialize the humidity & temperature sensor si7021

6. Start a periodic timer with period 10s. The timer callback will fire an external event to the BLE stack and the event handler will do:

    - Get the sensor humidity & temperature sample.
    - Update the OLED display with the sensor data
    - Make a log entry from sensor data.
    - Check the state of the BLE connection:
      - If the receiver is connected and the notification is enabled then send the log entry to the receiver over the BLE notification
      - If no BLE connection is made or the notification is not enabled:
        - If the logger is enabled then append the log entry to the sd card

7. When a new BLE connection is made and notification of the **SPP data** characteristic is enabled then:
    - If the log file exists on the sd card then all the logs on that file will be sent line by line over BLE notification of the **SPP data** characteristic. After that, the log file will be deleted
    - If the log file does not exist or after sending all the log data from a file, the new log data is continuing sent over BLE notification of the **SPP data** characteristic

8. When pressing button 0 the callback will fire an external event to the BLE stack and the event handler will do:
  
    - If the logger has been disabled then enable the data logger and save the config to the NVM.
    - If the logger has been enabled then disable the data logger and save the config to the NVM.

### OLED Display

- Display current sensor data
  
  ![OLED display](image/oled_display.png)

### Button

- Press the button to enable/disable the logger

### LED

- Indicate wether the logger is enabled or disabled

### Log data format

- The log data is sent line by line. They are separated by a line feed character.
- The log line format:

> yyyy/mm/dd hh:mm:ss Humidity = \<humidity_value_percentage\> RH, Temperature = \<temperature_value_degree\> C

### SPP Data notification

- To view the log data, we have 2 options:

  - Use **Simplicity Connect Mobile Application** to enable notification, the log data is received as an ASCII string

  - Use the [BLE SPP app for Windows](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_spp_with_windows) on a laptop or PC that has a Bluetooth adapter. First, we need to enter the name of the data logger (the default name is **"Data Logger"**). You should expect a similar output to the one below.

    ![Log View](image/log_view.gif)
