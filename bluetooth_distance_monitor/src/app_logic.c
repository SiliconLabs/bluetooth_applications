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
#include "sl_sleeptimer.h"
#include "sl_pwm.h"
#include "sl_pwm_instances.h"
#include "app_log.h"
#include "gatt_db.h"

#include "glib.h"
#include "buzz2.h"
#include "vl53l1x.h"
#include "vl53l1x_config.h"

#include "app_config.h"
#include "app_logic.h"
#include "app_events.h"
#include "app_callbacks.h"


const uint8_t silicon_labs_logo[] = { 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0x81, 0x81, 0x81, 0x81, 0x01,
    0x41, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0x81, 0x81, 0x81, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81,
    0x81, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xE0, 0xF0, 0xF0, 0xF8, 0xF8,
    0x3C, 0x1C, 0x0C, 0x06, 0x06, 0x02, 0x02, 0x03, 0x01, 0x01, 0x61, 0xF0,
    0xF8, 0xFC, 0xFC, 0x7E, 0x7E, 0x7F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x7E, 0x76, 0x60, 0xE0, 0xE0,
    0xE0, 0xF0, 0xF0, 0xF8, 0xFD, 0xFF, 0xFF, 0xFE, 0xFC, 0x78, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x7F, 0x3F, 0x1F, 0x1F, 0x0E, 0x0C, 0x1C, 0x1C, 0xFC, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9,
    0xFD, 0xFF, 0x7F, 0x7E, 0x3E, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
    0x40, 0x40, 0x60, 0x30, 0x39, 0x1F, 0x1F, 0x0F, 0x0F, 0x07, 0x03, 0x01,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x03, 0x03, 0x03, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xCC,
    0x12, 0x12, 0x12, 0xE4, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00,
    0xFE, 0x00, 0x78, 0x84, 0x02, 0x02, 0x02, 0xCC, 0x00, 0x78, 0x84, 0x02,
    0x02, 0x84, 0x78, 0x00, 0xFE, 0x04, 0x18, 0x60, 0x80, 0xFE, 0x00, 0x00,
    0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x80, 0x7C, 0x42, 0x7C, 0x80, 0x00,
    0xFE, 0x12, 0x12, 0x12, 0xEC, 0x00, 0xCC, 0x12, 0x12, 0x12, 0xE4, 0x00,
    0xFF, 0xFF, 0x80, 0x80, 0x81, 0x81, 0x81, 0x80, 0x80, 0x81, 0x80, 0x81,
    0x81, 0x81, 0x81, 0x80, 0x81, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x80,
    0x80, 0x80, 0x80, 0x81, 0x81, 0x80, 0x80, 0x80, 0x81, 0x80, 0x80, 0x80,
    0x80, 0x81, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x81, 0x80, 0x81, 0x80,
    0x80, 0x80, 0x81, 0x80, 0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x81,
    0x81, 0x81, 0x80, 0x80, 0xFF };

static buzz2_t buzzer;
static glib_context_t glib_context;

// Application runtime parameters
static distance_monitor_runtime_data_t dm_runtime_data =
DISTANCE_MONITOR_RUNTIME_DEFAULT_DATASET;

// Application configuration data ram mirror
distance_monitor_config_data_t distance_monitor_config =
DISTANCE_MONITOR_DEFAULT_CONFIG;

// Implemented BLE characteristics and their features
distance_monitor_feature_t distance_monitor_features[] =
DISTANCE_MONITOR_FEATURES;

// Local application logic functions
static void _distance_monitor_setup_main_timer(void);
static void _distance_monitor_load_default_status_flags(void);
static void _distance_monitor_load_config_from_nvm(void);

// Local notification control functions
static void _buzzer_init(void);
static void _buzzer_activate(void);
static void _buzzer_deactivate(void);
static void _notification_change_status(void);

// Local distance sensor related functions
static void _distance_sensor_init(void);
static void _distance_sensor_process_result(uint16_t distance);
static void _distance_sensor_check_thresholds(void);

// Local display manipulation functions
static void _display_init(void);
static void _display_draw_silabs_logo(void);
static void _display_draw_main_screen(void);
static void _display_draw_message(char *first_line, char *second_line);
static uint8_t _display_get_center_offset(char *line);
static void _display_set_paramline_next_text(uint8_t *text_buffer);
static void _display_update_distance_page(void);
static void _display_clear_distance_page(void);

