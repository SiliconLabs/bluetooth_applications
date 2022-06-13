/***************************************************************************//**
 * @file app_logic.c
 * @brief Application Logic Source File
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 *
 * # EXPERIMENTAL QUALITY
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "sl_bt_api.h"
#include "app_log.h"
#include "gatt_db.h"
#include "app_assert.h"

#include "em_gpio.h"
#include "gpiointerrupt.h"

#include "bma400.h"
#include "sl_i2cspm_instances.h"
#include "sl_bma400_i2c.h"

#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "sl_sleeptimer.h"

#include "app_config.h"
#include "app_logic.h"
#include "app_callbacks.h"

// Led blinking notify interval.
#define MD_INTERVAL_BLINK_NOTIFY  500

// BMA400 device structure.
static struct bma400_dev bma;

// Application runtime parameters.
static md_runtime_data_t md_runtime_data = MD_RUNTIME_DEFAULT_DATASET;

// Application configuration data ram mirror.
md_config_data_t md_config = MD_DEFAULT_CONFIG;

// Implemented BLE characteristics and their features.
md_feature_t md_features[] = MD_FEATURES;


// Local application logic functions
static void md_load_config_from_nvm(void);
static void md_print_config(void);
static void gpio_init(void);
static void check_object_moving(void);
static void handle_detected_moving(void);

// Local accelerometer sensor related functions.
static void acc_sensor_init(void);
static void acc_sensor_enter_lp_mode(void);
static void acc_sensor_enable_auto_wakeup_int(void);
static void acc_sensor_disable_auto_wakeup_int(void);

/***************************************************************************//**
 * Initializes GPIO and register the callback
 ******************************************************************************/
static void gpio_init(void)
{
  GPIOINT_Init();
  GPIO_ExtIntConfig(gpioPortB,
                    BMA400_INT_PIN,
                    BMA400_INT_PIN,
                    true,
                    false,
                    true);
  GPIO_PinModeSet(gpioPortB,
                  BMA400_INT_PIN,
                  gpioModeInputPull,
                  (unsigned int)1);
  GPIOINT_CallbackRegister(BMA400_INT_PIN,
                           app_external_int_auto_wakeup_callback);
}

/***************************************************************************//**
 * Load configuration from NVM
 ******************************************************************************/
static void md_load_config_from_nvm(void)
{
  sl_status_t status;
  size_t data_length = 0;
  uint8_t data[4] = {0};

  app_log("Loading parameters from NVM...\n");

  // Load parameters from NVM, or use the default values
  for (int i = 0; i < MD_BLE_FEATURE_LENGTH; i++) {
    // Try to load a parameter from NVM
    status = sl_bt_nvm_load(
              md_features[i].nvm_key,
              md_features[i].data_length,
              &data_length,
              data);

    // Check if there is valid data with the given key
    if ((status == SL_STATUS_OK)
        && (md_features[i].data_length == data_length)) {
      // Data has a valid length
      // Copy data into the runtime configuration structure
      memcpy(
        md_features[i].data,
        data,
        md_features[i].data_length);

      app_log(
        "> For char ID %d, parameter loaded successfully from NVM.\n",
        md_features[i].char_id);
    } else {
      app_log(
        "> For char ID %d, failed to load parameter.\n",
        md_features[i].char_id);
    }
  }
}

/***************************************************************************//**
 * Print configuration value.
 ******************************************************************************/
static void md_print_config(void)
{
  app_log("wakeup time period = %d (ms)\r\n",md_config.wake_up_time_period);
  app_log("threshold = %d (lsb)\r\n",md_config.movement_threshold);
  app_log("notification time = %d (ms)\r\n",md_config.notification_time);
  app_log("notification break time = %d (ms)\r\n",
          md_config.notification_break_time);
}

/***************************************************************************//**
 * Initializes sensor BMA400
 ******************************************************************************/
