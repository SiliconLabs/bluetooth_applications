/***************************************************************************//**
 * @file air_quality_app.c
 * @brief Air Quality demo using CCS811 sensor
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "sl_ccs811.h"
#include "sl_simple_button_instances.h"
#include "sl_i2cspm_instances.h"
#include "sl_pwm_instances.h"
#include "sl_sleeptimer.h"
#include "gatt_db.h"
#include "app_assert.h"
#include "app_log.h"

#include "micro_oled_ssd1306.h"
#include "mikroe_cmt_8540s_smt.h"
#include "buzz2.h"

#include "glib.h"
#include "nvm3_user.h"
#include "air_quality_app.h"

#define DATA_BUFFER_SIZE  5

// Status of air quality index
const char air_quality_status_text[][12] = {
  "           ",
  " EXCELLENT ",
  "   FINE    ",
  " MODERATE  ",
  "   POOR    ",
  " VERY POOR ",
  "  SEVERE   "
};

// Buffer to store measurement data.
static uint16_t co2_buffer[DATA_BUFFER_SIZE];
static uint16_t tvoc_buffer[DATA_BUFFER_SIZE];
static uint32_t samples_counter;

// The instance for OLED LCD
static glib_context_t glib_context;

// CO2 result from the CCS811.
static uint16_t co2;

// tVOC result from the CCS811.
static uint16_t tvoc;

// Application data
static air_quality_data_t air_quality_data;

// Buzzer state.
static volatile bool is_buzzer_active = false;

// Periodic timer handle.
static sl_sleeptimer_timer_handle_t air_quality_monitor_periodic_timer;
// Periodic timer callback.
static void air_quality_monitor_callback(sl_sleeptimer_timer_handle_t *handle,
                                         void *data);

static void oled_app_init(void);
static air_quality_status_t air_quality_co2_status(uint16_t data);
static air_quality_status_t air_quality_tvoc_status(uint16_t data);

static void buzzer_activate(void);
static void buzzer_deactivate(void);

static sl_status_t nvm3_load_configuration(void);
static void air_quality_monitor_event_handler(void);
static void air_quality_monitor_button_event_handler(void);
static void air_quality_update_display(void);
static void air_quality_data_process(void);
static uint16_t air_quality_moving_average(uint16_t *data);

static const unsigned char silicon_labs_logo_64x23[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x80, 0xff, 0x1f, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xf0,
  0xff, 0x7f, 0x00, 0x3e, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x7f, 0x00, 0x7f,
  0x00, 0x00, 0x00, 0xff, 0xff, 0x3f, 0x80, 0x7f, 0x00, 0x06, 0x80, 0xff,
  0xff, 0xff, 0xe0, 0xff, 0x80, 0x03, 0xc0, 0xff, 0x01, 0xf8, 0xff, 0x7f,
  0xe0, 0x01, 0xc0, 0x03, 0x00, 0x00, 0xfe, 0x7f, 0xf0, 0x01, 0x00, 0x01,
  0x00, 0x00, 0xf8, 0x3f, 0xf8, 0x03, 0x00, 0xf0, 0x07, 0x00, 0xe0, 0x3f,
  0xfc, 0x03, 0x00, 0x00, 0x20, 0x00, 0xe0, 0x1f, 0xfe, 0x0f, 0x00, 0x00,
  0xc0, 0x00, 0xe0, 0x07, 0xfe, 0xff, 0x00, 0x00, 0xfc, 0x00, 0xe0, 0x03,
  0xff, 0xc3, 0xff, 0xff, 0xff, 0x00, 0xf0, 0x00, 0x7e, 0x00, 0xfe, 0xff,
  0x7f, 0x00, 0x38, 0x00, 0x3e, 0x00, 0xff, 0xff, 0x3f, 0x00, 0x0c, 0x00,
  0x1c, 0x80, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x18, 0x00, 0xff, 0xff,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

/***************************************************************************//**
 * Initialize the AIR QUALITY application.
 ******************************************************************************/
