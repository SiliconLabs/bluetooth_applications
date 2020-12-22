# Explorer Kit Bluetooth barometer example using I2C bus DPS310 pressure sensor #

## Overview ##

This project shows an example of **Bluetooth Barometer** using the **Silicon Labs BGM220-EK4314A BGM220 Bluetooth Module Explorer Kit.**

It is using a reference barometer driver component for an **I2C-bus** connected **DPS310** barometric pressure sensor.

The application makes a pressure measurement every second when the Bluetooth connection is open. While the measurement is ongoing, the yellow LED is lit.

The pressure measurement can be manually read via Bluetooth Pressure-characteristic under Environmental Sensing-service or it can be also automatically updated using Notifications.

The sensor board used here is a **mikroE "Pressure 3 Click"** using **mikroE mikroBUS**-socket **I2C** connection. But also other **I2C** connected **DPS310**-sensors can be used here such as **SparkFun Qwiic Connect System**-compatible **Adafruit's DPS310**-based pressure sensor board. 

A target application of a barometer pressure sensor could be a weather station or an altitude sensor.

## Gecko SDK version ##

GSDK v3.0.0

## Hardware Required ##

- [**BGM220-EK4314A** BGM220 Bluetooth Module Explorer Kit (BRD4314A BGM220 Explorer Kit Board)](https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit)

Supported barometer boards:
- [**mikroE Pressure 3 click** digital barometric pressure sensor](https://www.mikroe.com/pressure-3-click), uses **I2C address 0x76** by default **(the example code default)**

or with a very minor changes also:
- [Adafruit DPS310 Precision Barometric Pressure / Altitude Sensor](https://www.adafruit.com/product/4494), uses **I2C address 0x77** by default (**the example code needs I2C address configuration**)


## Connections Required ##

The mikroBUS Pressure 3 Click board can be just "clicked" into it place. Be sure that the boards 45-degree corner matches the Explorer Kit's 45-degree white line. The board also has 10k I2C-bus pull-ups. Just be sure that the click board is configured into I2C-mode (the default) by the resistors and not into SPI-mode.

When using the Adafruit DPS310 Qwiic barometer sensor board, it can be easily connected by using one Qwiic cable. The board includes 10k I2C-bus pull-ups so it is ready to use.


## How It Works ##

The application is based into Bluetooth Empty-SoC example. And as the example already has Bluetooth GATT server, advertising and connection mechanisms operational, there are only minor changes.

Naturally the DPS310 driver with the configuration file needs to be added. And the DPS310 driver uses the I2C Simple Polled Master-driver so that is included as a software component.

barometer_config.h -file has the module I2C bus pin definitions and also the sensor I2C address. The default I2C address 0x76 is correct for the Pressure 3 click. But when using the Adafruit DPS310, it has to be changed into 0x77.

The GATT changes were adding a new Environmental Sensing service using UUID 0x181A that has a characteristic Pressure ​UUID 0x2A6D with Read and Notify properties. As it is a Bluetooth SIG Assigned UUID, most applications know how to display the value correctly. The Pressure characteristic size is 4 bytes and it is a 32-bit unsigned integer and the unit is pascals. This information can be seen from the Bluetooth GATT Configurator when Add:ing a Characteristic, see the Pressure there.

The application file app.c has barometer and LED initialization in the app_init(). When the connection is opened, a 1 second timer periodical is started. When the timer is triggered, then the LED is lit and the barometer measurement is started. When the measurement is ready, then the LED is turned off and the GATT characteristic is updated. If the notification was enabled, also the the client is notified about the value update. sl_bt_evt_gatt_server_characteristic_status_id-event is handling the notification enable/disable control. If the connection closed, also the barometer measurement timer is stopped.

More information about the barometer driver and it's usage can be found from the driver project and documentation.

The example can be used with a minor changes also in the other products.

# Create an example application #

Simplicity Studio 5 was used to create the example code.

You can either create an example application code as basis and modify it according to the instructions below or use the ready made .sls-project.

First create a "Bluetooth - SoC Empty" project for the" BGM220 Explorer Kit Board" using SimplicityStudio 5 Launcher-perspective EXAMPLE PROJECTS-tab. Use the default project settings. Be sure to connect and select the BGM220 Explorer Kit Board from the "Debug Adapters" on the left before creating a project.

Then copy the files *app.c*, *barometer.h*, *barometer_config.h* and *dps310.c* in to the project root folder (app.c is replacing the old app.c). Remember the correct I2C address, see above.

Next add the I2C Simple Polled Master-driver from the project .SLCP-file's SOFTWARE COMPONENTS-tab by adding "I2CSPM Core"-component. Use Search to find.

And last import the GATT configuration by using the SOFTWARE COMPONENTS-tab and open the "Bluetooth GATT Configurator" and use it's Import-button to import the *gatt_configuration.btconf*

Save the files, build and ready to flash or debug! To Build select the project from the "Simplicity IDE"-perspectives "Project Explorer" and then press the hammer sign from the above toolbar to build. If there were 0 warnings, then there should be a Binaries-folder in the project. Expand the folder and use right menu button for the .s37 file and select "Flash to Device". Flash Programmer dialog should be opened. It might ask to select the Explorer Kit Board first and to press the "Click to Query Lock Status".The correct file is selected so just select Program.

*Note*: The Explorer Kit also requires a bootloader to run the application. Most likely there is one already but in case not, the easiest way is to use for example iBeacon demo from the Launcher view DEMOS tab after selecting the Explorer Kit from the Debug Adapters.

## .sls Projects Used ##

explorer_kit_example_baro_mikrobus.sls - This application uses I2C address 0x76 that is the Pressure 3 click default

explorer_kit_example_baro_qwiic.sls - This application uses I2C address 0x77 that is the Adafruit DPS310 default

# More information #

## Barometer driver ##

More information about the barometer driver, how it works, architecture, state diagram, setup and so on, can be found from it's own reference driver component project: https://github.com/SiliconLabs/platform_hardware_drivers/tree/master/barometer

## mikroE mikroBUS ##

- a configurable header pair socket with 5V and 3.3V powers, SPI, I2C, UART and configurable pins such as Analog, PWM, Interrupt, Reset and Chip select

mikroBUS: https://www.mikroe.com/mikrobus

## SparkFun Qwiic Connect System ##

- a 4-pin bus carrying I2C-bus and 3.3V powers

Qwiic Connect System: https://www.sparkfun.com/qwiic
