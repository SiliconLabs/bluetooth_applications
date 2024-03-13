# Bluetooth - Air Quality Monitor #

![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_air_quality_monitor_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_air_quality_monitor_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_air_quality_monitor_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_air_quality_monitor_common.json&label=SDK&query=sdk&color=green)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_air_quality_monitor_build_status.json)
![Flash badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_air_quality_monitor_common.json&label=Flash&query=flash&color=blue)
![RAM badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_air_quality_monitor_common.json&label=RAM&query=ram&color=blue)

## Overview ##

This project aims to implement an air quality monitor and notification system using Silicon Laboratories development kits and external sensors integrated with the BLE wireless stack.

The block diagram of this application is shown in the image below:

![system overview](images/system_overview.png)

More detailed information can be found in the section [How it works](#how-it-works).

This code example referred to the following code examples. More detailed information can be found here:

- [OLED SSD1306 driver](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/silabs/micro_oled_ssd1306)
- [Buzzer driver](https://github.com/SiliconLabs/third_party_hw_drivers_extension/tree/master/driver/public/mikroe/buzz2_cmt_8540s_smt)
- [Bluetooth security feature](https://github.com/SiliconLabs/bluetooth_stack_features/tree/master/security)

## Gecko SDK version ##

- GSDK v4.4.0
- [Third-Party Hardware Drivers v2.0.0.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Hardware Required ##

- [Thunderboard Sense 2 Sensor-to-Cloud Advanced IoT Kit](https://www.silabs.com/development-tools/thunderboard/thunderboard-sense-two-kit)

- [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532)

- [BUZZ 2 click](https://www.mikroe.com/buzz-2-click)

- [Silabs Click Shield](https://www.mikroe.com/silabs-click-shield)

## Connections Required ##

The hardware connection is shown in the image below:

![hardware connection](images/hardware_connection.png)

The Thunderboard Sense 2 and the MikroE Buzzer 2 Click can be plugged into the Silabs Click Shield directly via the Thunderboard socket and the mikroBus socket respectively. To connect the Thunderboard with the SparkFun Micro OLED Breakout (Qwiic) board we can use some [Flexible Qwiic Cable - Female Jumper](https://www.sparkfun.com/products/17261) as shown in the table below:

| EFR32BG22 Thunderboard Kit markings |  Qwiic cables color scheme  |
|-------------------------------------|:---------------------------:|
| GND - EXP1                          |  Red - 3.3V                 |
| 3V3 - EXP2                          |  Black - GND                |
| PC6 - EXP7                          |  Yellow - SCL               |
| PC7 - EXP9                          |  Blue - SDA                 |

 Silabs Click Shield schematic is shown below.

![hardware connection](images/thunderboard_qwiic_shield.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware. You should connect the Thunderboard Sense 2 Sensor-to-Cloud Advanced IoT Kit to the PC using a MicroUSB cable.

### Create a project based on an example project ###

1. From the Launcher Home, add the BRD4166A  to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with the filter 'air quality'.

   ![create_demo](images/creat_demo.png "Create a project based on an example project")

2. Click **Create** button on the **Bluetooth - Air Quality Monitor** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy the .c files 'src/app.c' to the following directory of the project root folder (overwriting the existing files).

3. Install the software components:

    - Open the .slcp file in the project.

    - Select the SOFTWARE COMPONENTS tab.

    - Install the following components:

        - [Services] →  [Timers] →  [Sleep Timer]
        - [Services] →  [NVM3]
        - [Services] →  [IO Stream] → [IO Stream: USART] → vcom
        - [Application] →  [Utility] → [Log]
        - [Platform] →  [Driver] → [I2C] →  [I2CSPM] → qwiic
        - [Platform] →  [Driver] → [I2C] →  [I2CSPM] → sensor_gas
        - [Platform] →  [Driver] → [PWM] →  [PWM] → mikroe
        - [Platform] →  [Driver] → [Button] →  [Simple Button] → btn0
        - [Platform] →  [Board Driver] → [CCS811 - Gas/Air Quality Sensor]
        - [Platform] →  [Board] → [Board Control] →  Enable Air Quality sensor
        - [Third Party Hardware Drivers] → [Audio & Voice] → CMT_8540S_SMT - Buzz 2 Click (Mikroe)
        - [Third Party Hardware Drivers] → [Display & LED] → SSD1306 - Micro OLED Breakout (Sparkfun) - I2C
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

![app overview](images/overview.png)

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

![Flow diagram](images/work_flow.png)

1. First, the software initializes the peripherals, the Bluetooth stack, and logging to the virtual COM port.

2. After the *sl_bt_evt_system_boot_id* event arrives, it sets up the security manager to bond with an iOS/Android device. Then, it starts advertising.

3. Every time the timer expires, an Air quality monitor event handler retrieves and processes the measured air quality data as described below:

    ![Flow diagram](images/timer_event.png)

4. When the BTN0 button is pressed, the software checks the notification feature status, and buzzer state by the flowchart below:

    ![Flow diagram](images/btn0.png)

### OLED Display ###

Measured values are displayed on the OLED display:

- CO2 (ppm)

- tVOC (ppb)

- The health impact of the measured air quality levels is described in the following document: [calculating an actionable indoor air quality index](https://www.breeze-technologies.de/blog/calculating-an-actionable-indoor-air-quality-index/).

More detailed information can be found in the section [Testing](#testing).

### Testing ###

Upon reset, the application will display the Silicon Labs logo on the OLED screen for a few seconds. Then you can check the measured CO2 and tVOC values on the OLED screen. You should expect a similar output to the one below.

![OLED display](images/display.png)

**Note:** The measured CO2 and tVOC values will be more accurate after the sensor is warmed up.

Follow the below steps to test the example with the EFR Connect app:

1. Open the EFR Connect app on your iOS/Android device.

2. Find your device in the Bluetooth Browser, advertising as Air Quality, and tap Connect. Then you need to accept the pairing request when connected for the first time.

    ![pair request](images/pairing_request.png)

3. Find the unknown service at the above OTA service.

4. Try to read, write, re-read the characteristics, and check the value.

5. You can launch the Console that is integrated into Simplicity Studio or can use a third-party terminal tool like TeraTerm to receive the data from the virtual COM port. Use the following UART settings: baud rate 115200, 8N1, no flow control. You should expect a similar output to the one below.

    ![logs](images/logs.png)