sl_status_t air_quality_app_init(void)
{
  sl_status_t status;
  Ecode_t err;

  // Initialize the oled
  oled_app_init();

  // Load configuration from NVM
  err = nvm3_load_configuration();
  if (err != ECODE_NVM3_OK) {
    return SL_STATUS_FAIL;
  }

  // Initialize the buzzer
  sl_status_t ret_code = mikroe_cmt_8540s_smt_init(&sl_pwm_mikroe);
  app_assert_status(ret_code);

  mikroe_cmt_8540s_smt_set_duty_cycle(air_quality_data.buzzer_data / 10.0);
  app_log(">> Buzzer is initialized\r\n");
  app_log(">> Buzzer set volume = %d\r\n", air_quality_data.buzzer_data);

  // Initialize sensor and set measure mode
  status = sl_ccs811_init(sl_i2cspm_sensor_gas);

  if (status != SL_STATUS_OK) {
    return status;
  }
  status = sl_ccs811_set_measure_mode(sl_i2cspm_sensor_gas,
                                      CCS811_MEASURE_MODE_DRIVE_MODE_1SEC);
  if (status != SL_STATUS_OK) {
    return status;
  }

  app_log("Initialize gas sensor and set measure mode\r\n");

  // Start timer used for periodic measurement.
  app_log_info("Starting timer used for periodic measurement.\r\n");
  status = sl_sleeptimer_start_periodic_timer_ms(
    &air_quality_monitor_periodic_timer,
    air_quality_data.measurement_period_data * 1000,
    air_quality_monitor_callback,
    NULL,
    0,
    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  return status;
}

/***************************************************************************//**
 * Initialize and Load configuration from NVM
 ******************************************************************************/
static Ecode_t nvm3_load_configuration(void)
{
  Ecode_t err;

  // Load configuration from NVM
  nvm3_user_init();

  app_log_info("Loading parameters from NVM...\r\n");

  err = nvm3_user_get_notification_active(&air_quality_data.notification_data);
  if (err != ECODE_NVM3_OK) {
    return err;
  }
  app_log_info("Loaded from NVM. Notification status = %d\r\n",
               air_quality_data.notification_data);

  err = nvm3_user_get_update_period(&air_quality_data.measurement_period_data);
  if (err != ECODE_NVM3_OK) {
    return err;
  }
  app_log_info("Loaded from NVM. "
               "Air quality monitor sleep timer period = %ds\r\n",
               air_quality_data.measurement_period_data);

  err = nvm3_user_get_buzzer_volume(&air_quality_data.buzzer_data);
  if (err != ECODE_NVM3_OK) {
    return err;
  }
  app_log_info("Loaded from NVM. Buzzer volume = %d\r\n",
               air_quality_data.buzzer_data);

  err = nvm3_user_get_threshold_co2(&air_quality_data.threshold_co2_ppm);
  if (err != ECODE_NVM3_OK) {
    return err;
  }
  app_log_info("Loaded from NVM. "
               "Notification threshold for CO2 level = %dppm\r\n",
               air_quality_data.threshold_co2_ppm);

  err = nvm3_user_get_threshold_tvoc(&air_quality_data.threshold_tvoc_ppb);
  if (err != ECODE_NVM3_OK) {
    return err;
  }
  app_log_info("Loaded from NVM. "
               "Notification threshold for tVOC level = %dppb\r\n",
               air_quality_data.threshold_tvoc_ppb);

  return ECODE_NVM3_OK;
}

/***************************************************************************//**
 * Process Bluetooth external events.
 ******************************************************************************/
void air_quality_process_event(uint32_t event_flags)
{
  if (event_flags & AIR_QUALITY_MONITOR_EVENT) {
    air_quality_monitor_event_handler();
  }

  if (event_flags & AIR_QUALITY_MONITOR_BUTTON_EVENT) {
    air_quality_monitor_button_event_handler();
  }
}

/***************************************************************************//**
 * Air quality monitor event handler retrieve and
 * process the measured air quality data.
 ******************************************************************************/
static void air_quality_monitor_event_handler(void)
{
  uint16_t _eco2;
  uint16_t _tvoc;

  if (sl_ccs811_is_data_available(sl_i2cspm_sensor_gas)) {
    uint8_t index = samples_counter % DATA_BUFFER_SIZE;
    // Get measurement data from the CCS811
    sl_ccs811_get_measurement(sl_i2cspm_sensor_gas, &_eco2, &_tvoc);

    // Store measurement data.
    // appends a new value to the measurement data, removes the oldest one.
    co2_buffer[index] = _eco2;
    tvoc_buffer[index] = _tvoc;
    samples_counter++;

    // Run limit check and alarm logic
    air_quality_data_process();

    // Update display data
    air_quality_update_display();
  }
}

/***************************************************************************//**
 * This event handler checks the BTN0's status. When the button is released,
 * it checks the notification feature status.
 ******************************************************************************/
static void air_quality_monitor_button_event_handler(void)
{
  app_log_info("> BTN0 Button released. Change notification status.\r\n");
  // For the button logic, enable notification if it is disabled
  if (0 == air_quality_data.notification_data) {
    air_quality_data.notification_data = 1;
    // Disable notification if it is enabled
  } else {
    air_quality_data.notification_data = 0;
    // Deactivate buzzer if it is active
    if (true == is_buzzer_active) {
      buzzer_deactivate();
    }
  }
}

/***************************************************************************//**
 * Simple Button
 * Button state changed callback
 * @param[in] handle
 *    Button event handle
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  // Button released.
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      sl_bt_external_signal(AIR_QUALITY_MONITOR_BUTTON_EVENT);
    }
  }
}

static void buzzer_activate(void)
{
  app_log("buzzer_activate\r\n");
  is_buzzer_active = true;

  mikroe_cmt_8540s_smt_pwm_start();
}

static void buzzer_deactivate(void)
{
  app_log("buzzer_deactivate\r\n");
  is_buzzer_active = false;
  mikroe_cmt_8540s_smt_pwm_stop();
}

/***************************************************************************//**
 * Timer callback
 * Called periodically to reading data from CCS811.
 * Or used to send a notification
 ******************************************************************************/
static void air_quality_monitor_callback(sl_sleeptimer_timer_handle_t *timer,
                                         void                         *data)
{
  (void) data;

  if (timer == &air_quality_monitor_periodic_timer) {
    sl_bt_external_signal(AIR_QUALITY_MONITOR_EVENT);
  }
}

/*******************************************************************************
 *   Function to handle read data
 ******************************************************************************/
void air_quality_user_read_callback(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint16_t characteristic_size = 0;
  const uint8_t *characteristic_ptr = NULL;
  uint16_t sent_len;

  switch (evt->data.evt_gatt_server_user_read_request.characteristic) {
    // Notification characteristics value read
    case gattdb_notification_data:
      characteristic_size = sizeof(air_quality_data.notification_data);
      characteristic_ptr =
        (const uint8_t *) &air_quality_data.notification_data;
      break;

    // co2 characteristics value read
    case gattdb_co2_data:
      characteristic_size = sizeof(co2);
      characteristic_ptr = (const uint8_t *) &co2;
      break;

    // tvoc characteristics value read
    case gattdb_tvoc_data:
      characteristic_size = sizeof(tvoc);
      characteristic_ptr = (const uint8_t *) &tvoc;
      break;

    // Buzzer volume characteristics value read
    case gattdb_buzzer_data:
      characteristic_size = sizeof(air_quality_data.buzzer_data);
      characteristic_ptr = (const uint8_t *) &air_quality_data.buzzer_data;
      break;

    // Measurement period data characteristics value read
    case gattdb_measurement_period_data:
      characteristic_size = sizeof(air_quality_data.measurement_period_data);
      characteristic_ptr =
        (const uint8_t *) &air_quality_data.measurement_period_data;
      break;

    // Do nothing
    default:
      break;
  }

  // Send response
  sc = sl_bt_gatt_server_send_user_read_response(
    evt->data.evt_gatt_server_user_read_request.connection,
    evt->data.evt_gatt_server_user_read_request.characteristic,
    (uint8_t) 0x00, // SUCCESS
    characteristic_size, characteristic_ptr, &sent_len);
  app_assert_status(sc);
}

/*******************************************************************************
 *   Function to handle write data
 ******************************************************************************/
void air_quality_user_write_callback(sl_bt_msg_t *evt)
{
  uint8_t response_code = 0;
  sl_status_t status;
  Ecode_t err;
  uint16_t len = evt->data.evt_gatt_server_user_write_request.value.len;

  switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
    // Notification characteristic written
    case gattdb_notification_data:
    {
      uint8_t notification =
        evt->data.evt_gatt_server_user_write_request.value.data[0];
      if (len > NOTIFICATION_ATT_LENGTH) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
        break;
      }

      if (notification > IS_NOTIFICATION_ACTIVE_MAX) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_OUT_OF_RANGE;
        break;
      }
      // Store value in the runtime configuration structure
      if (notification) {
        air_quality_data.notification_data = 1;
      } else {
        air_quality_data.notification_data = 0;
        // Deactivate buzzer if it is active
        if (true == is_buzzer_active) {
          buzzer_deactivate();
        }
      }

      err = nvm3_user_set_notification_active(
        air_quality_data.notification_data);
      app_log_status(err);
      app_log("Write characteristic, ID: %x, value: %d\r\n",
              evt->data.evt_gatt_server_user_read_request.characteristic,
              notification);
      break;
    }

    case gattdb_buzzer_data:
    {
      uint8_t buzzer_data =
        evt->data.evt_gatt_server_user_write_request.value.data[0];

      if (len > BUZZER_ATT_LENGTH) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
        break;
      }

      if ((buzzer_data > BUZZER_VOLUME_MAX)
          || (buzzer_data < BUZZER_VOLUME_MIN)) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_OUT_OF_RANGE;
        break;
      }
      // Store value in the runtime configuration structure
      air_quality_data.buzzer_data = buzzer_data;
      err = nvm3_user_set_buzzer_volume(buzzer_data);
      app_log_status(err);

      mikroe_cmt_8540s_smt_set_duty_cycle(buzzer_data / 10.0);
      app_log("Buzzer set volume = %d\r\n", buzzer_data);

      app_log("Write characteristic, ID: %x, value: %d\r\n",
              evt->data.evt_gatt_server_user_read_request.characteristic,
              buzzer_data);
      break;
    }

    case gattdb_co2_data:
    {
      uint16_t threshold_co2 =
        (evt->data.evt_gatt_server_user_write_request.value.data[1] << 8)
        | evt->data.evt_gatt_server_user_write_request.value.data[0];

      if (len > CO2_ATT_LENGTH) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
        break;
      }

      if ((threshold_co2 > THRESHOLD_CO2_PPM_MAX)
          || (threshold_co2 < THRESHOLD_CO2_PPM_MIN)) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_OUT_OF_RANGE;
        break;
      }
      // Store value in the runtime configuration structure
      air_quality_data.threshold_co2_ppm = threshold_co2;
      err = nvm3_user_set_threshold_co2(threshold_co2);
      app_log_status(err);
      app_log("Write characteristic, ID: %x, value: %d\r\n",
              evt->data.evt_gatt_server_user_read_request.characteristic,
              threshold_co2);
      break;
    }

    case gattdb_tvoc_data:
    {
      uint16_t threshold_tvoc =
        (evt->data.evt_gatt_server_user_write_request.value.data[1] << 8)
        | evt->data.evt_gatt_server_user_write_request.value.data[0];

      if (len > TVOC_ATT_LENGTH) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
        break;
      }

      if ((threshold_tvoc > THRESHOLD_TVOC_PPB_MAX)
          || (threshold_tvoc < THRESHOLD_TVOC_PPB_MIN)) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_OUT_OF_RANGE;
        break;
      }
      // Store value in the runtime configuration structure
      air_quality_data.threshold_tvoc_ppb = threshold_tvoc;
      err = nvm3_user_set_threshold_tvoc(threshold_tvoc);
      app_log_status(err);
      app_log("Write characteristic, ID: %x, value: %d\r\n",
              evt->data.evt_gatt_server_user_read_request.characteristic,
              threshold_tvoc);
      break;
    }

    case gattdb_measurement_period_data:
    {
      uint8_t update_period =
        evt->data.evt_gatt_server_user_write_request.value.data[0];

      if (len > UPDATE_PERIOD_ATT_LENGTH) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
        break;
      }

      if ((update_period > UPDATE_PERIOD_IN_SECOND_MAX)
          || (update_period < UPDATE_PERIOD_IN_SECOND_MIN)) {
        response_code = (uint8_t) SL_STATUS_BT_ATT_OUT_OF_RANGE;
        break;
      }
      // Store value in the runtime configuration structure
      air_quality_data.measurement_period_data = update_period;
      err = nvm3_user_set_update_period(update_period);
      app_log_status(err);
      app_log("Write characteristic, ID: %x, value: %d\r\n",
              evt->data.evt_gatt_server_user_read_request.characteristic,
              update_period);
      // Start timer used for periodic measurement.
      app_log("Restarting timer for periodic measurement.\r\n");
      status = sl_sleeptimer_restart_periodic_timer_ms(
        &air_quality_monitor_periodic_timer, update_period * 1000,
        air_quality_monitor_callback,
        NULL,
        0,
        SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
      app_log_status(status);
      break;
    }

    // Write operation not permitted by default
    default:
      response_code = (uint8_t) SL_STATUS_BT_ATT_VALUE_NOT_ALLOWED;
      break;
  }

  // Send response
  status = sl_bt_gatt_server_send_user_write_response(
    evt->data.evt_gatt_server_user_write_request.connection,
    evt->data.evt_gatt_server_user_write_request.characteristic,
    response_code);
  app_assert_status(status);
}

