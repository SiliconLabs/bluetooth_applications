# Environment humidity and temperature data logger with BLE
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_data_logger_sd_card_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_data_logger_sd_card_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_data_logger_sd_card_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_data_logger_sd_card_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_data_logger_sd_card_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_data_logger_sd_card_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_data_logger_sd_card_common.json&label=RAM&query=ram&color=blue)

## Overview

This project aims to implement a data logger application using Silicon Laboratories development kits integrated with the BLE wireless stack and a humidity & temperature sensors (si72xx).

The block diagram of this application is shown in the image below:

![overview](images/overview.png)

More detailed information can be found in the section [How it works](#how-it-works).

This code example referred to the following code examples. More detailed information can be found here:

- [OLED SSD1306 driver](https://github.com/SiliconLabs/platform_hardware_drivers/tree/master/oled_ssd1306_i2c)
- [External Storage - SD card driver](https://github.com/SiliconLabs/platform_hardware_drivers/tree/master/storage_sd_card)

## Gecko SDK Suite version

GSDK v4.1.0

## Hardware Required

- [Thunderboard Sense 2](https://www.silabs.com/development-tools/thunderboard/thunderboard-sense-two-kit)

- [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532)

- [MICRO SD CLICK](https://www.mikroe.com/microsd-click)

## Connections Required

The hardware connection is shown in the image below:

![hardware connection](images/hardware_connection.png)

- The **Thunderboard sense 2** and the **MICRO SD CLICK** can be plugged into the [Mikroe Silabs Click Shield](https://www.mikroe.com/silabs-click-shield) via the Thunderboard socket and the mikroBus respectively or we can use some **female to female jumper wire** to connect them directly as shown in the table below

  | Thunderboard sense 2 | MICRO SD CLICK |
  | --- | --- |
  | EXP10 - SPI_CS - PA5 (pin 10) | CS |
  | EXP8 - SPI_SCLK - PF7 (pin 8) | SCK |
  | EXP6 - SPI_MISO - PK2 (pin 6) | SDO |
  | EXP4 - SPI_MOSI - PK0 (pin 4) | SDI |
  | PA6 - EXP7 (pin 7) | CD |
  | GND - EXP1 (pin 1) | GND |
  | EXP2 - VMCU (pin 2) | +3V3 |

  The Thunderboard sense 2  and MICRO SD CLICK pinout diagram is shown as below
  | Thunderboard sense 2 | MICRO SD CLICK |
  | --- | --- |
  | ![Thunderboard sense 2 diagram](images/thunderboardsense2.png) | ![MICRO SD CLICK](images/microsd_click.png)  

- We can use some **female to female jumper wire** to connect **Micro OLED** and the **Thunderboard sense 2** or we can connect them through the [Mikroe Silabs Click Shield EXP header](https://www.mikroe.com/silabs-click-shield) with some **male to female jumper wire** as shown in the table below:

  | Thunderboard sense 2 | Micro OLED 2.5mm Header |
  | --- | --- |
  | PC11 - I2C_SCL - EXP15 (pin 15) | SCL (pin 4) |
  | EXP16 - I2C_SDA - PC10 (pin 16) | SDA (pin 3) |
  | EXP2 - VMCU (pin 2) | 3V3 (pin 2) |
  | GND - EXP1 (pin 1) | GND (pin 1) |

  Mikroe Silabs Click Shield EXP header pinout diagram is shown as below

  ![Mikroe Silabs Click Shield EXP header](images/silabs_click_shield.png)

  Micro OLED 2.5mm Header pintout diagram is shown as below

  ![Micro OLED 2.5mm Header pintout](images/micro_oled_qwiic_header.png)

## Setup

To test this application, you can either import the provided `bluetooth_data_logger_sd_card.sls` project file or start with an empty example project as the following:

1. Create a **Bluetooth - SoC Empty** project for the **Thunderboard Sense 2** using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing app.c).

3. Import the GATT configuration:

   - Open the .slcp file in the project.

   - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

   - Find the Import button and import the attached [gatt_configuration.btconf](config/gatt_configuration.btconf) file.

   - Save the GATT configuration (ctrl-s).

4. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    - Install **[Platform] > [Driver] > [I2CSPM]** component with the default instance name: **qwiic**. Set this component to use I2C1 peripheral, SCL to PC11 pin, SDA to PC10 pin.

    - Install **[Platform] > [Driver] > [SPIDRV]** component with the default instance name: **exp**.

    - Install **[Application] > [Sensor] > [Relative Humidity and Temperature sensor]**

    - Install **[Platform] > [IO Stream] > [IO Stream: USART]** component with the default instance name: **vcom**.

    - Install **[Platform] > [Driver] > [Button] > [Simple Button]** component with the default instance name: **btn0**.

    - Install **[Platform] > [Driver] > [LED] > [Simple LED]** component with the default instance name: **led0**.

    - Install **[Bluetooth] > [NVM] > [NVM Support]** component.

    - Install **[Application] > [Utility] > [Log]** component.

5. Build and flash the project to your device.

    *Note*: Flash the application image to the device by using the .hex or .s37 output file.

## How it Works

### Application overview

![Application overview](images/application_overview.png)

### GATT Configurator

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT re-use the [SPP over BLE](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_secure_spp_over_ble) custom service:

- [Service] **SPP Service**: UUID `4880c12c-fdcb-4077-8920-a450d7f9b907`
  - [Char] **SPP Data**: UUID `fec26ec4-6d71-4442-9f81-55bc21d658d6`
    - [**Notifiable**] - Get notification of log data

### Data logger Implementation

#### Application initialization

![Application init](images/application_init.png)  

#### Device connect event

![Device connect event](images/device_connect_event.png)  

#### Priodic timer event

![Priodic timer event](images/priodic_timer_event.png)

#### Application Workflows

1. Initialize the peripherals, the Bluetooth stack

2. Get the logger enable configuration from nvm

3. Initialize the OLED display

4. Mount file system on the SD card

    - If failure to mount the file system then re-format it (make new file system on the SD card)

5. Initialize the humidity & temperature sensor si7021

6. Start a periodic timer with period 10s. The timer callback will fire an external event to BLE stack and the event handler will do:

    - Get the sensor humidity & temperature sample.
    - Update OLED display with the sensor data
    - Make a log entry from sensor data.
    - Check state of the BLE connection:
      - If the receiver is connected and the notification is enabled then send the log entry to the receiver over BLE notification
      - If no BLE connection is made or the notification is not enabled:
        - If the logger is enabled then append the log entry to the sdcard

7. When a new BLE connection is made and notification of the **SPP data** characteristic is enabled then:
    - If the log file is exist on the sdcard then all the log on that file will be send line by line over BLE notification of the **SPP data** characteristic. After that the log file will be deleted
    - If the log file is not exist or after sending all the log data from file, the new log data is continuing sent over BLE notification of the **SPP data** characteristic

8. When press button 0 the callback will fire an external event to BLE stack and the event handler will do:
  
    - If the logger has been disabled then enable data logger and save the config to the NVM.
    - If the logger has been enabled then disable data logger and save the config to the NVM.

### OLED Display

- Display current sensor data
  
  ![OLED display](images/oled_display.png)

### Button

- Press button to enable/disable the logger

### LED

- Indicate the logger is enabled or disabled

### Log data format

- The log data is sent line by line. They are seperated by a line feed character.
- The log line format:

> yyyy/mm/dd hh:mm:ss Humidity = \<humidity_value_percentage\> RH, Temperature = \<temperature_value_degree\> C

### SPP Data notification

- To view the log data, we have 2 option:

  - Use **EFR Connect Mobile Application** to enable notification, the log data is received as ascii string

  - Use [BLE SPP app for windows](https://github.com/SiliconLabs/bluetooth_applications/tree/master/bluetooth_spp_with_windows) on a laptop or PC that has a bluetooth adapter. First we need to enter the name of the data logger (default name is **"Data Logger"**). You should expect a similar output to the one below.

    ![Log View](images/log_view.gif)

## .sls Projects Used

- [bluetooth_data_logger_sd_card.sls](SimplicityStudio/bluetooth_data_logger_sd_card.sls)
