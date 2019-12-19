# BLE man-in-the-middle example #

## Summary ##

This project shows the implementation of Man-In-The-Middle with BLE. It also demonstrates how to solve this vunerablility.

## Gecko SDK version ##

v2.7

## Hardware Required ##

- Two BRD4162A EFR32MG12 radio boards
- Two BRD4001A WSTK boards

## Setup ##

To test the man-in-the-middle(MITM), you need two WSTK boards. One board needs to be programmed with the bluetooth_soc-mitm_mg12.sls project and the other needs the basic SOC Health Thermometer example provided by Simplicity Studio.

Turn both of the devices on and click on Bluetooth Browse in the Blue Gecko App. The regular server will be named "Thermometer Example" and the MITM will be named "Thermometer Example M". If the MITM device already intercepted the connection, you should only see "Thermometer Example M".

Notice the values for the temperature measurement characteristic has its sign flipped (negative instead of positive).

## How It Works ##

1. The MITM device will connect to the real Health Thermometer server (an example project already included in Simplicity) and then advertise itself as the real server.
2. A smart phone will connect to the MITM device.
3. The MITM will get the real data from the real Health Thermometer server, switch the signs (i.e. make the value be negative) and send the wrong data to the smartphone.

Another project has been provided showing the initialization and configuration with the Bluetooth Security Manager API to prevent MITM. The bluetooth_soc_thermometer_auth_mg12.sls shows how to initialize the security manager in the system_boot_id event. The temperature measurement characteristic is configured with authenticated notify. Replace the SOC Health Thermometer board with the generated code fro the bluetooth_soc_thermometer_auth_mg12.sls project.

## .sls Projects Used ##

- bluetooth_soc-mitm_mg12.sls
- bluetooth_soc_thermometer_auth_mg12.sls

## How to Port to Another Part ##

Open the "Project Properties" and navigate to the "C/C++ Build -> Board/Part/SDK" item. Select the new board or part to target and "Apply" the changes. Note: there may be dependencies that need to be resolved when changing the target architecture.