static void acc_sensor_init(void)
{
  struct bma400_sensor_conf conf;
  struct bma400_device_conf dev_conf[2];
  struct bma400_int_enable int_en;
  int8_t ret = 0;

  ret = bma400_i2c_init(sl_i2cspm_mikroe, BMA400_I2C_ADDRESS_SDO_HIGH, &bma);
  app_assert_status((sl_status_t)ret);
  ret = bma400_soft_reset(&bma);
  app_assert_status((sl_status_t)ret);

  ret = bma400_init(&bma);
  app_assert_status((sl_status_t)ret);
  app_log("chip id = %d\n\r", bma.chip_id);

  // Put accelerometer sensor in normal mode before configuration.
  ret = bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);
  app_assert_status((sl_status_t)ret);

  // Configure accelerometer data.
  conf.type = BMA400_ACCEL;
  conf.param.accel.odr = BMA400_ODR_100HZ;
  conf.param.accel.range = BMA400_RANGE_2G;
  conf.param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;
  ret = bma400_set_sensor_conf(&conf, 1, &bma);
  app_assert_status((sl_status_t)ret);

  // Configure auto wake-up interrupt.
  dev_conf[0].type = BMA400_AUTOWAKEUP_INT;
  dev_conf[0].param.wakeup.int_chan = BMA400_INT_CHANNEL_1;
  dev_conf[0].param.wakeup.wakeup_axes_en = BMA400_AXIS_XYZ_EN;
  dev_conf[0].param.wakeup.int_wkup_threshold = md_config.movement_threshold;
  dev_conf[0].param.wakeup.wakeup_ref_update = BMA400_UPDATE_ONE_TIME;

  // Configure interrupt pin.
  dev_conf[1].type = BMA400_INT_PIN_CONF;
  dev_conf[1].param.int_conf.int_chan = BMA400_INT_CHANNEL_1;
  dev_conf[1].param.int_conf.pin_conf = BMA400_INT_PUSH_PULL_ACTIVE_1;

  ret = bma400_set_device_conf(dev_conf, 2, &bma);
  app_assert_status((sl_status_t)ret);

  // Enable auto wakeup interrupt.
  int_en.type = BMA400_AUTO_WAKEUP_EN;
  int_en.conf = BMA400_ENABLE;
  ret = bma400_enable_interrupt(&int_en, 1, &bma);
  app_assert_status((sl_status_t)ret);

  // Put sensor in low power mode.
  ret = bma400_set_power_mode(BMA400_MODE_LOW_POWER, &bma);
  app_assert_status((sl_status_t)ret);

  app_log("Init BMA400 done!\r\n");
}

/***************************************************************************//**
 * Check if the object is moving or not
 ******************************************************************************/
static void check_object_moving(void)
{
  sl_status_t status;

  md_runtime_data.sample_counter += 1;
  if (md_runtime_data.sample_counter >= MD_SAMPLES_COUNT_THRESHOLD) {
    if (md_runtime_data.movement_counter >= MD_MOVEMENT_COUNT_THRESHOLD) {
      md_runtime_data.movement_flag = true;
      app_log("Moving is detected\r\n");

      // Disable auto wakeup interrupt until break time is over.
      acc_sensor_disable_auto_wakeup_int();
      GPIO_IntDisable(1 << BMA400_INT_PIN);

      handle_detected_moving();
    } else {
      app_log("Moving is not detected\r\n");
    }

    app_log("Stop checking\r\n");
    status = sl_sleeptimer_stop_timer(
      &md_runtime_data.wake_up_period_timer_handle);
    app_assert_status(status);
    md_runtime_data.sample_counter = 0;
    md_runtime_data.movement_counter = 0;
  }
  acc_sensor_enter_lp_mode();
}

/***************************************************************************//**
 * Handles movement detection
 ******************************************************************************/
static void handle_detected_moving(void)
{
  sl_status_t status;

  status = sl_sleeptimer_start_timer_ms(
    &md_runtime_data.notify_timer_handle,
    md_config.notification_time,
    app_sleep_timer_notify_callback,
    NULL,
    0,
    0);
  if (status != SL_STATUS_OK) {
    app_log("Start notify timer failed\r\n");
  }
  app_assert_status(status);
  status = sl_sleeptimer_start_periodic_timer_ms(
    &md_runtime_data.blink_led_timer_handle,
    MD_INTERVAL_BLINK_NOTIFY,
    app_sleep_timer_blink_led_callback,
    NULL,
    0,
    0);
  if (status != SL_STATUS_OK) {
    app_log("Start blink led timer failed\r\n");
  }                                             
  app_assert_status(status);
}

/***************************************************************************//**
 * Sets the low power mode to the sensor
 ******************************************************************************/
static void acc_sensor_enter_lp_mode(void)
{
  int8_t ret;

  ret = bma400_set_power_mode(BMA400_MODE_LOW_POWER, &bma);
  app_assert_status((sl_status_t)ret);
}

/***************************************************************************//**
 * Enable the auto wake-up to the sensor
 ******************************************************************************/
static void acc_sensor_enable_auto_wakeup_int(void)
{
  struct bma400_int_enable int_en;
  int8_t ret;

  int_en.type = BMA400_AUTO_WAKEUP_EN;
  int_en.conf = BMA400_ENABLE;
  ret = bma400_enable_interrupt(&int_en, 1, &bma);
  app_assert_status((sl_status_t)ret);
}

/***************************************************************************//**
 * Disable the auto wake-up to the sensor
 ******************************************************************************/
static void acc_sensor_disable_auto_wakeup_int(void)
{
  struct bma400_int_enable int_en;
  int8_t ret;

  int_en.type = BMA400_AUTO_WAKEUP_EN;
  int_en.conf = BMA400_DISABLE;
  ret = bma400_enable_interrupt(&int_en, 1, &bma);
  app_assert_status((sl_status_t)ret);
}

