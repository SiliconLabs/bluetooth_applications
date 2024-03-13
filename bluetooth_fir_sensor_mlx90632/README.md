# Bluetooth - IrThermo 3 click (MLX90632) #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_fir_sensor_mlx90632_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_fir_sensor_mlx90632_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_fir_sensor_mlx90632_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_fir_sensor_mlx90632_common.json&label=SDK&query=sdk&color=green)
[![Required board](https://img.shields.io/badge/Mikroe-IRTHERMO%203%20CLICK-green)](https://www.mikroe.com/ir-thermo-3-click)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_fir_sensor_mlx90632_build_status.json)

## Overview ##

This example implements the Ambient temperature and the Object temperature services. It enables a device to connect and receive the Object temperature values every second via Bluetooth.

![overview](images/overview.png)

This example showcases the implementation of two services: Ambient temperature and Object temperature. The system consists of a client device and a sensor device that periodically read data from an IR thermometer sensor.

If the Indication feature is enabled, the client will be notified about any updated values. Specifically, the client will receive Object temperature values via Bluetooth every second. This feature enhances convenience and efficiency by automatically delivering relevant data at regular intervals.

## Gecko SDK version ##

- GSDK v4.4.0
- [Third Party Hardware Drivers v2.0.0.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Hardware Required ##

- [BGM220 Bluetooth Module Explorer Kit - BRD4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)
- [IrThermo 3 Click - MLX90632 FIR sensor](https://www.mikroe.com/ir-thermo-3-click)

**NOTE:**
Tested boards for working with this example:

| Board ID | Description  |
| -------- | ------ |
| BRD2703A | [EFR32xG24 Explorer Kit - XG24-EK2703A](https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit?tab=overview)    |
| BRD4108A | [BG22 Bluetooth SoC Explorer Kit - BG22-EK4108A](https://www.silabs.com/development-tools/wireless/bluetooth/bg22-explorer-kit?tab=overview) |
| BRD4314A | [BGM220 Bluetooth Module Explorer Kit - BGM220-EK4314A](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit?tab=overview)   |

## Connections Required ##

The IrThermo 3 Click board supports MikroBus; therefore, it can connect easily to the MikroBus header of the BGM220 Explorer Kit. Be sure that the 45-degree corner of the IrThermo board matches the 45-degree white line of the Silabs Explorer Kit.
The Power LED on IrThermo 3 Click board will light when the kit is powered.

![hardware connection](images/hardware_connect.png)

## Setup ##

To test this application, you can either create a project based on an example project or start with a "Bluetooth - SoC Empty" project based on your hardware.

### Create a project based on an example project ###

1. From the Launcher Home, add your product name to My Products, click on it, and click on the **EXAMPLE PROJECTS & DEMOS** tab. Find the example project with filter "mlx90632".

2. Click **Create** button on **Bluetooth - IR Thermometer 3 Click (MLX90632)** example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

    ![create an example project](images/create_example_project.png)

3. Build and flash this example to the board.

### Start with a "Bluetooth - SoC Empty" project ###

1. Create a **Bluetooth - SoC Empty** project for your hardware using Simplicity Studio 5.

2. Copy the [app.c](src/app.c) file to the following directory of the project root folder (overwriting the existing file).

3. Install the software components:

    - Open the .slcp file in the project.

    - Select the SOFTWARE COMPONENTS tab.

    - Install the following components:

        - [Services] →  [Timers] →  [Sleep Timer]
        - [Services] →  [IO Stream] → [IO Stream: USART] → vcom
        - [Application] →  [Utility] → Log
        - [Platform] →  [Driver] → [I2C] →  [I2CSPM] → mikroe
        - [Third Party Hardware Drivers] → [Sensors] → MLX90632 - IrThermo 3 Click (Mikroe)
        - [Bluetooth] → [OTA] → [In-Place OTA DFU] → uninstall
        - [Platform] → [Bootloader Application Interface] → uninstall.

4. Import the GATT configuration:

    - Open the .slcp file in the project again.
    - Select the CONFIGURATION TOOLS tab and open the "Bluetooth GATT Configurator".
    - Find the Import button and import the [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.
    - Save the GATT configuration (ctrl-s).

5. Build and flash this project to the board.

**Note:**

- Make sure that the [SDK extension](https://github.com/SiliconLabs/third_party_hw_drivers_extension/blob/master/README.md) already be installed and this repository is added to [Preferences > Simplicity Studio > SDKs](https://github.com/SiliconLabs/third_party_hw_drivers_extension/blob/master/README.md#how-to-add-to-simplicity-studio-ide).

- SDK Extension must be enabled for the project to install some components for the IR thermometer sensor.

- Do not forget to flash a bootloader to your board, see [Bootloader](https://github.com/SiliconLabs/bluetooth_applications/blob/master/README.md#bootloader) for more information.

## How It Works ##

The application is based on the Bluetooth - SoC Empty example. Since the example already has the Bluetooth GATT server, advertising, and connection mechanisms, only minor changes are required.

The GATT changes were adding a standard service **Health Thermometer** with UUID ```0x1809``` that has a Temperature characteristic with UUID ```0x2A1C``` with Read and Indicate properties. The Temperature characteristic shows the temperature value, which is measured by IR Thermometer 3 click.

After resetting, the application will periodically read the IR thermometer sensor. If the Indication is enabled, the client is indicated about the updated values.

## Testing ##

Follow the below steps to test the example:

1. Open the EFR Connect app on your smartphone.

2. Find your device in the Bluetooth Browser, scan with the name "IR Thermo Example" and tap Connect.

3. Find the Health Thermometer service, try to read the Temperature measurement characteristic and check the value.

4. Enable Indicate the unknown characteristic. Now, you can cover the IR thermometer sensor by hand or other temperature source and check the value.

![IR thermometer device on efr connect app](images/IR_thermoter_device.png)

You can launch the Console that is integrated in Simplicity Studio or can use a third-party terminal tool like TeraTerm to receive the logs from the virtual COM port.

![server console log](images/sensor_device_log.png)
