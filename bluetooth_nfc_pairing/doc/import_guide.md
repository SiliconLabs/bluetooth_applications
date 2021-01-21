# SDK 3.x (Studio v5) Import Instructions

This guide is using BGM220P Explorer Kit (BRD4314A) as example.

1. Clone [platform hardware driver](https://github.com/SiliconLabs/platform_hardware_drivers) to somewhere. We will be needing specific nfc controller driver.

2. Clone [nfc middleware](https://github.com/SiliconLabs/nfc). We will be needing the nfc library.

3. Clone this repository.

4. Create an SOC - Empty project through MCU Project for the radio board you are using.
    1. In launcher view, select your device and create click "Create New Project" 

        <img src="images/ig_v5_new_proj.png" width="1500">

    2. Select bluetooth in the left column then select "Bluetooth - SoC Empty" and click "NEXT" 

        <img src="images/ig_v5_soc_empty.png" width="600">

    3. Give your project a good name and click "FINISH"

        <img src="images/ig_v5_proj_name.png" width="600">
 
5. Drag [ble_dbg_v5](ble_dbg_v5) folder into the project. These files are helper functions that print bluetooths event and related information, crucial for monitoring the pairing process.

    <img src="images/ig_v5_ble_dbg_v5.png" width="600">

6. Drag nfc controller driver from [platform hardware driver](https://github.com/SiliconLabs/platform_hardware_drivers) into the project. See project README for needed nfc controller driver. Using nt3th2x11 driver as example here.

    <img src="images/ig_v5_nt3h2x11_driver.png" width="600">

7. Drag [nfc_library](https://github.com/SiliconLabs/nfc/nfc_library) from [nfc middleware](https://github.com/SiliconLabs/nfc) into the project.

    <img src="images/ig_v5_nfc_library.png" width="600">

8. Replace main.c with main.c in src folder in each project. Using a NT3H2x11 project main.c as example here.

    <img src="images/ig_v5_app_c_replacement.png" width="600">

9. Add include paths through project properties. Using a NT3H2x11 path as example here.

    Right click to find "Properties".

    <img src="images/ig_proj_properties.png" width="400">

    Add path as shown below. 

    <img src="images/ig_add_inc_path.png" width="700">

10. Make sure all paths are added. See project README for needed include paths. Using a NT3H2x11 project as example here.

    <img src="images/ig_inc_paths.png" width="700">

11. Modify main.c board macro definition to match the radio board you have. Using BRD4182A and BRD4314A as example here.

    <img src="images/ig_board_num_brd4182a.jpg" height="500">

    <img src="images/ig_board_num_brd4314a.jpg" height="500">
    
    <br>

    <img src="images/ig_v5_change_board_num.png" width="400">

    This would set up the right pin definitions.

    <img src="images/ig_pinout.png" width="400">

12. Open project configurator and add I2C component to the project.

    Open project configurator (.slcp file).

    <img src="images/ig_v5_slcp.png" width="400">

    Click on "software components".

    <img src="images/ig_v5_software_components.png" width="800">

    Go to Platform -> Peripheral -> I2C. Click "Install".

    <img src="images/ig_v5_install_i2c.png" width="800">

13. Stay inside project configurator. 

    Add IO stream: USART. This is the dependency for serial output.

    <img src="images/ig_v5_io_stream.png" width="800">

    Use default vcom instance name.

    <img src="images/ig_v5_vcom.png" width="400">

    Add IO Stream: Retarget STDIO. This would allow us to retarget printf to serial output.

    <img src="images/ig_v5_retarget.png" width="800">


14. Project should build and run. 
