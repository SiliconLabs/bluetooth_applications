# Bluetooth - IR Generator Example #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Bluetooth-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2024.12.0-green)
[![Required board](https://img.shields.io/badge/Amazon-Infrared%20diode-green)](https://www.amazon.com/Digital-Receiver-Transmitter-Arduino-Compatible/dp/B01E20VQD8/ref=sr_1_14?dchild=1&keywords=IR+receiver&qid=1591754671&s=aht&sr=1-14)
[![Required board](https://img.shields.io/badge/Amazon-4x4%20Matrix%2016%20Keys%20Button%20Keypad.-green)](https://www.amazon.com/Tegg-Matrix-Button-Arduino-Raspberry/dp/B07QKCQGXS/ref=sr_1_4?dchild=1&keywords=Key+matrix&qid=1591754882&sr=8-4)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-197.56%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-10.93%20KB-blue)

## Summary ##

This project shows the implementation of an IR signal generator and 4x4 matrix key scan with BLE on our EFR32 devices.
The expectation is to ensure the IR signal generator works well in cases of heavy BLE traffic.

![framework](image/framework.png)

## SDK version ##

- [SiSDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk)
- [Third Party Hardware Drivers v4.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
- [Simplicity Connect Mobile App](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app)

## Hardware Required ##

- 1x [SLWRB4182A](https://www.silabs.com/development-tools/wireless/slwrb4182a-efr32xg22-wireless-gecko-radio-board) EFR32xG22 Wireless Gecko 2.4 GHz +6 dBm 5x5, QFN40 Radio Board
- 1x [SI-MB4002A](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard) Wireless Pro Kit Mainboard
- 1x [4x4 Matrix 16 Keys Button Keypad.](https://www.amazon.com/Tegg-Matrix-Button-Arduino-Raspberry/dp/B07QKCQGXS/ref=sr_1_4?dchild=1&keywords=Key+matrix&qid=1591754882&sr=8-4)
- 1x [Infrared diode](https://www.amazon.com/Digital-Receiver-Transmitter-Arduino-Compatible/dp/B01E20VQD8/ref=sr_1_14?dchild=1&keywords=IR+receiver&qid=1591754671&s=aht&sr=1-14)
- 1x smartphone running the 'Simplicity Connect' mobile app

## Connections Required ##

![connection](image/hardware_connection.png)

Connect [4x4 Matrix 16 Keys Button Keypad](https://www.amazon.com/Tegg-Matrix-Button-Arduino-Raspberry/dp/B07QKCQGXS/ref=sr_1_4?dchild=1&keywords=Key+matrix&qid=1591754882&sr=8-4) and [Infrared diode](https://www.amazon.com/Digital-Receiver-Transmitter-Arduino-Compatible/dp/B01E20VQD8/ref=sr_1_14?dchild=1&keywords=IR+receiver&qid=1591754671&s=aht&sr=1-14) to WSTK board through the expansion header.

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

**NOTE**:

- Make sure that the [Third Party Hardware Drivers extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension) is installed as part of the SiSDK and the [bluetooth_applications](https://github.com/SiliconLabs/bluetooth_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

- SDK Extension must be enabled for the project to install the required components.

### Create a project based on an example project ###

![create_example](image/create_example.png)

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project filtering by "ir generator".

2. Click **Create** button on both **Bluetooth - IR Generator** examples. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy all attached files in the *inc* and *src* folders into the project root folder (overwriting existing).

3. Import the GATT configuration:

    - Open the .slcp file in the project.

    - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

    - Find the Import button and import the attached gatt_configuration.btconf file.

4. Open the .slcp file. Select the **SOFTWARE COMPONENTS** tab and install the software components:
    - [Services] → [IO Stream] → [IO Stream: USART] → default instance name: **vcom**
    - [Application] → [Utility] → [Log]
    - [Platform] → [Driver] → [GPIOINT]
    - [Third Party Hardware Drivers] → [Miscellaneous] → [IR Generator (Silabs)]
    - [Platform] → [Driver] → [Button] → [Simple Button] → default instance name: **btn0** and **btn1**

5. Build and flash the project to your device.

**Note:**

- A bootloader needs to be flashed to your board if the project starts from the "Bluetooth - SoC Empty" project, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

### Keypad ###

When a key is active (pressed), the GPIO interrupt wakes up the system, the key then scans and a key timer (10ms) starts to work. When the key is available or released, the key callback is invoked to indicate that it has been detected or released. After being released, the system reverts back to idle state once again.

- Initialization.
  - key_init() function initializes the keypad with 2 callbacks, set GPIO direction, and interrupt.
  - key_wakeup_callback_t wakeup_cb, is called in GPIO IRQ. It starts the key timer.
  - key_callback_t cb, is called in the key detection. It reports which key is detected or released.
- Running the key detection
  - key_scan() function runs in a key time slice, checking which keys and how long the key is pressed, then reports the key status.

#### Keypad flowchart ###

![keypad flowchart](image/keypad.png)

### IR generate ###

Most of the time the system stays in an idle state. When an IR send is required, the system configures the stream according to the given data, then sends out all the stream bit. If no stop command is inputted, the system will repeat the stream. If no need to repeat the IR signal, the system will go back to the idle state.

- Initialization.
  - ir_generate_init() function initializes the keypad with callback.
  - code_t ir_code sets the IR protocol, currently supporting NEC and SONY types (the default is SONY) only.
  - ir_callback_t cb is called if one frame stream is sent.
- Running the IR generate
  - ir_generate_stream() function configures the desired data to send and start, repeats flag used in NEC IR protocol.
  - ir_generate_stop() function stops the IR generate.

#### IR flowchart ####

![ir flowchart](image/ir.png)

### Testing ###

In this example, IR stream start/stop is controlled by a key event. We can use Console window for tracking the run status and logic analyzer/oscilloscope to check the IR waveform.

![console](image/console.png)
