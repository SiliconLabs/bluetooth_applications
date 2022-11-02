# Explorer Kit Bluetooth accelerometer example using I2C bus BMA400 accelerometer #
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_explorer_kit_i2c_accelerometer_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_explorer_kit_i2c_accelerometer_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_explorer_kit_i2c_accelerometer_common.json&label=License&query=license&color=green)
![SDK badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_explorer_kit_i2c_accelerometer_common.json&label=SDK&query=sdk&color=green)
[![GitHub](https://img.shields.io/badge/Mikroe-ACCEL%205%20CLICK-green)](https://www.mikroe.com/accel-5-click)
![Build badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/bluetooth_explorer_kit_i2c_accelerometer_build_status.json)

## Overview ##

This project shows an example of **Bluetooth Accelerometer** using the **Silicon Labs BGM220-EK4314A BGM220 Bluetooth Module Explorer Kit.**

It is using a reference accelerometer driver component for an **I2C-bus** connected **BMA400** accelerometer sensor. The BMA400 can be also used via SPI-bus but it is not demonstrated in this example.

The application makes an accelerometer measurement every second when the Bluetooth connection is open. While the measurement is ongoing, the yellow LED is lit. So in a normal situation it is blinking once per second.

The acceleration measurement can be seen via 3-byte custom characteristic under custom service by reading it manually or it can be also automatically updated using Notifications.

The sensor board used here is a **mikroE "Accel 5 Click"** using **mikroE mikroBUS**-socket **I2C** connection. But also other I2C connected BMA400-sensors can be used here.

A target application of an accelerator sensor could be sensing the movement, orientation, vibration or even a falling situation.

## Simplicity Studio and Gecko SDK Suite version ##

Simplicity Studio SV5.1.1.0 and GSDK v3.1.1

## Hardware Required ##

- [**BGM220-EK4314A** BGM220 Bluetooth Module Explorer Kit (BRD4314A BGM220 Explorer Kit Board)](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)

Supported accelerometer boards:
- [**mikroE Accel 5 click** ultra-low power triaxial accelerometer sensor](https://www.mikroe.com/accel-5-click)

## Connections Required ##

The mikroBUS Accel 5 Click board can be just "clicked" into it place. Be sure that the boards 45-degree corner matches the Explorer Kit's 45-degree white line. The board also has 4.7k I2C-bus pull-ups. Just be sure that the click board is configured into I2C-mode (the default) by the resistors and not into SPI-mode. Also the application uses by default **I2C address 0x15** as it is the Accel 5 click default (the resistor labeled "I2C ADD" is on the "1". If setting "I2C ADD" resistor "0", the address will be 0x14).

## How It Works ##

The application is based into Bluetooth Empty-SoC example. And as the example already has Bluetooth GATT server, advertising and connection mechanisms operational, there are only minor changes.

Naturally the BMA400 driver with the configuration file needs to be added. The BMA400 driver has built-in polled master driver so only addiing I2C peripheral component is required. BMA400 uses USTIMER for timing the operations.

bma400_config.h -file has the module I2C bus pin definitions and also the sensor I2C address. The default I2C address is BMA400_I2C_ADDRESS_SDO_HIGH (i.e. 0x15 that is defined as "(0x15<1)") is correct for the Accel 5 click default settings. But when using the Adafruit DPS310, it has to be changed into 0x77.

The GATT changes were adding a new custom service using UUID ```03519cae-ce32-44be-ad30-e2c7d068da03``` that has a characteristic UUID ```27038e55-8e48-b5f1-6e20-84a124258810``` with Read and Notify properties. The acceleration characteristic is 3 bytes containing 1 byte per X-, Y- and Z-axis accelerations. The axis values are absolute accelerations where 1 g is about value of 32. When using EFR Commander, it is easiest look for the values as 3 separate decimal values, typically "0 0 31" when the board is on a level plane like a table.

The application file app.c has accelerometer and LED initialization in the app_init(). The LED blinks once if the accelerometer initialization is successful. If the LED stays on, the initialization has been failed. The reason is typically wrong sensor I2C address (see "I2C ADD" resistors) or wrongly configured Click board mode (SPI-mode instead I2C) or if using some own ways to connect the sensor, various other reasons as we are in the embedded world. But no panic, in most cases, both sensors work without any issues if used without any modifications.

When the connection is opened, a 1 second timer periodical is started. When the timer is triggered, then the LED is lit and the accelerometer measurement done. Then the LED is turned off and the GATT characteristic is updated. If the notification was enabled, also the the client is notified about the value update. sl_bt_evt_gatt_server_characteristic_status_id-event is handling the notification enable/disable control. If the connection closed, also the accelerometer measurement timer is stopped.

More information about the accelerometer driver and it's usage can be found from the driver project and documentation.

The example can be used with a minor changes also in the other Silabs Bluetooth products.

# Create an example application #

Simplicity Studio 5-series was used to create the example code.

You can either create an example application code as basis and modify it according to the instructions below or import the ready made .sls-project.

First create a "Bluetooth - SoC Empty" project for the" BGM220 Explorer Kit Board" using SimplicityStudio 5 Launcher-perspective EXAMPLE PROJECTS-tab. Use the default project settings. Be sure to connect and select the BGM220 Explorer Kit Board from the "Debug Adapters" on the left before creating a project.

Then copy the files *app.c*, *bma400.h*, *bma400_conf.h* and *bma400.c* in to the project root folder (app.c is replacing the old app.c).

Next add the *USTIMER - Microsecond Timer* from the project .SLCP-file's SOFTWARE COMPONENTS-tab by adding "USTIMER"-component. Use Search to find. Also add the *I2C - Inter-Integrated Circuit* by adding the peripheral "I2C"-component the similar way.

And last import the GATT configuration by using the SOFTWARE COMPONENTS-tab and open the "Bluetooth GATT Configurator" and use it's Import-button to import the *gatt_configuration.btconf*

Save the files, build and ready to flash or debug! To Build select the project from the "Simplicity IDE"-perspectives "Project Explorer" and then press the hammer sign from the above toolbar to build. If there were 0 errors, then there should be a Binaries-folder in the project. Expand the folder and use right menu button for the .s37 file and select "Flash to Device". Flash Programmer dialog should be opened. It might ask to select the Explorer Kit Board first and to press the "Click to Query Lock Status". The correct file is selected so just select Program.

*Note*: The Explorer Kit also requires a bootloader to run the application. Most likely there is one already but in case not, the easiest way is to use for example iBeacon demo from the Launcher view DEMOS tab after selecting the Explorer Kit from the Debug Adapters.

## .sls Projects Used ##

_explorer_kit_example_i2c_mikrobus_accel_gsdk311.sls_ - Import this project to have a ready-to-compile -project.

Also precompiled binaries in S-Record format (.s37) are included for the projects above test the applications instantly. The files can be programmed using for example _Simplicity Studio Flash Programmer_ tool or _Simplicity Commander_ application. Remember to flash also the bootloader at least once, it is also included for a smooth experience.

# More information #

## Accelerometer driver ##

More information about the accelerometer driver, how it works and so on, can be found from it's own reference driver component project.


## mikroE mikroBUS ##

- a configurable header pair socket with 5V and 3.3V powers, SPI, I2C, UART and configurable pins such as Analog, PWM, Interrupt, Reset and Chip select

mikroBUS: https://www.mikroe.com/mikrobus