/***************************************************************************//**
 * Resets configuration time so that the configuration timeout
 * is not exceeded.
 ******************************************************************************/
void app_logic_reset_last_req_conf_timer(void)
{
  sl_status_t status;

  status = sl_sleeptimer_stop_timer(
    &md_runtime_data.last_req_conf_timer_handle);
  app_assert_status(status);

  status = sl_sleeptimer_start_timer_ms(
    &md_runtime_data.last_req_conf_timer_handle,
    MD_LAST_REQ_TIMEOUT_MS,
    app_sleep_timer_last_req_conf_callback,
    NULL,
    0,
    0);
  app_assert_status(status);
}

/***************************************************************************//**
 * Handles notification timer expired event.
 ******************************************************************************/
void app_logic_handle_notify_timer(void)
{
  sl_status_t status;

  status = sl_sleeptimer_stop_timer(&md_runtime_data.blink_led_timer_handle);
  app_assert_status(status);

  sl_simple_led_turn_off(sl_led_led0.context);

  status = sl_sleeptimer_start_timer_ms(
    &md_runtime_data.notify_break_timer_handle,
    md_config.notification_break_time,
    app_sleep_timer_notify_break_callback,
    NULL,
    0,
    0);
  app_assert_status(status);
}

/***************************************************************************//**
 * Handles the expired event of notification breaking timer.
 ******************************************************************************/
void app_logic_handle_notify_break_timer(void)
{
  app_log("Break time is over\r\n");

  // Enable auto wakeup interrupt because break time is over.
  acc_sensor_enable_auto_wakeup_int();
  GPIO_IntEnable(1 << BMA400_INT_PIN);

  acc_sensor_enter_lp_mode();
  md_runtime_data.movement_flag = false;
}

/***************************************************************************//**
 * Initializes the external drivers, configures the
 * accelerometer sensor and set the application parameters.
 ******************************************************************************/
void app_logic_init(void)
{
  md_load_config_from_nvm();
  md_print_config();
}

/***************************************************************************//**
 * Handle system boot event, check button 0 is pressed or
 * not and determines device enter configure mode or normal mode.
 ******************************************************************************/
movement_detection_mode_t app_logic_handle_system_boot_evt(void)
{
  sl_status_t status;

  // If BTN0 is held down then choose configuration mode.
  if (sl_simple_button_get_state(&sl_button_btn0)
      == SL_SIMPLE_BUTTON_PRESSED) {

    // Device enter configuration mode, turn on led0 to indicate.
    sl_simple_led_turn_on(sl_led_led0.context);

    // Start last request configuration timer.
    status = sl_sleeptimer_start_timer_ms(
      &md_runtime_data.last_req_conf_timer_handle,
      MD_LAST_REQ_TIMEOUT_MS,
      app_sleep_timer_last_req_conf_callback,
      NULL,
      0,
      0);
    app_assert_status(status);

    return MD_CONFIGURED_MODE;
  } else {
    // Device enters normal mode.
    acc_sensor_init();
    gpio_init();
    return MD_NORMAL_MODE;
  }
}

/***************************************************************************//**
 * Handles the wake-up event of accelerometer sensor.
 ******************************************************************************/
void app_logic_handle_acc_wakeup_evt(void)
{
  sl_status_t ret;
  uint16_t int_status = 0;
  int8_t bma_ret;

  bma_ret = bma400_get_interrupt_status(&int_status, &bma);
  app_assert_status((sl_status_t)bma_ret);

  // Ignore if interrupt source is not sensor auto wakeup.
  if ((int_status & BMA400_ASSERTED_WAKEUP_INT) == 0) {
    return;
  }

  if (md_runtime_data.movement_counter == 0) {
    app_log("Start checking\r\n");
    // Start wake-up time period timer.
    ret = sl_sleeptimer_start_periodic_timer_ms(
      &md_runtime_data.wake_up_period_timer_handle,
      md_config.wake_up_time_period,
      app_sleep_timer_wakeup_period_callback,
      NULL,
      0,
      0);
    app_assert_status(ret);

    check_object_moving();
  } else {
    acc_sensor_enter_lp_mode();
  }
  md_runtime_data.movement_counter += 1;

  // Disable auto wakeup interrupt.
  acc_sensor_disable_auto_wakeup_int();
  GPIO_IntDisable(1 << BMA400_INT_PIN);
}

/***************************************************************************//**
 * Handles the wake-up time period event.
 ******************************************************************************/
void app_logic_handle_wakeup_time_period_evt(void)
{
  // Enable auto wakeup interrupt.
  acc_sensor_enable_auto_wakeup_int();
  GPIO_IntEnable(1 << BMA400_INT_PIN);

  check_object_moving();
}