/**************************************************************************//**
 * Application Main Logic
 *****************************************************************************/
void app_logic_init(void)
{
  _distance_monitor_load_config_from_nvm();
  _display_init();
  _buzzer_init();
  _distance_sensor_init();
}

void app_logic_main_function(void)
{
  sl_status_t vl53_status = SL_STATUS_OK;
  uint8_t is_data_ready = 0;
  uint16_t distance;

  // Check measurement data status
  vl53_status = vl53l1x_check_for_data_ready(VL53L1X_ADDR, &is_data_ready);

  if (0 != is_data_ready) {
    // Measurement data is ready to read from the sensor

    vl53_status = vl53l1x_get_distance(VL53L1X_ADDR, &distance);

    // Print result
    //app_log(" > Distance: %4u mm\n", distance);

    // Clear sensor's interrupt status
    vl53_status = vl53l1x_clear_interrupt(VL53L1X_ADDR);

    // Process result
    _distance_sensor_process_result(distance);

    // Check thresholds
    if (dm_runtime_data.is_sensor_ready) {
      _distance_sensor_check_thresholds();
    }
  }

  if (SL_STATUS_OK != vl53_status) {
    app_log("> VL53L1X sensor operation has been failed "
        "during the periodic measurement.\n");

    // Deactivate notification and clear status flags
    _distance_monitor_load_default_status_flags();

    // Display error message
    _display_draw_message("SEN.COM.", "FAIL");
    return;
  }
}

static void _distance_monitor_load_config_from_nvm(void)
{
  uint16_t data_length = 0;
  uint8_t data[4];

  app_log("Loading parameters from NVM...\n");

  // Load parameters from NVM, or use the default values
  for (int i = 0; i < DISTANCE_MONITOR_BLE_FEATURE_LENGTH; i++) {

    // Try to load a parameter from NVM
    sl_bt_nvm_load(distance_monitor_features[i].nvm_key,
        distance_monitor_features[i].data_length, (size_t*) &data_length, data);

    // Check if there is valid data with the given key
    if (distance_monitor_features[i].data_length == data_length) {
      // Data has a valid length
      // Copy data into the runtime configuration structure
      memcpy(distance_monitor_features[i].data, data,
          distance_monitor_features[i].data_length);

      app_log("> For char ID %d, parameter loaded from NVM.\n",
          distance_monitor_features[i].char_id);
    }
  }
}

static void _distance_monitor_load_default_status_flags(void)
{
  dm_runtime_data.calculated_average_distance = 0;
  dm_runtime_data.is_distance_out_of_range = false;
  dm_runtime_data.is_sensor_ready = false;

  if (dm_runtime_data.is_notification_active) {
    _buzzer_deactivate();
  }

  // Stop main periodic timer
  sl_sleeptimer_stop_timer(&dm_runtime_data.main_timer_handle);
}

static void _distance_monitor_setup_main_timer(void)
{
  // Setup a timer for display animations
  sl_sleeptimer_start_periodic_timer_ms(&dm_runtime_data.main_timer_handle,
  DISTANCE_MONITOR_MAIN_TIMER_PERIOD, app_sleep_timer_main_callback, NULL, 0,
      0);
}

