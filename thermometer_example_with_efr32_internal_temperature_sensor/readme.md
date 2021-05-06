# Thermometer Example with EFR32 Internal Temperature Sensor #
 
## Description ##
 
This example is an adaptation of the standard 'SOC - Thermometer' example. However, instead of accessing the Si7021 Temperature and Relative Humidity sensor through I2C, this example uses the EFR32's own internal temperature sensor. This sensor is measured during the production test. The temperature readout from the ADC at production temperature as well as the Celsius value are given in the device information page. Using these and the millivolts per degrees slope in the sensor data sheet, the current temperature can be calculated as follows:

```
T_Celsius = T_Calibration - (ADC_Calibration_Reading - ADC_Current_Reading) * V_Ref / (4096 * Slope)
```

For more information, see the ADC section of the reference manual of your chosen hardware, e.g., [EFR32xG12](https://www.silabs.com/documents/public/reference-manuals/efr32xg12-rm.pdf).

These are the changes to the basic SoC-Thermometer example:

- *init_adc* function to initialize ADC with correct voltage reference and prescaler.

- *read_adc* function to get single sample readout from ADC, which has a busy-wait-loop for simplicity.

- *convert_to_millicelsius* function takes the sample and uses the calibration values in the above formula to get the result as degrees Celsius. The returned value is given in millicelsius.

- *measure_temperature* function is nearly identical to the one in the Thermometer example except for this line that uses the above functions:

```C
temperature_data = convert_to_millicelsius(read_adc());
```

## Gecko SDK version ##
 
GSDK v3.1.1
 
## Hardware Required ##
 
- A WSTK board.
- A Bluetooth capable radio board, e.g: BRD4161A.
 
## Setup
 
1. Create a **Bluetooth - SoC Empty** project.

2. Import the GATT configuration:
   - Open the .slcp file in the project.
   - Select the CONFIGURATION TOOLS tab and open the "Bluetooth GATT Configurator".
   - Find the Import button and import the attached *gatt_configuration.btconf* file.
   - Save the GATT configuration (ctrl-s).

![](images/import_gatt_configuaration.png)

3. Open the .slcp file again. Select the "SOFTWARE COMPONENTS" tab and do the following changes:

- Install **IO Stream: USART** component with the default instance name: **vcom**.

![](images/install_usart.png)

- Find the **Board Control** component  and enable *Virtual COM UART* under its configuration.

- Install the **Log** component (found under Bluetooth > Utility group).

4. Replace the *app.c* file in the project with the provided *app.c*.

5. Build and flash to the target.

6. Do not forget to flash a bootloader to your board, if you have not done so already.
 
## How It Works ##
 
Follow the below steps to test the example:

1. Open the EFR Connect app on your smartphone.

2. Find your device in the Health Thermometer, advertising as IntTemp and connect to it.

![](images/htm_scan.png)

*HTM Scan*

![](images/htm_reading.png)

*HTM Reading*

You can launch the Console that is integrated on Simplicity Studio or can use a third-party terminal tool like TeraTerm to receive the logs from the virtual COM port.

![](images/console.png)