/***************************************************************************//**
 * @brief
 *  This function processes the measured values.
 *  Filters the lowest and greatest values, and calculates an average.
 *
 * @param[in] data
 *  Pointer to data storage structure
 *
 * @return
 *  Returns sample filtered.
 ******************************************************************************/
static uint16_t air_quality_moving_average(uint16_t *data)
{
  uint32_t sum = 0;
  uint16_t min, max;
  uint8_t i;

  if (!samples_counter) {
    return 0;
  }

  // Calculate average value
  min = max = data[0];
  for (i = 0; i < samples_counter && i < DATA_BUFFER_SIZE; i++) {
    sum += data[i];
    if (data[i] > max) {
      max = data[i];
    }

    if (data[i] < min) {
      min = data[i];
    }
  }

  // Exclude  min. and max. values
  if (i > 2) {
    i -= 2;
    sum = sum - (min + max);
  }

  return (uint16_t) (sum / i);
}

/***************************************************************************//**
 * This function processes the measured values, checks the values against
 * the configured thresholds. If the measured value(s) are above
 * the configured thresholds => the buzzer will be activated.
 ******************************************************************************/
static void air_quality_data_process(void)
{
  co2 = air_quality_moving_average(co2_buffer);
  tvoc = air_quality_moving_average(tvoc_buffer);

  // Ignore if notification is not enable
  if (0 == air_quality_data.notification_data) {
    return;
  }

  // Check thresholds
  if ((co2 > air_quality_data.threshold_co2_ppm)
      || (tvoc > air_quality_data.threshold_tvoc_ppb)) {
    // Activate buzzer if it is not activated
    if (false == is_buzzer_active) {
      buzzer_activate();
    }
  } else {
    // deactivate buzzer if it is active
    if (true == is_buzzer_active) {
      buzzer_deactivate();
    }
  }
}