static void _distance_sensor_process_result(uint16_t distance)
{
  static uint16_t result_buffer[DISTANCE_MONITOR_SAMPLE_BUFFER_LENGTH] = { 0 };
  static uint16_t *latest_result = result_buffer;
  static uint8_t samples_count = 0;
  static uint32_t average_distance = 0;
  uint16_t min;
  uint16_t max;
  uint8_t samples_divider = 0;

  // Set min. and max. values to the first element of the result's buffer
  min = max = result_buffer[0];

  // Append distance value to the buffer
  *latest_result = distance;

  // Advance sample counter
  if (samples_count < DISTANCE_MONITOR_SAMPLE_BUFFER_LENGTH) {
    samples_count++;
  } else {
    // Set sensor state to ready
    if (!dm_runtime_data.is_sensor_ready) {
      dm_runtime_data.is_sensor_ready = true;
    }
  }

  // Calculate average distance
  if (samples_count >= 2) {

    for (uint8_t i = 0; i < samples_count; i++) {

      // Average distance
      average_distance += result_buffer[i];

      // Set min. and max. values
      if (samples_count >= 4) {
        if (result_buffer[i] < min) {
          min = result_buffer[i];
        }
        if (result_buffer[i] > max) {
          max = result_buffer[i];
        }
      }
    }
    samples_divider = samples_count;

    // Exclude  min. and max. values
    if (samples_count >= 4) {
      //Subtract min. and max. values
      average_distance -= (min + max);
      samples_divider -= 2;
    }

    // Calculate average
    average_distance /= samples_divider;
    dm_runtime_data.calculated_average_distance = (uint16_t) average_distance;

  } else {
    // There is no enough sample so display the latest value
    dm_runtime_data.calculated_average_distance = *latest_result;
  }

  // Arrange last_result pointer
  if (latest_result
      == (uint16_t*) &result_buffer[DISTANCE_MONITOR_SAMPLE_BUFFER_LENGTH - 1]) {
    latest_result = result_buffer;
  } else {
    latest_result++;
  }
}

static void _distance_sensor_check_thresholds(void)
{
  bool is_threshold_in_range = false;
  uint16_t upper_range_limit;

  // Configure upper limit value in accordance with the configured range mode
  if (distance_monitor_config.range_mode == DISTANCE_MONITOR_RANGE_MODE_SHORT) {
    upper_range_limit = DISTANCE_MONITOR_RANGE_MODE_SHORT_UPPER_LIMIT;
  } else {
    upper_range_limit = DISTANCE_MONITOR_RANGE_MODE_LONG_UPPER_LIMIT;
  }

  // Check range
  if (dm_runtime_data.calculated_average_distance
      >= DISTANCE_MONITOR_RANGE_LOWER_LIMIT
      && dm_runtime_data.calculated_average_distance <= upper_range_limit) {

    // Calculated average value is in the valid range
    dm_runtime_data.is_distance_out_of_range = false;

    // Check thresholds

    switch (distance_monitor_config.threshold_mode) {
    case DISTANCE_MONITOR_THRESHOLD_BELOW:
      if (dm_runtime_data.calculated_average_distance
          < distance_monitor_config.threshold_value_lower) {
        is_threshold_in_range = true;
      }
      break;
    case DISTANCE_MONITOR_THRESHOLD_ABOVE:
      if (dm_runtime_data.calculated_average_distance
          > distance_monitor_config.threshold_value_upper) {
        is_threshold_in_range = true;
      }
      break;
    case DISTANCE_MONITOR_THRESHOLD_IN:
      if (dm_runtime_data.calculated_average_distance
          > distance_monitor_config.threshold_value_lower
          && dm_runtime_data.calculated_average_distance
              < distance_monitor_config.threshold_value_upper) {
        is_threshold_in_range = true;
      }
      break;
    case DISTANCE_MONITOR_THRESHOLD_OUT:
      if (dm_runtime_data.calculated_average_distance
          < distance_monitor_config.threshold_value_lower
          || dm_runtime_data.calculated_average_distance
              > distance_monitor_config.threshold_value_upper) {
        is_threshold_in_range = true;
      }
      break;
    default:
      break;
    }

    // Check if average value in range
    if (is_threshold_in_range) {

      if (DISTANCE_MONITOR_NOTIFICATION_ENABLED
          == distance_monitor_config.notification_status) {

        // Activate buzzer if it is not activated
        if (!dm_runtime_data.is_notification_active) {
          _buzzer_activate();
        }
      }
    } else {
      // Outside of range, deactivate buzzer if it is active
      if (dm_runtime_data.is_notification_active) {
        _buzzer_deactivate();
      }
    }

  } else {
    // Measured average distance is out of range
    dm_runtime_data.is_distance_out_of_range = true;

    if (dm_runtime_data.is_notification_active) {
      _buzzer_deactivate();
    }
  }
}

/**************************************************************************//**
 * Distance Sensor Services
 *****************************************************************************/
