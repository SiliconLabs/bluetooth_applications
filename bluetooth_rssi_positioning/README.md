# Bluetooth - RSSI Positioning #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Sparkfun-Micro%20OLED%20Breakout%20(Qwiic)%20board-green)](https://www.sparkfun.com/products/14532)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-198.73%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-10.66%20KB-blue)
## Description ##

Bluetooth RSSI Positioning is an application intended to showcase a room finder or asset locator service using the BLE stack on Silicon Labs development kits.

The setup of this example is shown in the image below:

![room finder](image/room_finder.png)

More detailed information can be found in the section [How it works](#how-it-works).

This code example referred to the following code examples. More detailed information can be found here:

- [OLED SSD1306 driver](https://github.com/SiliconLabs/platform_hardware_drivers/tree/master/oled_ssd1306_i2c)

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required ##

- 2x [Bluetooth Low Energy Development Kit](https://www.silabs.com/development-tools/wireless/bluetooth). One is for the Asset, another is for the Gateway. For simplicity, Silicon Labs recommends the [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- 1x [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532) is used for the Asset
- 1x smartphone running the 'Simplicity Connect' mobile app

**NOTE:**

- At least one gateway shall be active, however any number of gateways/rooms could be used.

## Connections Required ##

The hardware connection for the asset is shown in the image below:

![hardware connection](image/asset_hw.png)

The BGM220 explorer kit and the SparkFun Micro OLED Breakout (Qwiic) board can be connected with a [Flexible Qwiic Cable - Female Jumper](https://www.sparkfun.com/products/17261).

Gateways do not require a display.

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

**NOTE**:

- Make sure that the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

### Create a project based on an example project ###

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by "positioning".

2. Click **Create** button on the **Bluetooth - RSSI Positioning Asset** or **Bluetooth - RSSI Positioning Gateway** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.
![create_project](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your device using Simplicity Studio 5.

2. Copy all attached files in the *inc* and *src* folders into the project root folder (overwriting existing app.c).

3. Copy the *oled* folder and its contents to the asset's project root folder - this contains the OLED driver and the GLIB library.

4. Import the GATT configuration:

   - Open the .slcp file in the project.

   - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

   - Find the Import button and import the attached config/gatt_configuration.btconf file.

   - Save the GATT configuration (ctrl-s).

5. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    **Asset:**
    - [Platform] → [Driver] → [I2CSPM] → default instance name: **qwiic**
    - [Platform] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom**
    - [Platform] → [Driver] → [Button] → [Simple Button] → default instance name: **btn0**
    - [Application] → [Utility] → [Log]
    - [Third Party Hardware Drivers] → [Display & LED] → [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C]
    - [Third Party Hardware Drivers] → [Services] → [GLIB - OLED Graphics Library]

    **Gateway:**
    - [Platform] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom**
    - [Platform] → [Driver] → [Button] → [Simple Button] → default instance name: **btn0**
    - [Application] → [Utility] → [Log]

6. Build and flash the projects to your devices.

**NOTE:**

- A bootloader needs to be flashed to your board if the project starts from the "Bluetooth - SoC Empty" project, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

### BLE RSSI Indoor Positioning Implementation ###

The Initialization software flow is as follows:

![Flow_diagram_init](image/initialization.png)

## How it Works ##

1. At first, the software initializes the peripherals, the Bluetooth stack, and logs to the virtual COM port.
2. Application checks for the status of Button0 - either pressed or released.
3. Based on the button status, application either enters Configuration mode or Normal mode, both detailed below:

### Normal mode ###

Normal Mode is different for the two applications:

**Gateway**:

- Creates advertisement package from the configured Room ID, Room Name and Device Name
- Starts advertisements infinitely

**Asset**:

- Starts a periodic timer to trigger Room finding in given intervals.
- If the timer elapses, it triggers the room finding service which sets up and starts scanner to find gateways.
- Application is looking for gateways for a defined time limit (GATEWAY_FINDER_TIMEOUT_MS). Default value is 5 seconds.
- Once Gateway finding finished application checks if at least one gateway is found or not.
  - In case no gateways were found it displays "Waiting For Gateways" message on the OLED displays, resets the state of the room finder application and waits for the next trigger.
  - In case at least one gateway was found, it starts collecting RSSI samples from all found gateways.
- Once enough samples were gathered, calculation starts and the application finds the closest gateway based on its RSSI.
- The closest gateways data(Room name, Room Id) is used to create an advertisement package and starts advertising this data for 5 seconds.
- At the same time, the closest room's name is displayed on the OLED.
- Application resets the state and waits for the next trigger.

### Configuration mode ###

![Configuration](image/configmode_flow.png)

Configuration mode starts advertising, enables connections to the device and makes configuration updates possible.

Both the Asset and the Gateway should be configured before using for the first time.

Follow the below steps to test the example:

1. Hold down Button 0 while pressing the Reset button to enter configuration mode.

2. Open the Simplicity Connect app on your smartphone and allow the permission requested the first time it is opened.

3. Find your device in the Bluetooth Browser, advertising as either *IPAS_####* or *IPGW_####*, and tap Connect. Then you need accept the pairing request when connected for the first time.

   | ![browser_asset](image/browser_asset.png) | ![browser_gw](image/browser_gw.png) |
   | --- | --- |

4. Find the Indoor Positioning service above the OTA service.

   ![indoorpos_service](image/indoor_pos_service.png)

5. Read the descriptions to identify configuration options.

   ![configuration options_asset](image/config_options_asset.png)         ![configuration options_gw](image/config_options_gw.png)

6. For both the **Asset** and the **Gateways** the Network Unique Identifiers shall match, so enter the same custom value on all assets and gateways (unsigned 4bytes (0 ... 4294967295))

7. For the **Asset** the _Reporting Interval_ configuration value is the amount of time in seconds the asset recalculates its own position and finds the closest room

8. You can launch the Console that is integrated on Simplicity Studio or can use a third-party terminal tool like TeraTerm to receive the data from the virtual COM port. Use the following UART settings: baud rate 115200, 8N1, no flow control. You should expect a similar output to the one below.

   |![logs_asset](image/logs_asset.PNG)  | ![logs_config_asset](image/logs_config_asset.PNG)|
   |-|-|

### GATT Configurator ###

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT changes were adding a new service (Indoor Positioning) using UUID `1821` with 2 custom characteristics:

**Asset:**

- **Network Unique Identifier**: UUID `7719544e-308b-4853-996a-e50ba5297e03`

  - [**Readable**] - Get current unique network identifier

  - [**Writable with response**] - Set unique network identifier

- **Reporting Interval**: UUID `b4e04b7f-94ff-4aa5-b149-9b78d36d7754`

  - [**Readable**] - Get current reporting interval (in seconds)

  - [**Writable with response**] - Set reporting interval (in seconds)

**Gateway:**

- **Network Unique Identifier**: UUID `7719544e-308b-4853-996a-e50ba5297e03`
  - [**Readable**] - Get current unique network identifier
  - [**Writable with response**] - Set unique network identifier
- **Room ID**: UUID `41841d0f-5fee-400b-be5c-36323a41308d`
  - [**Readable**] - Get currently set Room Identifier
  - [**Writable with response**] - Set Room Identifier
- **Room Name**: UUID `c476ba11-db09-4bf5-9b82-6798560eb7aa`
  - [**Readable**] - Get currently set Room name
  - [**Writable with response**] - Set Room Name

### OLED Display ###

After the BLE stack's boot event is done, the following message will be displayed:

![Waiting For Gateways](image/waiting_for_gws_oled.png)

In case at least one gateway is in range "room finding" will start and will display the name of the closest room:

![Room found](image/room_found_oled.png)
