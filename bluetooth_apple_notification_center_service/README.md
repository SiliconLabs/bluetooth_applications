# Bluetooth - Apple Notification Center Service #
![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-197.25%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-10.49%20KB-blue)

## Description ##

This example demonstrates how to receive [Apple Notification Center Service (ANCS)](https://developer.apple.com/library/content/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Specification/Specification.html) Notifications, such as phone calls, calendar events, and so on and print them out to the VCOM.

The purpose of the Apple Notification Center Service is to give Bluetooth accessories (that connect to iOS devices through a Bluetooth Low Energy link) a simple and convenient way to access notifications that are generated on iOS devices.

### Terminology ###

- The Apple Notification Center Service is referred to as the ANCS.

- The publisher of the ANCS service (the iOS device) is referred to as **Notification Provider** (NP).

- Any client of the ANCS service (an accessory) is referred to as a **Notification Consumer** (NC).

- A notification displayed on an iOS device in the iOS Notification Center is referred to as **iOS notification**.

- A notification sent by a GATT characteristic as an asynchronous message is referred to as a **GATT notification**.

### The Apple Notifications Center Service ###

In its basic form, the ANCS exposes three characteristics:

- **Notification Source:** UUID: 9FBF120D-6301-42D9-8C58-25E699A21DBD (notifiable)

- **Control Point:** UUID: 69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9 (writable with response)

- **Data Source:** UUID: 22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB (notifiable)

All these characteristics require authorization for access.

Support for the Notification Source characteristic is mandatory, whereas support for the Control Point characteristic and Data Source characteristic is optional.

**Note:** In this demo project only the Notification Source characteristic is used.

A GATT notification delivered through the Notification Source characteristic contains the following information:

- **EventID:** This field informs the accessory whether the given iOS notification was added, modified, or removed. The enumerated values for this field are defined in [EventID Values](https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Appendix/Appendix.html#//apple_ref/doc/uid/TP40013460-CH3-SW3).

- **EventFlags:** A bitmask whose set bits inform an NC of specificities with the iOS notification. For example, if an iOS notification is considered “important”, the NC may display a more aggressive user interface (UI) to properly alert the user. The enumerated bits for this field are defined in [EventFlags](https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Appendix/Appendix.html#//apple_ref/doc/uid/TP40013460-CH3-SW4).

- **CategoryID:** A numerical value providing a category in which the iOS notification can be classified. The NP will make a best effort to provide an accurate category for each iOS notification. The enumerated values for this field are defined in [CategoryID Values](https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Appendix/Appendix.html#//apple_ref/doc/uid/TP40013460-CH3-SW2).

- **CategoryCount:** The current number of active iOS notifications in the given category. For example, if two unread emails are sitting in a user’s email inbox and a new email is pushed to the user’s iOS device, the value of CategoryCount is 3.

- **NotificationUID:** A 32-bit numerical value that is the unique identifier (UID) for the iOS notification. This value can be used as a handle in commands sent to the Control Point characteristic to interact with the iOS notification.

For more information about ANCS, see the [specification page](https://developer.apple.com/library/content/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Specification/Specification.html).

### ANCS Implementation ###

The software flow is as follows:

1. First, the software initializes the peripherals, the Bluetooth stack, and logging to the virtual COM port.

2. After the *sl_bt_evt_system_boot_id* event arrives, it sets up the security manager to bond with an iOS device. Then, it starts advertising.

3. After the *sl_bt_evt_gatt_mtu_exchanged_id* event is received, the device is connected, and the connection parameters are negotiated. Then, the device starts searching for the ANCS service in the remote GATT with the *sl_bt_gatt_discover_primary_services_by_uuid* API.

4. If it finds the ANCS service, it will start searching for the notification source characteristic in the remote GATT with the *sl_bt_gatt_discover_characteristics_by_uuid* API.

5. If the notification source characteristic is found in the remote GATT, the device tries to subscribe to characteristic notification with the *sl_bt_gatt_set_characteristic_notification* API. If the iOS device and the EFR module are not bonded, subscribing is not possible. After the bonding is completed, enable the characteristic notification again and subscribing should work.

6. Receiving a *sl_bt_evt_gatt_characteristic_value_id* event with the *att_opcode* == *gatt_handle_value_notification* means a GATT notification is received from the remote device. In this case, the notification type is determined based on the notification UID and printed out with the **ancCharValueReceivedCallback** function.

The activity diagram below shows the described flow.

![Flow diagram](image/flow_diagram.png)

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required ##

- 1x [Bluetooth Low Energy Development Kit](https://www.silabs.com/development-tools/wireless/bluetooth). For simplicity, Silicon Labs recommends the [BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required ##

Connect the Bluetooth Development Kits to the PC through a compatible-cable. For example, a micro USB cable for the BGM220 Bluetooth Module Explorer Kit.

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

**NOTE**:

- Make sure that the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

### Create a project based on an example project ###

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by "ancs".

2. Click **Create** button on the **Bluetooth - Apple Notification Center Service** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

   ![board](image/create_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy the attached src/app.c file into your project (overwriting existing app.c).

3. Change your device name to **ANCS** by openning gatt configuration and editing value of Device Name characteristic as below

   ![Change Device Name](image/change_device_name.png)

4. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

   - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: vcom

   - [Platform] → [Board] → [Board Control] → enable *Virtual COM UART*

   - [Application] → [Utility] → [Log]

5. Build and flash the project to your device.

**Note:**

- Do not forget to flash a bootloader to your board, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

Follow the below steps to test the example:

1. On your PC open a terminal program, such as the Console that is integrated into Simplicity Studio or a third-party tool terminal like TeraTerm to receive the logs from the virtual COM port.

2. Open the Simplicity Connect app on your smartphone and allow the permission requested the first time it is opened.

3. Find your device in the Bluetooth Browser, advertising as "ANCS", and tap Connect. Then you need accept the pairing request and allow application to display your iOS device notifications when connected for the first time.

   ![paring](image/pairing_request.png)

4. Now you should get ANCS notifications, as shown below when you get an email, a text message, an incoming call, and so on.

   ![ancs](image/ANCS.png)
