# People Counting Application with BLE #

## Overview ##

This project aims to implement a people counting application using [SparkFun Human Presence Sensor Breakout - AK9753 (Qwiic)](https://www.sparkfun.com/products/14349) and Silabs development kit [BGM220P Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit) with integrated BLE stack.

The system overview of this application is shown in the image below:

![hardware connection](images/system_overview.png)

## Gecko SDK version ##

- GSDK v4.1.1

## Hardware Required ##

- [BGM220P Bluetooth Module Explorer Kit](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- [SparkFun Human Presence Sensor Breakout - AK9753 (Qwiic)](https://www.sparkfun.com/products/14349)
- [SparkFun Micro OLED Breakout (Qwiic)](https://www.sparkfun.com/products/14532)

## Connections Required ##

The hardware connection is shown in the image below:

![hardware connection](images/connection.png)

## Setup ##

To test this application, you can either import the provided bluetooth_people_counting.sls project file or start with an empty example project as the following:

1. Create a Bluetooth - SoC Empty project for the BGM220 Bluetooth Module Explorer Kit using Simplicity Studio 5.

2. Copy all attached files in inc and src folders into the project root folder (overwriting existing app.c).

3. Import the GATT configuration:

    - Open the .slcp file in the project.

    - Select the CONFIGURATION TOOLS tab and open the Bluetooth GATT Configurator.

    - Find the Import button and import the attached gatt_configuration.btconf file.

    - Save the GATT configuration (ctrl-s).
4. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:
    - Install [Platform] > [Driver] > [I2CSPM] component with the default instance name: qwiic. Set this component to use I2C1 peripheral, SCL to PD02 pin, SDA to PD03 pin.
    - Install [Platform] > [IO Stream] > [IO Stream: USART] component with the default instance name: vcom.
    - Install [Platform] > [Driver] > [Button] > [Simple Button] component with the default instance name: btn0.
    - Install [Bluetooth] > [NVM] > [NVM Support] component.
    - Install [Application] > [Utility] > [Log] component.
5. Build and flash the project to your device.

    *Note*: You need to create the bootloader project and flash it to the device before flashing the application. When flash the application image to the device, use the .hex or .s37 output file. Flashing the .bin files may overwrite (erase) the bootloader.

## How it Works

### Application overview

![Application overview](images/application_overview.png)

### GATT Configuration

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

A new custom service (People Counting) with 8 characteristic must be added.

- **People Entered So Far**:
  - [**Readable**] - Get total number of people enter the room

  - [**Writable**] - Clear total number of people enter the room

- **People Count**:
  - [**Readable**] - Get number of people in the room

  - [**Writable**] - Clear number of people in the room

- **Room Capacity**:
  - [**Readable**] - Get the capacity of the room

  - [**Writable**] - Set the capacity of the

  - [**Notifiable**] - Get notification of room status( full or empty)

- **Lower Threshold**:
  - [**Readable**] - Get the lower threshold

  - [**Writable**] - Set the lower threshold

- **Upper Threshold**:
  - [**Readable**] - Get the upper threshold

  - [**Writable**] - Set the upper

- **IR Threshold**:
  - [**Readable**] - Get the IR threshold

  - [**Writable**] - Set the IR threshold

- **Hysteresis**:
  - [**Readable**] - Get hysteresis

  - [**Writable**] - Set hysteresis

- **Notification status**:
  - [**Readable**] - Get the notification status

  - [**Writable**] - Set the notification status

### People Counting Implementation

#### Application initialization

![Application init](images/app_init.png)

#### Sensor initialization

#### Application Workflows

1. Initialize the peripherals, the Bluetooth stack

2. Initialize and load the NVM3 configurations

3. Initialize AK9753 sensor with the configuration loaded from NVM3:
    - Upper threshold: 100
    - Lower threshold: -200
    - IR Threshold: 800
    - Hysteresis: 50

4. Initialize OLED display

5. Start a periodic timer with period 1000ms, The timer callback will fire an external event to ble stack and the event handler will display people counting data from the result of the counting algorithm calculation.

6. After the *sl_bt_evt_system_boot_id* event arrives, App sets up the security manager to bond with an iOS/Android device. And then start advertising.

7. Handle GATT event to help user configure the [counting algorithm](#counting-algorithm) and get the result from the algorithm calculation over the *EFR32 connect* mobile

### Counting algorithm

The AK9753 sensor includes 4 IR sensors that arranged in the picture below, and the detection algorithm will be based on the values of the IR2 and IR4.

![sensor](images/sensor.png)

We divide the ambient space into 3 areas: front, back and middle zone. Whenever a person appears in one of these 3 zones, a trigger will happend to notify that there is a person in this areas and the status of this area will be aslo stored into a list. The order of these state values in the list are used to detect the moving direction of the person. For example, if the consecutive states in the list are 0, 1, 3, 2 or 0, 2, 3, 1 this means a person has been detected in one direction or the other.

![entering pattern](images/entering_pattern.png)

![leaving pattern](images/leaving_pattern.png)

When no-one is seen in either of the two zones, the list of states will be reset. The workflow of the whole algorithm is describe in the picture below.

![algorithm_workflow](images/algorithm_workflow.png)
### OLED Display

- Display the current people count and people entered so far values.

### Button

- Press the button to reset the people count to 0.

### Room status notification

- To receive status of room (full or empty), the user should use the [EFR Connect Mobile Application](#use-efr-connect-mobile-application) to enable notification
- If the number of people count is greater than room capacity then the device will send a "room is full" notification
- If the number of people count is zero then the device will send a "room is empty" .

### Reset the counting value

- To reset the number of total people entered the room, the user should use the [EFR Connect Mobile Application](#use-efr-connect-mobile-application) to write 0 to the *People Entered So Far* characteristic
- To reset the number of people count, the user should use the [EFR Connect Mobile Application](#use-efr-connect-mobile-application) to write 0 to the *People Count* characteristic.

### Use EFR Connect Mobile Application

#### Connect to the device

The Silicon Labs EFR Connect application utilizes the Bluetooth adapter on your phone/tablet to scan, connect and interact with BLE devices. To run this example, an iOS or Android smartphone with the EFR Connect app installed is required.

Open the EFR Connect application on your smartphone and allow the permission request when opened for the first time. Click [Develop] -> [Browser] and you will see a list of nearby devices which are sending Bluetooth advertisements. Find the one named *People Counting* and click the connect button on the right side. If app show the pairing request dialog, press **Pair** button to confirm authentication for the pairing process. After that, wait for the connection to be established and the GATT database to be loaded.

**Note**: The pairing process on Android and iOS devices is different. For more information, refer to bluetooth security.

| ![EFR32 Connect App](images/efr32_connect_app1.png) | ![EFR32 Connect App](images/efr32_connect_app2.png)|![EFR32 Connect App](images/efr32_connect_app3.png)|
| - | - | -|

#### Read/Write characteristics

The parameters of this example application can be easly configured via BLE characteristics. Values for the characteristics are handled by the application as ASCII strings. Tap on the main service to see the available characteristics. Please refer [GATT Configurator](#gatt-configurator) to choose correct characteristic.

**Read**

Push read button to request the value of a characteristic. (See ASCII fields.)

**Write**

For setting a parameter select a characteristic and tap on its write button. Type a new value in the ASCII field and push the **Send** button.

|People entered so far count|People count|Upper threshold|Lower threshold|
|-|-|-|-|
|- **Read** to get the current total people entered count <br> - **Write** *00 00* to reset total people entered count|- **Read** to get the current people count <br>- **Write** *00 00* to reset people count|- **Read** to get the current upper threshold setting <br>- **Write** to set upper threshold settings|- **Read** to get the current lower threshold setting <br>- **Write** to set lower threshold settings|

|IR threshold|Hysteresis|Notification status|Room capacity|
|-|-|-|-|
|- **Read** to get the current IR threshold setting <br>- **Write** to set IR threshold setting|- **Read** to get the current hysteresis setting<br>- **Write** to set hyteresis setting|- **Read** to get the current notification status setting<br>- **Write** to set notification status setting|- **Read** to get the current room capacity setting.<br>- **Write** to set room capacity setting|

## .sls Projects Used

- [bluetooth_people_counting.sls](SimplicityStudio/bluetooth_people_counting.sls)