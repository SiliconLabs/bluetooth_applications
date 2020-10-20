# BLE IR Generator Example #

## Summary ##

This project shows the implementation of IR signal generate and 4x4 matrix key scan with BLE on our EFR32 device(lynx). 
The expectation is ensure IR signal generate work well in case that BLE in a heavy communication.
<div align="left">
  <img src="./doc/images/framework.png" height="480">
</div>

## Gecko SDK Version ##

v2.7

## Hardware Required ##

- BRD4182A EFR32xG22 radio board
- BRD4001A WSTK board

## Setup ##
1. Hardware connection.

   ![](doc/images/hardware_connection.png)

   Connect [matrix(4x4) key pad](https://www.amazon.com/Tegg-Matrix-Button-Arduino-Raspberry/dp/B07QKCQGXS/ref=sr_1_4?dchild=1&keywords=Key+matrix&qid=1591754882&sr=8-4) and [Infrared diode](https://www.amazon.com/Digital-Receiver-Transmitter-Arduino-Compatible/dp/B01E20VQD8/ref=sr_1_14?dchild=1&keywords=IR+receiver&qid=1591754671&s=aht&sr=1-14) to WSTK board through the expansion header. 

2. Import the bluetooth_ir_generator.sls file, build and program the EFR32 device with the hex file.

## How It Works ##

### Keypad

Most of time the system is stay in idle status. When key active, GPIO interrupt wakeup system, then key scan and key timer(10ms) start to work. When key is available or key release, key callback is invoked to indicate which key is detected or release, after key release the system back to idle again.
- Initialization.
    - key_init() function initialize the key pad with 2 callback, set GPIO direction and interrupt.
    - key_wakeup_callback_t wakeup_cb, is called in GPIO IRQ. It start the key timer.
    - key_callback_t cb, is called in the key detection, report which key is detected and key release.
- Running the key Detection
    - key_scan() functions run in a key time slice, check which key and how long is pressed then report key status.

#### Keypad flowchart

<div align="left">
  <img src="./doc/images/keypad.png" height="480">
</div>

### IR generate

Most of time the system is stay in idle status.When IR send is required, configure the stream according to given data, send out all the stream bit. If don't got stop command, it keep repeat. If no need to repeat the IR signal, system will back to idle status.  
- Initialization.
    - ir_generate_init() function initialize the key pad with callback.
    - code_t ir_code, set the IR protocol, currently support NEC and SONY type.
    - ir_callback_t cb, is called if one frame stream is sent.
- Running the IR generate
    - ir_generate_stream() function configure the data that desire to send and start, repeat flag use in NEC IR protocol.
    - ir_generate_stop() function can stop the IR generate.

#### IR flowchart

<div align="left">
  <img src="./doc/images/ir.png" height="480">
</div>

In this example IR stream start/stop is control by key event. We can use tera term for tracking the run status and logic analyzer/oscilloscope for check the IR waveform.

## .sls Projects Used ##

-bluetooth_ir_generator.sls

## How to Port to Another Part ##

1. Create a new SoC-Empty project for target device.

2. Import the gatt.xml file.

3. Press Save and then Generate in the GATT Configurator.

4. Copy the relevant files(app.c,app.h,ir_generate.c,ir_generate.h,key_scan.c,key_scan.h) into your project (overwriting the existing one).

5. Copy the src directory from ..\SiliconLabs\Simplicity\Studio\v4\developer\sdks\gecko_sdk_suite\v2.7\platform\emdrv\gpiointerrupt to your project's ..\platform\emdrv\gpiointerrupt directory.

6. Build and flash the project to your device.