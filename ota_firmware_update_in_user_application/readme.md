
# Implementing OTA Firmware Update in User Application

## Background

This code example has a related User's Guide, here: [Uploading Firmware Images Using OTA DFU](https://docs.silabs.com/bluetooth/latest/general/firmware-upgrade/uploading-firmware-images-using-ota-dfu)



## Description

The article referred in the [Background](#background) section discusses two methods of performing OTA:

1. Using Apploader

2. Using user application

This code example demonstrates the second method. See  the article [Uploading Firmware Images Using OTA DFU](https://docs.silabs.com/bluetooth/latest/general/firmware-upgrade/uploading-firmware-images-using-ota-dfu) for conceptual details.

The code sample provided in [AN1086: Using the Gecko Bootloader with the Silicon Labs Bluetooth® Applications](https://www.silabs.com/documents/public/application-notes/an1086-gecko-bootloader-bluetooth.pdf) is simplified to fit in one page. The full example provided here includes some additional debug prints and features, such as printing the estimated data transfer rate.

The main functional difference is related to erasing the download area. In the simplified code, the download area is erased when the remote OTA client starts the OTA process (writing value 0 to *ota_control*). In this example, the download area is erased at startup. The code also reads the content of the download area and does an erase only if needed (i.e., if the download area is not already blank) to avoid dropping connection because of the supervision timeout. Erasing the whole download area (256k or more) will take several seconds (it is a blocking function call) and this can lead to supervision timeout unless the connection parameters are specifically adjusted to prevent it.



## Setting up

1. Before installing this example, make sure you have flashed a suitable Gecko bootloader project to your device
   1. In Simplicity Studio create a new Internal Storage Bootloader or an SPI Flash Storage Bootloader. (Other bootloaders can also be used, but you must make sure, that at least one storage slot is defined for your bootloader.)
   2. In the storage configuration of the bootloader make sure that your storage slot does not overlap with the NVM3 area of your application.
   3. Build the project
   4. Flash the bootloader to your device. On series 1 devices (EFR32xG1x) make sure, that you flash the <bootloader_name>-combined.s37 file!
2. Create a Bluetooth - SoC Empty project for your radio board.
3. Since in this project we do not use Apploader for firmware upgrade, remove Apploader from your application

   1. Open the Project Configurator (that is, open the .slcp file in your project and select the Software Components tab)

   2. Find the **OTA DFU** software component (Bluetooth > Utility > OTA DFU), and uninstall it
4. Import the attached GATT configuration file into the project.

   1. In the Project Configurator find Advanced Configurators / Bluetooth GATT Configurator and open it
   2. In BLE GATT Configurator, click on `import`
   3. When the **File Importer** window is opened, drag and drop the attached `gatt_configuration.btconf` file.
   4. Save and close the GATT configurator.
5. Enable debug prints.

   1. Open the Project Configurator

   2. Find the **IO Stream: USART** component (Services > IO Stream > IO Stream: USART), and install it

   3. Find the **Log** component (Bluetooth > Utility > Log), and install it

   4. Navigate to the  **Platform** > **Board** > **Board Control**  software component, open its configurator and **Enable Virtual COM UART**
6. Copy the attached `app.c` file to the project (overwrites the default `app.c` from soc-empty project)
7. Build the project and flash it to your target.



## Usage

You can generate OTA files by running the create_bl_files script in your project (Note: you may need to set up some environmental variables first as described in section 3.10 of [AN1086: Using the Gecko Bootloader with the Silicon Labs Bluetooth® Applications](https://www.silabs.com/documents/public/application-notes/an1086-gecko-bootloader-bluetooth.pdf)).

Once you generated the upgrade files

1. Copy them to your smartphone
2. Open EFR Connect on your phone
3. Find your device (advertising as Application OTA) in the Bluetooth Browser and connect to it
4. In the context menu select OTA
5. Try OTA using the full.gbl file.

You can check the debug prints to make sure that the application can detect the bootloader version and download area information correctly as the first test before trying any updates. Below is a sample of the debug prints when running on a EFR32xG13 device: (with 512k flash).

```c
soc_empty initialised.
Bluetooth stack booted: v3.1.0-b178
Bluetooth public device address: 08:6B:D7:C3:7D:E5
Boot event - started advertising
Gecko bootloader version: 1.12
Slot 0 starts @ 0x00050000, size 131072 bytes
```