/***************************************************************************//**
 * Initialize OLED display.
 ******************************************************************************/
static void oled_app_init(void)
{
  // Initialize the OLED display
  ssd1306_init(sl_i2cspm_qwiic);
  glib_init(&glib_context);

  // Fill lcd with background color
  glib_clear(&glib_context);

  glib_draw_xbitmap(&glib_context,
                    0, 0, silicon_labs_logo_64x23,
                    64, 23, GLIB_WHITE);
  glib_update_display();

  sl_sleeptimer_delay_millisecond(1000);

  // Fill lcd with background color
  glib_clear(&glib_context);

  glib_draw_string(&glib_context, "AIR", 0, 0);
  glib_draw_string(&glib_context, "QUALITY", 21, 0);
  glib_draw_line(&glib_context, 0, 10, 63, 10, GLIB_WHITE);
  glib_draw_string(&glib_context, "CO2", 0, 14);
  glib_draw_string(&glib_context, "ppm", 45, 14);
  glib_draw_string(&glib_context, "VOC", 0, 26);
  glib_draw_string(&glib_context, "ppb", 45, 26);
  glib_draw_line(&glib_context, 0, 36, 63, 36, GLIB_WHITE);
  glib_draw_string(&glib_context, air_quality_status_text[0], 0, 39);
  glib_update_display();
  app_log(">> Oled display is initialized\r\n");
}

