# Bluetooth - Air Quality Monitor #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Mikroe-BUZZ%202%20click-green)](https://www.mikroe.com/buzz-2-click)
[![Required board](https://img.shields.io/badge/Sparkfun-Micro%20OLED%20Breakout%20(Qwiic)%20board-green)](https://www.sparkfun.com/products/14532)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-221.62%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-11.2%20KB-blue)

## Overview ##

This project aims to implement an air quality monitor and notification system using Silicon Labs development kits and external sensors integrated with the BLE wireless stack. The block diagram of this application is shown in the image below:

![system overview](image/system_overview.png)

More detailed information can be found in the section [How it works](#how-it-works).

This code example referred to the following code examples. More detailed information can be found here:

- [OLED SSD1306 driver](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/silabs/micro_oled_ssd1306)
- [Buzzer driver](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/mikroe/buzz2_cmt_8540s_smt)
- [CCS811 - Air Quality Sensor (Sparkfun)](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/silabs/environmental_bme280_ccs811)
- [Bluetooth security feature](https://github.com/SiliconLabs/bluetooth_stack_features/tree/master/security)

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required ##

- 1x [Bluetooth Low Energy Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For example, [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- 1x [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532)
- 1x [BUZZ 2 click](https://www.mikroe.com/buzz-2-click)
- 1x SparkFun CCS811/BME280 Combo Board
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required ##

The hardware connection is shown in the image below:

![hardware connection](image/hardware_connection.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware. You should connect your board to the PC using a MicroUSB cable.

**NOTE**:

- Make sure that the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is installed as part of the SiSDK and the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- SDK Extension must be enabled for the project to install the required components.

### Create a project based on an example project ###

1. From the Launcher Home, add your board to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by 'air quality'.

   ![create_demo](image/creat_demo.png "Create a project based on an example project")

2. Click **Create** button on the **Bluetooth - Air Quality Monitor** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all the header files and source files in the 'inc' and 'src' folders to the root folder of the project (overwriting the existing files).

3. Install the software components:

    - Open the .slcp file in the project.

    - Select the SOFTWARE COMPONENTS tab.

    - Install the following components:

        - [Services] → [Timers] → [Sleep Timer]
        - [Services] → [IO Stream] → [IO Stream: USART] → vcom
        - [Application] → [Utility] → [Log]
        - [Platform] → [Driver] → [I2C] → [I2CSPM] → qwiic
        - [Platform] → [Driver] → [PWM] → [PWM] → mikroe
        - [Platform] → [Driver] → [Button] → [Simple Button] → btn0
        - [Third Party Hardware Drivers] → [Sensors] → CCS811 - [Air Quality Sensor (Sparkfun)]
        - [Third Party Hardware Drivers] → [Audio & Voice] → [CMT_8540S_SMT - Buzz 2 Click (Mikroe)]
        - [Third Party Hardware Drivers] → [Display & LED] → [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C]
        - [Third Party Hardware Drivers] → [Services] → [GLIB - OLED Graphics Library]
        - [Bluetooth] → [OTA] → [In-Place OTA DFU] → uninstall
        - [Platform] → [Bootloader Application Interface] → uninstall.

4. Import the GATT configuration:

    - Open the .slcp file in the project again.
    - Select the CONFIGURATION TOOLS tab and open the "Bluetooth GATT Configurator".
    - Find the Import button and import the [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.
    - Save the GATT configuration (ctrl-s).

5. Build and flash this project to the board.

## How it Works ##

### Application Overview ###

![app overview](image/overview.png)

### GATT Configurator ###

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT changes were adding a new custom service (Air Quality Monitor) using UUID `3a79c933-c922-45c7-b5e7-9bdefd126dd9` which are 5 characteristics:

- **Notification**: UUID `ec26adea-75af-409d-b267-51a4e753e9ea`

  - [**Readable**] - Get notification status

  - [**Writable with response**] - Set notification status

- **Buzzer volume**: UUID `c5fd8492-9c55-4c18-b761-99b8cf9bca77`

  - [**Readable**] - Get configured buzzer volume

  - [**Writable with response**] - Set buzzer volume (0-10)

- **CO2**: UUID `1b621ff2-b789-4b7c-985f-b62a50802bbf`

  - [**Readable**] - Get the latest measured CO2 level (ppm)

  - [**Writable with response**] - Set CO2 Threshold (ppm)

- **tVOC**: UUID `ec099dd9-7887-4ca6-a169-92a5e9ed7926`

  - [**Readable**] - Get the latest measured tVOC level (ppb)

  - [**Writable with response**] - Set tVOC Threshold (ppb)

- **Measurement update period**: UUID `98205b49-a9e1-4bfc-a18d-60d36798397f`

  - [**Readable**] - Get configured measurement update period in s (1-30)

  - [**Writable with response**] -  Set configured measurement update period in s (1-30)

### Air Quality Monitor Implementation ###

The Initialization software flow is as follows:

![Flow diagram](image/work_flow.png)

1. First, the software initializes the peripherals, the Bluetooth stack, and logging to the virtual COM port.

2. After the *sl_bt_evt_system_boot_id* event arrives, it sets up the security manager to bond with an iOS/Android device. Then, it starts advertising.

3. Every time the timer expires, an Air quality monitor event handler retrieves and processes the measured air quality data as described below:

    ![Flow diagram](image/timer_event.png)

4. When the BTN0 button is pressed, the software checks the notification feature status, and buzzer state by the flowchart below:

    ![Flow diagram](image/btn0.png)

### OLED Display ###

Measured values are displayed on the OLED display:

- CO2 (ppm)

- tVOC (ppb)

- The health impact of the measured air quality levels is described in the following document: [calculating an actionable indoor air quality index](https://www.breeze-technologies.de/blog/calculating-an-actionable-indoor-air-quality-index/).

More detailed information can be found in the section [Testing](#testing).

### Testing ###

Upon reset, the application will display the Silicon Labs logo on the OLED screen for a few seconds. Then you can check the measured CO2 and tVOC values on the OLED screen. You should expect a similar output to the one below.

![OLED display](image/display.png)

**Note:** The measured CO2 and tVOC values will be more accurate after the sensor is warmed up.

Follow the below steps to test the example with the Simplicity Connect app:

1. Open the Simplicity Connect app on your smartphone and allow the permission requested the first time it is opened.

2. Find your device in the Bluetooth Browser, advertising as *Air Quality*, and tap Connect. Then you need to accept the pairing request when connected for the first time.

    ![pair request](image/pairing_request.png)

3. Find the unknown service.

4. Try to read, write, re-read the characteristics, and check the value.

5. You can launch the Console that is integrated into Simplicity Studio or can use a third-party terminal tool like TeraTerm to receive the data from the virtual COM port. Use the following UART settings: baud rate 115200, 8N1, no flow control. You should expect a similar output to the one below.

    ![logs](image/logs.png)
