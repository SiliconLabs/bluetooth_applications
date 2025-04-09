# Bluetooth - Distance Monitor (VL53L1X) #
![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Mikroe-BUZZ%202%20Click%20board-green)](https://www.mikroe.com/buzz-2-click)
[![Required board](https://img.shields.io/badge/Sparkfun-Micro%20OLED%20Breakout%20(Qwiic)%20board-green)](https://www.sparkfun.com/products/14532)
[![Required board](https://img.shields.io/badge/Sparkfun-Distance%20Sensor%20Breakout-green)](https://www.sparkfun.com/products/14722)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-216.76%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-11.33%20KB-blue)

## Overview ##

This project demonstrates a Bluetooth distance monitor application using SparkFun Distance Sensor Breakout - VL53L1X and BGM220 Explorer Kit Board BRD4314A with the BLE wireless stack.

The block diagram of this application is shown in the image below:

![overview](image/overview.png)

In normal operation, the sensor device uses the BGM220 Explorer Kit board to get the distance value from the VL53L1X sensor periodically and show it on the SparkFun Micro Oled. To monitor the distance, the user can turn on the notification status by pressing the button 0 on the BGM220 board. After that, the Buzzer 2 Click will be active every time the distance value matches the threshold value.

This device can be connected to Simplicity connect app that allows users configuration the threshold mode, range mode, threshold value, buzzer volume, and notification status.

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required ##

- 1x [Bluetooth Low Energy Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For example, [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- 1x [SparkFun Distance Sensor Breakout - 4 Meter, VL53L1X (Qwiic)](https://www.sparkfun.com/products/14722)
- 1x [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532)
- 1x [Mikroe BUZZ 2 Click board](https://www.mikroe.com/buzz-2-click)
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required ##

Sensor boards can be easily connected via qwiic and microbus connectors to the BGM220P explorer kit.

![connection](image/connection.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

**NOTE**:

- Make sure that the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is installed as part of the SiSDK and the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- SDK Extension must be enabled for the project to install the required components.

### Create a project based on an example project ###

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by "distance monitor".

2. Click **Create** button on **Bluetooth - Distance Monitor (VL53L1X)** example. From the project creation dialog pops up, click Create and Finish and source code should be generated.

   ![create example project](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all the [source](src/) and [header](inc/) files to the following directory of the project root folder (overwriting existing files).

3. Install the software components:

   - Open the .slcp file in the project.

   - Select the SOFTWARE COMPONENTS tab.

   - Install the following components:
      - [Services] → [Timers] → [Sleep Timer]
      - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom**
      - [Bluetooth] → [Bluetooth Host (Stack)] → [Additional Features] → [NVM Support]
      - [Application] → [Utility] → [Log]
      - [Platform] → [Driver]→ [I2C] → [I2CSPM] → default instance name: **qwiic**
      - [Platform] → [Driver]→ [Button] → [Simple Button] -> default instance name: **btn0**
      - [Third Party Hardware Drivers] → [Display & LED] → [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C]
      - [Third Party Hardware Drivers] → [Audio & Voice] → [CMT_8540S_SMT - Buzz 2 Click (Mikroe)]
      - [Third Party Hardware Drivers] → [Sensors] → [VL53L1X - Distance Sensor Breakout (Sparkfun)]
      - [Third Party Hardware Drivers] → [Services] → [GLIB - OLED Graphics Library]

4. Import the GATT configuration:

    - Open the .slcp file in the project again.
    - Select the CONFIGURATION TOOLS tab and open the "Bluetooth GATT Configurator".
    - Find the Import button and import the [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.
    - Save the GATT configuration (ctrl-s).

5. Build and flash the project to the board.

**Note:**

- A bootloader needs to be flashed to your board if the project starts from the "Bluetooth - SoC Empty" project, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

### Application Overview ###

![software layers](image/software_layers.png)

### Initialization ###

Application logic initialization function is invoked from the _app_init()_ function at startup.

![appplication init](image/init.png)

### BLE User Request Events ###

Where R = Readable, W = Writeable with response.

**BLE GATT Database:**

- [Service] Distance Monitor: `84b256e1-8292-4a07-b3d8-77d1f4bdb80e`
  - [Char] Lower Threshold Value (50-4000): `bf393a58-b4c7-11ec-b909-0242ac120002`
    - [R] Get the lower threshold value
    - [W] Set the lower threshold value
  - [Char] Upper Threshold Value (50-4000): `bf393cd8-b4c7-11ec-b909-0242ac120002`
    - [R]  Get the upper threshold value
    - [W] Set the lower threshold value
  - [Char] Threshold Mode (0-3): `bf393e2c-b4c7-11ec-b909-0242ac120002`
    - [R]  Get the threshold mode
    - [W] Set the threshold mode
  - [Char] Buzzer volume (0-10): `bf393f6c-b4c7-11ec-b909-0242ac120002`
    - [R] Get configured buzzer volume
    - [W] Set buzzer volume
  - [Char] Range Mode (1,2): `bf3940ac-b4c7-11ec-b909-0242ac120002`
    - [R] Get configured range mode
    - [W] Set range mode
  - [Char] Notification Status (0,1): `bf3941d8-b4c7-11ec-b909-0242ac120002`
    - [R] Get configured notification status
    - [W] Set notification status

BLE Characteristic user read/write requests are processed as the following flowchart.

![ble requests flow](image/ble_requests.png)

### External Runtime Events ###

- Application Logic main timer expires => 100 ms
- Screen update timer expires => 200 ms
- User press button (BTN0)

![runtime_events](image/runtime_events.png)

### Application Main Function ###

The application logic main function is executed periodically when the main periodic timer expires and raises an external event to the BLE stack every 100 ms. Then the BLE external event handler invokes this function in which the measured distance is processed and the calculated average value is checked against the configured thresholds by the selected threshold mode and notification status.

![main_flow](image/main_flow.png)

### Screen Update ###

The screen update function is invoked periodically. It updates the information displayed on the screen and handles buzzer toggling when the notification is active.

![screen_update](image/screen_update.png)

### Button Pressed ###

![button_event](image/button_event.png)

### Application main source files ###

- [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf): BLE GATT Database

- [app_config.h](inc/app_config.h): Application configuration parameters (e.g.: BLE Passkey) and BLE GATT Characteristic <-> Application feature binding.

- [app_logic.c](src/app_logic.c): Implements the applications's main logical blocks.

- [app_callbacks.c](src/app_callbacks.c): Implements callback functions for platform drivers. (Timers, Buttons)

- [app_ble_events.c](src/app_ble_events.c): Configures BLE stack and handles BLE events.

- [app_events.c](src/app_events.c): Application specific event handlers. (BLE User requests, timer events, and button events come from the BLE stack.)

## Testing ##

Upon reset, the application will display the Silicon Labs logo on the OLED screen for a few seconds. \
After the distance sensor is booted up and its configuration is done, the application starts the periodic distance measurement and gathers the configured samples. \
While the samples are gathered the display shows the **SENSOR INIT.** text. Once the application is gathered enough measurement data to calculate the average distance then the application will update the screen with the latest available average distance.

![application console log](image/app_console_log.png)
![display design](image/iddle.png)
![display show distance in range](image/range.png)

![application testing](image/testing.png)

### Configuration with the Simplicity Connect Mobile Application ###

**Connect to the device:**

Follow the below steps to test the example with the Simplicity Connect application:

- Open the Simplicity Connect app on your smartphone and allow the permission requested the first time it is opened.

- Find your device in the Bluetooth Browser, advertising as *Distance*, and tap Connect. Then you need accept the pairing request when connected for the first time. For iOS devices, enter the passkey (passkey default as **123456** (app_config.h: DISTANCE_MONITOR_PASSKEY)) to confirm authentication for the pairing process for the first time. For Android devices, the user must accept a pairing request first and do as above. After that, wait for the connection to be established and the GATT database to be loaded.

_Note_: The pairing process on Android and iOS devices is different. For more information, refer to [bluetooth security](https://github.com/SiliconLabs/bluetooth_stack_features/tree/master/security).

**Read/Write characteristics:**

The parameters of this example application can be easily configured via BLE characteristics.
Values for the characteristics are handled by the application as ASCII strings.
Tap on the main service to see the available characteristics.

***Read***

Push the read button to request the value of a characteristic. (See ASCII fields.)

***Write***

For setting a parameter select a characteristic and tap on its write button. Type a new value in the ASCII field and push the Send button.

![Simplicity connect application](image/efr_app.png)

By default, the application is configured to notify the user if the measured distance is below the configured lower threshold (default: 250 mm). The notification is muted by default, it can be enabled via either pushing the BTN0 button on the development kit or through a write request to the corresponding BLE characteristic.
Buzzer volume, threshold modes (below the lower threshold, above the upper threshold, inside, outside of the threshold limits), and threshold limits [50-4000 mm] can be configured too. If the measured average value is outside of the sensor's valid range [40-1300/4000 mm] the notification system is inactive and the "OUT OF RANGE" label is displayed on the screen.

This application only aims to demonstrate the basic capabilities of this distance sensor with the Silicon Labs BLE wireless stack. This means that the notification status in the application logic is not debounce filtered and the configured threshold parameters are not checked against the sensor's range mode configuration (upper distance limit: short range mode = 1300 mm, long range mode = 4000 mm).