static void _distance_sensor_init(void)
{
  uint8_t sensor_state = 0;
  sl_status_t vl53_status = SL_STATUS_OK;

  app_log("Distance sensor initialization...\n");
  app_log("> Booting VL53L1X...\n");

  // Waiting for device to boot up...
  while (0 == sensor_state) {
    // Read sensor's state (0 = boot state, 1 = device is booted )
    vl53_status = vl53l1x_get_boot_state(VL53L1X_ADDR, &sensor_state);

    if (SL_STATUS_OK != vl53_status) {
      break;
    }

    // Wait for 2 ms
    sl_sleeptimer_delay_millisecond(2);
  }

  if (SL_STATUS_OK == vl53_status) {
    app_log("> Platform I2C communication is OK.\n");
  } else {
    app_log("> Platform I2C communication test has been failed.\n"
        "  Please check the I2C bus connection and "
        "the I2C (I2CSPM) configuration.\n");

    // Display error message
    _display_draw_message("SEN.COM.", "FAIL");
    return;
  }

  app_log("> VL53L1X booted.\n");
  app_log("> Initialize VL53L1X...\n");

  // Initialize the sensor with the default settings
  vl53_status = vl53l1x_init(VL53L1X_ADDR);

  // Optional sensor configuration example function calls, see API documentation for options
  {
    vl53_status = vl53l1x_set_distance_mode(VL53L1X_ADDR,
        distance_monitor_config.range_mode); // Select distance mode

    vl53_status = vl53l1x_set_timing_budget_in_ms(VL53L1X_ADDR, 100); // in ms possible values [20, 50, 100, 200, 500]
    vl53_status = vl53l1x_set_inter_measurement_in_ms(VL53L1X_ADDR, 100); // in ms, IM must be > = TB
    vl53_status = vl53l1x_set_roi_xy(VL53L1X_ADDR, 16, 16); // min. ROI is 4,4
  }

  // Check return codes of the optional configuration function calls
  if (SL_STATUS_OK == vl53_status) {
    app_log("> VL53L1X sensor initialization and configuration are done.\n");
  } else {
    app_log(
        "> VL53L1X sensor initialization and configuration has been failed.\n");

    // Display error message
    _display_draw_message("SEN.CONF.", "ERROR");
    return;
  }

  // Start ranging
  vl53_status = vl53l1x_start_ranging(VL53L1X_ADDR);

  // Check ranging status
  if (SL_STATUS_OK == vl53_status) {
    app_log("> VL53L1X ranging has been started ...\n");

    // Setup periodic timer for the main loop
    _distance_monitor_setup_main_timer();

  } else {
    app_log("> VL53L1X starting ranging has been failed.\n");

    // Display error message
    _display_draw_message("SEN.RANG.", "ERROR");
    return;
  }
}

void app_logic_configure_distance_sensor_range_mode(void)
{
  if (SL_STATUS_OK
      != vl53l1x_set_distance_mode(VL53L1X_ADDR,
          distance_monitor_config.range_mode)) {
    // Deactivate notification and clear status flags
    _distance_monitor_load_default_status_flags();

    // Display error message
    _display_draw_message("SEN.COM.", "FAIL");
  }
}

/**************************************************************************//**
 * Notification Services
 *****************************************************************************/
void app_logic_configure_notification_status(bool toggle_status)
{
  // For the button logic
  if (toggle_status) {
    _notification_change_status();
  }

  if (DISTANCE_MONITOR_NOTIFICATION_DISABLED
      == distance_monitor_config.notification_status) {
    if (dm_runtime_data.is_notification_active) {
      _buzzer_deactivate();
    }
  }
}

static void _buzzer_init(void)
{
  // Legacy buzzer driver, this should be replaced to the revised version
  buzzer.pwm = sl_pwm_mikroe;
  buzzer.config.frequency = BUZZ2_NOTE_F4;
  app_logic_configure_buzzer_volume();
}

void app_logic_configure_buzzer_volume(void)
{
  buzz2_set_duty_cycle(&buzzer, distance_monitor_config.buzzer_volume);
}

static void _notification_change_status(void)
{
  if (DISTANCE_MONITOR_NOTIFICATION_ENABLED
      == distance_monitor_config.notification_status) {
    distance_monitor_config.notification_status =
    DISTANCE_MONITOR_NOTIFICATION_DISABLED;

  } else {
    distance_monitor_config.notification_status =
    DISTANCE_MONITOR_NOTIFICATION_ENABLED;
  }
}