/***************************************************************************//**
 * Update the Air Quality Demo LCD Display
 ******************************************************************************/
static void air_quality_update_display(void)
{
  uint8_t co2_text_buffer[32];
  uint8_t tvoc_text_buffer[32];
  air_quality_status_t co2_status, tvoc_status, status;

  // The equivalent CO2 (eCO2) output range for CCS811 is from 400ppm up to
  //   8192ppm.
  // The equivalent tVOC (tVOC) output range for CCS811 is from 0ppm up to
  //   1187ppb.
  sprintf((char *) co2_text_buffer, "%4d", co2);
  sprintf((char *) tvoc_text_buffer, "%4d", tvoc);

  glib_fill_rect(&glib_context, 20, 13, 24, 8, GLIB_BLACK);
  glib_fill_rect(&glib_context, 20, 25, 24, 8, GLIB_BLACK);

  glib_draw_string(&glib_context, (char *) co2_text_buffer, 20, 13);
  glib_draw_string(&glib_context, (char *) tvoc_text_buffer, 20, 25);

  // Get status of air quality index.
  co2_status = air_quality_co2_status(co2);
  tvoc_status = air_quality_tvoc_status(tvoc);

  // The overall air quality index for indoors is thus
  // based on the worst air quality index rating among them
  status = (co2_status < tvoc_status ? tvoc_status : co2_status);
  glib_fill_rect(&glib_context, 0, 39, 64, 8, GLIB_BLACK);
  glib_draw_string(&glib_context, air_quality_status_text[status], 0, 39);

  glib_update_display();
}

/***************************************************************************//**
 * Update the status of air quality index to prepare show on LCD display
 *
 *  Air quality should be indicated on the display (with text, poor, good
 *   etc...)
 *  in accordance with these levels below.
 *
 *
 ******************************************************************************/
static air_quality_status_t air_quality_co2_status(uint16_t data)
{
  if (data < 400) {
    return EXCELLENT;
  } else if (data < 1000) {
    return FINE;
  } else if (data < 1500) {
    return MODERATE;
  } else if (data < 2000) {
    return POOR;
  } else if (data < 5000) {
    return VERY_POOR;
  }

  return SEVERE;
}

/***************************************************************************//**
 * Update the status of air quality index to prepare show on LCD display
 *
 *  Air quality should be indicated on the display (with text, poor, good
 *   etc...)
 *  in accordance with these levels below.
 *
 ******************************************************************************/
static air_quality_status_t air_quality_tvoc_status(uint16_t data)
{
  if (data < 50) {
    return EXCELLENT;
  } else if (data < 100) {
    return FINE;
  } else if (data < 150) {
    return MODERATE;
  } else if (data < 200) {
    return POOR;
  } else if (data < 300) {
    return VERY_POOR;
  }

  return SEVERE;
}
