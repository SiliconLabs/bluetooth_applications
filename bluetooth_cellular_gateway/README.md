# Bluetooth Cellular Gateway with BG 96B #

This application is using Mikroe LTE IOT 2 CLICK shield on Silicon Laboratories BGM220 Explorer Kit Board (BRD4314A), and Hologram IO cloud service.

The application is using our BG_96 driver (https://github.com/SiliconLabs/platform_hardware_drivers/tree/master/cellular_gnss_bg96)

## Hardware Required ##

- Mikroe LTE IOT 2 CLICK, GNSS antenna, GSM antenna, SIM card

![](doc/bg96_module.jpg)

- BGM220 Explorer Kit Board (BRD4314A), USB mini cable

- Any Silicon Laboratories Thunderboard (in the example we tested with BG22 Thunderboard (BRD4108A), and Thunderboard Sense2 (BRD4166A))

## Software Required ##

- [Hologram.io kit](https://www.hologram.io/)
- Simplicity Studio 5
- GSDK v4.0.2
- TeraTerm or another serial console.

## About the setup ##

LTE IoT 2 click is a Click board™ that allows connection to the LTE networks, featuring Quectel BG96 LTE module, which offers two LTE technologies aimed at Machine to Machine communication (M2M) and Internet of Things (IoT). This module is an embedded IoT communication solution that supports the LTE Cat M1 and NB1 technologies, offering an alternative to similar Low Power Wide Area Network (LPWAN) solutions, such as the ones provided by Sigfox and LoRa. The LTE CAT1 and NB1 technologies are designed with specific requirements of the IoT network in mind. LTE IoT 2 click also offers various other features, allowing simple and reliable connection to these new 3GPP IoT technologies.

## Preparations ##

The application will run on the BGM220 Explorer Kit Board, with the fitted Mikroe LTE IOT 2 CLICK shield. You need to attach the cellular and GNSS antenna to the proper connectors (CN1 (Brown) is the GSM one, a spiky antenna, and CN2 (Blue) is the octagonal GNSS antenna). Place the GNSS antenna to be able detect GPS satellites. GSM service is also required in the area.

From Simplicity Studio create a Thunderboard sample app (or demo) and flash it to the Thunderboard, and power it.

Use the attached project for BMG220 (SimplicityStudio/Bluetooth-Cellular_Gateway.sls), import it to Simplicity Studio, and assign your hologram.io token to the variable **cloud_token[]** in the app.c file. Build the project and flashit to BGM220 Explorer Kit Board.

If you use another board than BGM220 Explorer Kit Board, you need to follow these steps:

1. Create a "BLE SOC empty Project" project for your board using Simplicity Studio v5. Use the default project settings. Be sure to connect and select the proper board Board from the "Debug Adapters" on the left before creating a project.

2. Overwrite the project's *app.c* with the Git repository's app.c .
3. set your hologram.io token for variable **cloud_token[]** in the app.c file.
4. Copy the **BG96_driver** folder from the Git repository to your Simplicity Studio project folder, and add to the include path:

![](doc/include_directory.png)

5. Install the software components:

- Open the .slcp file in the project.

- Select the SOFTWARE COMPONENTS tab.

- Install **[Services] > [IO Stream] > [IO Stream: USART]** component with the default instance name: **vcom**.

![](doc/iostream.png)

- Install **[Services] > [IO Stream] > [IO Stream: Retarget STDIO]** component

![](doc/iostream_retarget.png)

- Install the **[Services] > [Sleep Timer]** component.

- Add two pin instances to **[Peripherial] > [Init] >[GPIO init]**

with the names:
bg96_pwk

![](doc/pin_pwk.jpg)

bg96_sta ()

![](doc/pin_sta.jpg)

- Install the **[Platform] > [Utilities] > [Circular Queue]** component with a parameter value 20 Max Queue Length  

![](doc/Circular_queue.png)

![](doc/Circular_queue_set_to_20.png)

Install the [Platform] > [Driver] > [UART] > [UARTDRV Core] component, with a given parameters 
1. FLow control support: enable, maximum number of driver instances: 4
2. UART software flow control code: request peer to start TX: 17
3. UART software flow control code: request peer to stop TX: 19
4. Enable reception when sleeping: enable

![](doc/uart.png)

![](doc/UART_setting.png)

6. Build and flash the project to your BGM220 device.

## How to use ##

Power the Thunderboard. Connect the BMG220 with a micro usb cable to your computer, and open TeraTerm.

The serial window will inform you about collecting humidity, temperature, and GPS position. Values: "t" is temperature, 25°C is represented as 2500, "h" is humidity, 50% RHT is represented as 5000. Latitude and Longitude values are also present (southern hemisphere S, northern hemisphere N, eastern longitude E, western longitude: W) after the valid GPS position was received.

![](doc/TetaTerm.png)

The string containing these data will appear at the cloud provider.

![](doc/hologram_cloud.png)