static void _buzzer_activate(void)
{
  buzz2_pwm_start(&buzzer);
  dm_runtime_data.is_notification_active = true;
}

static void _buzzer_deactivate(void)
{
  buzz2_pwm_stop(&buzzer);
  dm_runtime_data.is_notification_active = false;
}

/**************************************************************************//**
 * OLED Display Services
 *****************************************************************************/
static void _display_init(void)
{
  glib_context.backgroundColor = Black;
  glib_context.foregroundColor = White;
  glib_context.font = glib_font_6x8;

  dm_runtime_data.display.max_char_per_line = (uint8_t) (64
      / (glib_font_6x8.width + glib_font_6x8.spacing));

  glib_init();
  _display_draw_silabs_logo();
  sl_sleeptimer_delay_millisecond(1000);

  _display_draw_main_screen();

  // Setup a timer for display animations
  sl_sleeptimer_start_periodic_timer_ms(&dm_runtime_data.display.timer_handle,
      200, app_sleep_timer_display_callback, NULL, 2, 0);

}

void app_logic_draw_parameter_line(void)
{
  // Prepare text
  switch (distance_monitor_config.threshold_mode) {
  case DISTANCE_MONITOR_THRESHOLD_BELOW:
    sprintf((char*) dm_runtime_data.display.param_text, "BELOW %dmm |",
        distance_monitor_config.threshold_value_lower);
    break;
  case DISTANCE_MONITOR_THRESHOLD_ABOVE:
    sprintf((char*) dm_runtime_data.display.param_text, "ABOVE %dmm |",
        distance_monitor_config.threshold_value_upper);
    break;
  case DISTANCE_MONITOR_THRESHOLD_IN:
    sprintf((char*) dm_runtime_data.display.param_text, "IN %d-%dmm RANGE |",
        distance_monitor_config.threshold_value_lower,
        distance_monitor_config.threshold_value_upper);
    break;
  case DISTANCE_MONITOR_THRESHOLD_OUT:
    sprintf((char*) dm_runtime_data.display.param_text,
        "OUT OF %d-%dmm RANGE |", distance_monitor_config.threshold_value_lower,
        distance_monitor_config.threshold_value_upper);
    break;
  default:
    break;
  }

  if (strlen((char*) dm_runtime_data.display.param_text)
      > dm_runtime_data.display.max_char_per_line) { // Max. char per line
    if (!dm_runtime_data.display.is_animation_active) {

      // Set animation parameters
      dm_runtime_data.display.first_letter = dm_runtime_data.display.param_text;
      dm_runtime_data.display.text_length = strlen(
          (char*) dm_runtime_data.display.param_text);

      // Start animation
      dm_runtime_data.display.is_animation_active = true;
    }

  } else {
    if (dm_runtime_data.display.is_animation_active) {
      // Stop animation
      dm_runtime_data.display.is_animation_active = false;
    }
  }
  // Display parameter line
  glib_draw_string(&glib_context, (char*) dm_runtime_data.display.param_text, 1,
      40);
  glib_update_display();
}

void app_logic_on_update_display(void)
{
  static bool flip_range = false;
  uint8_t param_text_buffer[dm_runtime_data.display.max_char_per_line + 1];

  // Update display
  if (dm_runtime_data.is_sensor_ready) {
    _display_update_distance_page();

  }

  // If notification is active, then blink RANGE label
  if (dm_runtime_data.is_notification_active) {

    // Blink RANGE label if notification is active
    if (flip_range) {
      glib_draw_string(&glib_context, "  RANGE  ", 1, 40);
      _buzzer_activate();
    } else {
      glib_draw_string(&glib_context, "         ", 0, 40);
      _buzzer_deactivate();
    }
    flip_range = !flip_range;

  } else {

    // Scroll parameter line is it is too long to display
    if (dm_runtime_data.display.is_animation_active) {

      _display_set_paramline_next_text(param_text_buffer);

      glib_draw_string(&glib_context, (char*) param_text_buffer, 1, 40);

    }
  }

  // Update display
  glib_update_display();
}

