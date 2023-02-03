# Hand Signal Recognition #

## Overview ##

This application uses TensorFlow Lite for Microcontrollers to run image classification machine learning models to detect the hand gestures from image data recorded from a Far Infrared Sensor. The detection is visualized using the OLED and the classification results are written to the VCOM serial port.
Additionally, the classification results are transmitted to a connected Bluetooth Low Energy (BLE) client.

The hand gestures:

- Nothing
- Thumbs Up
- Thumbs Down

## Gecko SDK Suite version ##

- GSDK v4.2.0
- [Third Party Hardware Drivers v1.1.0](https://github.com/SiliconLabs/third_party_hw_drivers_extension)

## Hardware Required ##

- [EFR32xG24 Dev Kit (BRD2601B)](https://www.silabs.com/development-tools/wireless/efr32xg24-dev-kit)

- [SparkFun Micro OLED Breakout (Qwiic) board](https://www.sparkfun.com/products/14532)

- [Sparkfun MLX90640 IR Array (MLX90640 FIR sensor)](https://www.sparkfun.com/products/14844)

## Connections Required ##

The SparkFun Micro OLED Breakout (Qwiic) board and the Sparkfun MLX90640 IR Array can be easily connected with the EFR32xG24 Dev Kit by using the Qwiic cable.

![board](images/connection.png)

## Setup ##

To test this application, you can either import the provided [bt_hand_signal.sls](SimplicityStudio/bt_hand_signal.sls) project file or start with an empty example project as the following:

1. Create a **Bluetooth - SoC Empty** project for the **BRD2601B EFR32xG24 Dev Kit** using Simplicity Studio 5.

2. Copy all attached files in *inc* and *src* folders into the project root folder (overwriting existing).

3. Load the model file into the project:

    Create a tflite directory inside the config directory of the project and then copy the [.tflite](config/tflite/thumbs_up_and_down.tflite) model file into it. The project configurator provides a tool that will automatically convert .tflite files into sl_tflite_micro_model source and header files.

4. Import the GATT configuration:

   - Open the .slcp file in the project.

   - Select the **CONFIGURATION TOOLS** tab and open the **Bluetooth GATT Configurator**.

   - Find the Import button and import the attached [gatt_configuration.btconf](config/btconf/gatt_configuration.btconf) file.

   - Save the GATT configuration (ctrl-s).

5. Open the .slcp file. Select the SOFTWARE COMPONENTS tab and install the software components:

    - [Services] → [IO Stream] → [IO Stream: EUSART] → default instance name: vcom
    - [Application] → [Utility] → [Log]
    - [Third Party] → [Tiny Printf]
    - [Platform] → [Driver] → [LED] → [Simple LED] → default instance name: **led0**.
    - [Platform] → [Toolchain] → [Memory configuration] → Set this component to use 10240 Stack size and 12288 Heap size.

        ![mem_config](images/mem_config.png)

    - [Machine Learning] → [Kernels] → [TensorFlow Lite Micro] → Configure this component to use 9000 Tensor Arena Size.

        ![tflite_configure](images/tflite_configure.png)

    - [Third Party Hardware Drivers] → [Display & LED] → [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C] → use default configuaration

        ![ssd1306_config](images/ssd1306_config.png)

    - [Third Party Hardware Drivers] → [Service] → [GLIB - OLED Graphics Library]

    - [Third Party Hardware Drivers] → [Sensors] → [MLX90640 - IR Array Breakout (Sparkfun)]

    - Open [Platform] → [Driver] → [I2C] → [I2CSPM] → [inst0] → Set this component to use I2C1 peripheral, SCL to PC04 pin, SDA to PC05 pin and speed mode to fast mode (400kbit/s).

        ![i2c qwiic](images/i2c_instance.png)

6. Build and flash the project to your device.

**NOTE:**

- Make sure the SDK extension already be installed. If not please follow [this documentation](https://github.com/SiliconLabs/third_party_hw_drivers_extension/blob/master/README.md).

- SDK Extension must be enabled for the project to install [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C], [GLIB - OLED Graphics Library] and [MLX90640 - IR Array Breakout (Sparkfun)] components. Selecting [SSD1306 - Micro OLED Breakout (Sparkfun) - I2C] and [MLX90640 - IR Array Breakout (Sparkfun)] components will also include the "I2CSPM" component with unconfigurated instance: **inst0**.

- You need to create the bootloader project and flash it to the device before flashing the application. When flash the application image to the device, use the .hex or .s37 output file. Flashing the .bin files may overwrite (erase) the bootloader.

- There are many warnings when you try to build the project. These warnings come from the GSDK and you can ignore them.

## Hand Signal Model ##

### Model overview ###

Before continuing with this project, it is recommended to review the [MLTK Overview](https://siliconlabs.github.io/mltk/docs/overview.html), which provides an overview of the core concepts used by the this project.

Image classification is one of the most important applications of deep learning and Artificial Intelligence. Image classification refers to assigning labels to images based on certain characteristics or features present in them. The algorithm identifies these features and uses them to differentiate between different images and assign labels to them [[1]](https://www.simplilearn.com/tutorials/deep-learning-tutorial/guide-to-building-powerful-keras-image-classification-models).

In this project, we have a dataset with three different image types:

- **Thumbs up** - Images of a person's hand making a "like" gesture
- **Thumbs down** - Images of a persons's hand making a "dislike" gesture
- **Nothing** - Random images not containing any of the above

We assign an ID, a.k.a. **label**, 0-2, to each of these classes.  
We then "train" a machine learning model so that when we input an image from one of the classes is given to the model, the model's output is the corresponding class ID. In this way, at runtime on the embedded device when the camera captures an image of a person's hand, the ML model predicts its corresponding class ID which the firmware application uses accordingly. i.e.

![model overview](images/model_overview.png)

### Dataset Model ###

The most important part of a machine learning model is the dataset that was used to train the model.

Class: [Thumbs up](dataset/thumbs_up.zip)

![thumbs up](images/dataset_up.png)

Class: [Thumbs down](dataset/thumbs_down.zip)

![thumbs up](images/dataset_down.png)

Class: [Nothing](dataset/nothing.zip)

![thumbs up](images/dataset_nothing.png)

### Model Input ###

The model input should have the shape:  `<image height>  x <image width> x <image channels>`  
where:

- `<image height>`: 24
- `<image width>`: 32
- `<image channels>`: 1

The datatype should be `float32`.

### Model Input Normalization ###

The application supports "normalizing" the input.

If the `samplewise_norm.rescale` [model parameter](https://siliconlabs.github.io/mltk/docs/guides/model_parameters.html#imagedatasetmixin)
is given, then each element in the image is multiplied by this scaling factor, i.e.:

```c
model_input_tensor = img * samplewise_norm.rescale
```

If the `samplewise_norm.mean_and_std` [model parameter](https://siliconlabs.github.io/mltk/docs/guides/model_parameters.html#imagedatasetmixin)
is given, then each element in the image is centered about its mean and scaled by its standard deviation, i.e.:

```c
model_input_tensor = (img  - mean(img)) / std(img)
```

In both these cases, the model input data type must be `float32`.

### Model Output ###

The model output should have the shape `1 x <number of classes>`  
where `<number of classes>` should be the number of classes that the model is able to detect.

The datatype should be `uint8`:

- 0: Nothing

- 1: Thumbs up

- 2: Thumbs down

### Model Evaluation ###

```txt
Name: thumbs_up_and_down
Model Type: classification
Overall accuracy: 98.795%
Class accuracies:
- thumbs_up = 99.682%
- thumbs_down = 98.457%
- nothing = 98.446%
Average ROC AUC: 99.472%
Class ROC AUC:
- thumbs_up = 99.996%
- nothing = 99.246%
- thumbs_down = 99.174%
```

![Model Evaluation](images/thumbs_up_and_down-roc.png)
![Model Evaluation](images/thumbs_up_and_down-precision_vs_recall.png)
![Model Evaluation](images/thumbs_up_and_down-tpr.png)
![Model Evaluation](images/thumbs_up_and_down-fpr.png)
![Model Evaluation](images/thumbs_up_and_down-tfp_fpr.png)

## How It Works ##

### Video ###

[![Watch the video](images/video_demo.png)](https://youtu.be/xF_SeU2ZNU4)

### System Overview Diagram ###

![system overview](images/system_overview.png)

### Application Workflows ##

#### Startup and initialization ####

![initialization](images/initialization.png)

#### Application event loop ####

![loop](images/app_loop.png)

#### User Configuration ####

```c
// These are parameters that are optionally embedded
// into the .tflite model file
typedef struct AppSettings
{
  // norm_img = img * rescale
  float samplewise_norm_rescale;
  // norm_img = (img - mean(img)) / std(img)
  bool samplewise_norm_mean_and_std;
  // enable verbose inference logging
  bool verbose_inference_output;
  // Drop all inference results older than this value
  uint32_t average_window_duration_ms;
  // The minimum number of inference results to average
  uint32_t minimum_count;
  // Minimum averaged model output threshold for a class
  // to be considered detected, 0-255; 255 = highest confidence
  uint8_t detection_threshold;
  // The number of samples that are different than the last detected sample
  // for a new detection to occur
  uint32_t suppression_count;
  // This the amount of time in milliseconds between inference
  uint32_t inference_time;
} app_settings_t;
```

#### Display ####

![display](images/display.png)

**Note:** score is the probability score of the detected gesture as a uint8 (i.e. 255 = highest confidence that the correct gesture was detected)

#### BLE ####

- **Device Name:** `Hand Signal Recognition`

- **Service:** Hand signal with UUID: `b1448626-7e61-40a5-8ac7-aabae70f3b2a`

Which has one characteristic with the properties: `Read` and `Notify`.
The contents of this characteristic is two bytes with the format:

```txt
<detect_class_id> <score>
```

Where:  

- **detect_class_id** - Is the class ID of the detected gesture (0 - Nothing, 1 - Thumbs up, 2 - Thumbs down)

- **score** - Is the probability score of the detected gesture as a uint8 (i.e. 255 = highest confidence that the correct gesture was detected)

### Testing ###

Upon reset, the application will display the Silicon Labs's logo on the OLED screen for a few seconds. Then you can bring your hand close to the camera and make thumbs up or down. The classification results will be displayed on the OLED screen.

Follow the below steps to test the example with the EFR Connect app:

1. Open the EFR Connect app on your iOS/Android device.

2. Find your device in the Bluetooth Browser, advertising as Air Quality, and tap Connect. Then you need accept the pairing request when connected for the first time.

3. Find the unknown service at the above of the OTA service.

4. Try to read, subcrible the characteristic, and check the value.

    ![efr app](images/efr_app.png)

5. You can launch the Console that is integrated on Simplicity Studio or can use a third-party terminal tool like TeraTerm to receive the data from the virtual COM port. Use the following UART settings: baud rate 115200, 8N1, no flow control. You should expect a similar output to the one below.

    ![logs](images/logs.png)
