<table border="0">
  <tr>
    <td align="left" valign="middle">
    <h1>EFR32 Bluetooth Application Examples</h1>
  </td>
  <td align="left" valign="middle">
    <a href="https://www.silabs.com/wireless/bluetooth">
      <img src="http://pages.silabs.com/rs/634-SLU-379/images/WGX-transparent.png"  title="Silicon Labs Gecko and Wireless Gecko MCUs" alt="EFM32 32-bit Microcontrollers" width="250"/>
    </a>
  </td>
  </tr>
</table>

# Silicon Labs Bluetooth Applications #

The Silicon Labs Bluetooth stack allows for a wide variety applications to be built on its foundation. This repo showcases some example applications using the Silicon Labs Bluetooth stack.

This repository provides both SLCP projects (as External Repositories) and SLS projects as standalone projects, which are configured for development boards.

## Examples ##

| No | Example name | Link to example |
|:--:|:-------------|:---------------:|
| 1  | Bluetooth - AI/ML Hand Signal Recognition (MLX90640) | [Click Here](./bluetooth_ai_ml_hand_signal_recognition) |
| 2  | Bluetooth - People Counting (AK9753) | [Click Here](./bluetooth_people_counting_ak9753) |
| 3  | Bluetooth - Door Lock RFID (ID-12LA) | [Click Here](./bluetooth_door_lock_rfid) |
| 4  | Bluetooth - Soc Blinky - Sparkfun Thing Plus Matter - MGM240P | [Click Here](./bluetooth_soc_blinky_sparkfun_thingplus) |
| 5  | Bluetooth - Dosimeter (Sparkfun Type 5) | [Click Here](./bluetooth_dosimeter) |

## Requirements ##

1. Silicon Labs EFR32 Development Kit
2. Simplicity Studio 5
3. Gecko SDK Suite 4.2.1, available via Simplicity Studio or [here](https://github.com/SiliconLabs/gecko_sdk)
4. Third-Party Hardware Drivers extension, available [here](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Working with Projects ##

1. To add an external repository, perform the following steps.

    - From Simpilicity Studio 5, go to **Preferences > Simplicity Studio > External Repos**. Here you can add the repo `https://github.com/SiliconLabs/bluetooth_applications.git`. 

    - Cloning and then selecting the branch, tag, or commit to add. The default branch is Master. This repo cloned to `<path_to_the_SimplicityStudio_v5>\developer\repos\`

2. From Launcher, select your device from the "Debug Adapters" on the left before creating a project. Then click the **EXAMPLE PROJECTS & DEMOS** tab -> check **bluetooth_applications** under **Provider** to show a list of Bluetooth example projects compatible with the selected device. Click CREATE on a project to generate a new application from the selected template.

## Legacy Projects - Importing *.sls projects ###

1. Place the *.sls file(s) to be imported in a folder.

2. From Simpilicity Studio 5, select **File > Import**, select the folder containing *.sls file(s). Select a project from the detected projects list and click on Next. Name the project and click Finish.

See [Import and Export](https://docs.silabs.com/simplicity-studio-5-users-guide/5.6.0/ss-5-users-guide-about-the-simplicity-ide/import-and-export) for more information.

## Porting to Another Board ##

To change the target board, navigate to Project -> Properties -> C/C++ Build -> Board/Part/SDK. Start typing in the Boards search box and locate the desired development board, then click Apply to change the project settings. Ensure that the board specifics include paths, found in Project -> Properties -> C/C++ General -> Paths and Symbols, correctly match the target board.

## Bootloader ##

Note that example projects do not include bootloaders regardless of Bluetooth-based example projects requiring a bootloader on the device to support device firmware upgrade (DFU). To have a running application, you should either

- flash the proper bootloader or
- remove the DFU functionality from the project.

If your application does not require adding a bootloader, remove the DFU functionality by uninstalling the Bootloader Application Interface software component including all of its dependents. This automatically puts your application code to the start address of the flash, which means that a bootloader is no longer needed. Note that you are not able to upgrade your firmware after the removal of the DFU functionality.

If you want to add a bootloader, you can either

- Create a bootloader project, build it and flash it to your device. Note that different projects expect different bootloaders:

  - for NCP and RCP projects create a BGAPI UART DFU type bootloader
  - for SoC projects on Series 1 devices create a Bluetooth in-place OTA DFU type bootloader - or any Internal Storage type bootloader
  - for SoC projects on Series 2 devices create a Bluetooth Apploader OTA DFU type bootloader

- or run a pre-compiled demo on your device from the Launcher view before flashing your application. Pre-compiled demos flash both bootloader and application images to the device. Flashing your own application image after the demo overwrites the demo application, but leave the bootloader in place.

  - For NCP and RCP projects, flash the Bluetooth - NCP demo.
  - For SoC projects, flash the Bluetooth - SoC Thermometer demo.

Important Notes:

- when you flash your application image to the device, use the .hex or .s37 output file. Flashing .bin files may overwrite (erase) the bootloader.

- On Series 1 devices (EFR32xG1x), both first stage and second stage bootloaders have to be flashed. This can be done at once by flashing the -combined.s37 file found in the bootloader project after building the project.

- On Series 2 devices SoC example projects require a Bluetooth Apploader OTA DFU type bootloader by default. This bootloader needs a lot of flash space and does not fit into the regular bootloader area, hence the application start address must be shifted. This shift is automatically done by the Apploader Support for Applications software component, which is installed by default. If you want to use any other bootloader type, you should remove this software component in order to shift the application start address back to the end of the regular bootloader area. Note, that in this case you cannot do OTA DFU with Apploader, but you can still implement application-level OTA DFU by installing the Application OTA DFU software component instead of In-place OTA DFU.

For more information on bootloaders, see [UG103.6: Bootloader Fundamentals](https://www.silabs.com/documents/public/user-guides/ug103-06-fundamentals-bootloading.pdf) and [UG489: Silicon Labs Gecko Bootloader User's Guide for GSDK 4.0 and Higher](https://www.silabs.com/documents/public/user-guides/ug489-gecko-bootloader-user-guide-gsdk-4.pdf).

## Documentation ##

The official Bluetooth documentation is available on the [Developer Documentation](https://docs.silabs.com/bluetooth/latest/) page.

## Reporting Bugs/Issues and Posting Questions and Comments ##

To report bugs in the Application Examples projects, please create a new "Issue" in the "Issues" section of this repo. Please reference the board, project, and source files associated with the bug, and reference line numbers. If you are proposing a fix, also include information on the proposed fix. Since these examples are provided as-is, there is no guarantee that these examples will be updated to fix these issues.

Questions and comments related to these examples should be made by creating a new "Issue" in the "Issues" section of this repo.

## Disclaimer ##

The Gecko SDK suite supports development with Silicon Labs IoT SoC and module devices. Unless otherwise specified in the specific directory, all examples are considered to be EXPERIMENTAL QUALITY which implies that the code provided in the repos has not been formally tested and is provided as-is.  It is not suitable for production environments.  In addition, this code will not be maintained and there may be no bug maintenance planned for these resources. Silicon Labs may update projects from time to time.