static void _display_draw_main_screen(void)
{
  glib_clear(&glib_context);
  glib_draw_string(&glib_context, "DISTANCE", 5, 0);
  glib_draw_line(&glib_context, 0, 9, 63, 9);
  glib_draw_line(&glib_context, 0, 37, 63, 37);

  _display_draw_message("SENSOR", "INIT");

  app_logic_draw_parameter_line();
}

static void _display_update_distance_page(void)
{
  uint8_t text_buffer[32];

  if (dm_runtime_data.is_distance_out_of_range) {
    _display_draw_message("OUT OF", "RANGE");

  } else {
    sprintf((char*) text_buffer, "%4d",
        dm_runtime_data.calculated_average_distance);

    // Clear distance space area
    _display_clear_distance_page();

    // Switch to larger font
    glib_context.font = glib_font_11x18;
    glib_draw_string(&glib_context, (char*) text_buffer, 0, 16);

    // Switch back to default font
    glib_context.font = glib_font_6x8;
    glib_draw_string(&glib_context, "mm", 52, 20);
  }
}

static void _display_draw_message(char *first_line, char *second_line)
{
  // Clear distance space area
  _display_clear_distance_page();

  glib_draw_string(&glib_context, first_line,
      _display_get_center_offset(first_line), 15);
  glib_draw_string(&glib_context, second_line,
      _display_get_center_offset(second_line), 25);
  glib_update_display();
}

static void _display_clear_distance_page(void)
{
  glib_context.foregroundColor = Black;
  for (int i = 10; i < 37; i++) {
    glib_draw_line(&glib_context, 0, i, 63, i);
  }
  glib_context.foregroundColor = White;
}

// Shift characters for scrolling long texts horizontally.
// For short texts, consider to use the SSD1306 hardware based scrolling feature.
// This OLED display can show only 9 characters per line (6x8 default font)
// e.g.:
// full text: IN 70-90mm RANGE |
// Shifts letters to show scrolling message like this:
// ]IN 70-90m[
// ]N 70-90mm[
// ] 70-90mm [
// ]70-90mm R[
// ]0-90mm RA[
// ]-90mm RAN[
// ]90mm RANG[
// ]0mm RANGE[
// ]mm RANGE [
// ]m RANGE |[
// ] RANGE | [
// ]RANGE | I[
// and so on...

static void _display_set_paramline_next_text(uint8_t *text_buffer)
{
  uint8_t txt_length = strlen((char*) dm_runtime_data.display.first_letter);
  uint8_t txt_remainder_length = 0;

  if (txt_length > dm_runtime_data.display.max_char_per_line) {
    txt_length = dm_runtime_data.display.max_char_per_line;
  } else {
    txt_remainder_length = dm_runtime_data.display.max_char_per_line
        - txt_length;
  }

  // Copy letters starting from first_letter pointer
  memcpy(text_buffer, dm_runtime_data.display.first_letter, txt_length);

  // Copy remaining letters if any
  if (0 != txt_remainder_length) {
    // Append a whitespace between the last and first characters
    text_buffer[txt_length] = ' ';
    memcpy(&text_buffer[txt_length + 1], dm_runtime_data.display.param_text,
        txt_remainder_length);
  }

  // Advance first letter pointer
  dm_runtime_data.display.first_letter++;

  // If it points to the tailing char then
  if (*dm_runtime_data.display.first_letter == '\0') {
    dm_runtime_data.display.first_letter = dm_runtime_data.display.param_text;
  }
  text_buffer[dm_runtime_data.display.max_char_per_line] = '\0';
}

static void _display_draw_silabs_logo(void)
{
  glib_draw_bmp(NULL, (const uint8_t*) silicon_labs_logo);
  glib_enable_display(true);
}

static uint8_t _display_get_center_offset(char *line)
{
  uint8_t line_length = strlen(line);
  uint8_t char_pixel = (glib_font_6x8.width + glib_font_6x8.spacing);

  if (line_length < dm_runtime_data.display.max_char_per_line) {
    return (((((dm_runtime_data.display.max_char_per_line + 1) * char_pixel) - 1)
        - ((line_length * char_pixel) + 1)) / 2);
  } else {
    return 1;
  }
}
