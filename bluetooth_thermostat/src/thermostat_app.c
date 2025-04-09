/***************************************************************************//**
 * @file thermostat_app.c
 * @brief Thermostat application code
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "sl_bluetooth.h"
#include "sl_udelay.h"
#include "gatt_db.h"
#include "printf.h"
#include "app_assert.h"
#include "user_config_nvm3.h"
#include "sl_simple_led_instances.h"
#include "sl_simple_button_instances.h"

#include "thermostat_app.h"
#include "buzz2_app.h"
#include "temphum9_app.h"
#include "mikroe_shtc3.h"
#include "mikroe_cmt_8540s_smt.h"
#include "glib.h"
#include "oled_app.h"
#include "thermostat_app.h"
#include "buzz2_app.h"

/***************************************************************************//**
 * @addtogroup thermostat_app
 * @brief  Thermostat application.
 * @details
 * @{
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Defines
#define INVALID_BT_HANDLE                (0xff)
#define THERMOSTAT_TIMER_EVENT           (1 << 0)
#define THERMOSTAT_BUTTON_EVENT          (1 << 1)
#define DATA_BUFFER_SIZE                 5

// -----------------------------------------------------------------------------
// Enum
typedef enum {
  HEAT = 0,
  COOL =1
} mode_t;

typedef enum {
  ACTUATOR_OFF = 0,
  ACTUATOR_ON =1
} output_control_t;

// -----------------------------------------------------------------------------
// Private variables
static sl_sleeptimer_timer_handle_t thermostat_timer;

static mikroe_shtc3_measurement_data_t measurement_value;
static bool is_actuator_enable = false;
static bool is_alarm_enable = true;
static bool is_alarm_active = false;
static bool is_invalid_value = false;
static float lower_threshold;
static float upper_threshold;
static float setpoint;
static float hysteresis;
static mode_t mode;
// Buffer to store measurement data.
static float temperature_buffer[DATA_BUFFER_SIZE];
static float humidity_buffer[DATA_BUFFER_SIZE];
static uint32_t samples_counter;
static bool buffer_full = false;

// temperature result from the SHTC3.
static float temperature;

// humidity result from the SHTC3.
static float humidity;

// -----------------------------------------------------------------------------
// Private function declarations

static void thermostat_periodic_timer_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data);

static uint8_t bt_connection_handle = INVALID_BT_HANDLE;
static void thermostat_timer_event_handler(void);
static void thermostat_data_process(void);
static void thermostat_button_event_handler (void);
static int16_t thermostat_app_get_temperature(void);
static uint16_t thermostat_app_get_humidity(void);
static uint8_t thermostat_app_get_threshold_alarm_status(void);
static void thermostat_output_control(output_control_t output_control);
static float thermostat_calculate_average(float *data);
static void buzzer_activate(void);
static void buzzer_deactivate(void);

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
void thermostat_app_init(void)
{
  oled_app_init();

  // Load configuration from NVM
  user_config_nvm3_init();

  hysteresis = user_config_nvm3_get_hysteresis() / 100;
  setpoint = user_config_nvm3_get_setpoint() / 100;
  lower_threshold = user_config_nvm3_get_lower_threshold() / 100;
  upper_threshold = user_config_nvm3_get_uppper_threshold() / 100;
  is_alarm_enable = user_config_nvm3_get_alarm_status();

  buzz2_app_init();

  temphum9_app_init();

  // Create oled display periodic timer
  sl_sleeptimer_start_periodic_timer_ms(&thermostat_timer,
                                        5000,
                                        thermostat_periodic_timer_callback,
                                        NULL,
                                        0,
                                        0);
}

/***************************************************************************//**
 * Thermostat Application Set Current Bluetooth Connection Handle.
 ******************************************************************************/
void thermostat_set_bt_connection_handle(uint8_t connection)
{
  bt_connection_handle = connection;
}

/***************************************************************************//**
 * Thermostat Application Reset Current Bluetooth Connection Handle.
 ******************************************************************************/
void thermostat_reset_bt_connection_handle(void)
{
  bt_connection_handle = INVALID_BT_HANDLE;
}

/***************************************************************************//**
 * Thermostat Application Process GATT Server User Write request.
 ******************************************************************************/
void thermostat_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data)
{
  sl_status_t sc = SL_STATUS_NOT_SUPPORTED;

  // -------------------------------

  switch (data->characteristic) {
    case gattdb_mode: {
      if (data->value.len == 1) {
        uint8_t value = (data->value.data[0]);
        app_log("GATT: write:  mode: %d\r\n", value);

        mode = (mode_t)value;
        if (SL_STATUS_OK == user_config_nvm3_set_mode(value)) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;
    }
    case gattdb_setpoint: {
      if (data->value.len == 2) {
        int16_t value =
          (int16_t)  ((data->value.data[0] << 8)
                      | (data->value.data[1]));
        app_log("GATT: write:  set point: %d\r\n", value);

        setpoint = (float)value / 100;
        if (SL_STATUS_OK == user_config_nvm3_set_setpoint(value)) {
          oled_update(temperature, humidity, setpoint, hysteresis);

          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;
    }

    case gattdb_hysteresis: {
      if (data->value.len == 2) {
        int16_t value = (int16_t)((data->value.data[0] << 8)
                                  | (data->value.data[1]));
        app_log("GATT: write: hysteresis: %d\r\n", value);

        hysteresis = (float)value / 100;
        if (SL_STATUS_OK == user_config_nvm3_set_hysteresis(value)) {
          oled_update(temperature, humidity, setpoint, hysteresis);
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;
    }
    case gattdb_lower_threshold: {
      if (data->value.len == 2) {
        int16_t value = (int16_t)((data->value.data[0] << 8)
                                  | (data->value.data[1]));
        app_log("GATT: write: lower threshold: %d\r\n", value);
        lower_threshold = (float)value / 100;
        if (SL_STATUS_OK == user_config_nvm3_set_lower_threshold(value)) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;
    }
    case gattdb_upper_threshold: {
      if (data->value.len == 2) {
        int16_t value = (int16_t)((data->value.data[0] << 8)
                                  | (data->value.data[1]));
        app_log("GATT: write: upper threshold: %d\r\n", value);
        upper_threshold = (float)value / 100;
        if (SL_STATUS_OK == user_config_nvm3_set_upper_threshold(value)) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;
    }
    case gattdb_threshold_alarm_status: {
      if (data->value.len == 1) {
        uint8_t value = (data->value.data[0]);
        app_log("GATT: set threshold alarm status %d\r\n", value);
        is_alarm_enable = (bool)(value);
        if (SL_STATUS_OK == user_config_nvm3_set_alarm_status(value)) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;
    }
  }
  // Send write response.
  sc = sl_bt_gatt_server_send_user_write_response(
    data->connection,
    data->characteristic,
    sc);
  app_assert_status(sc);
}

/***************************************************************************//**
 * Thermostat Application Process GATT Server User Read request.
 ******************************************************************************/
void thermostat_process_evt_gatt_server_user_read_request(
  sl_bt_evt_gatt_server_user_read_request_t *data)
{
  sl_status_t sc;

  // -------------------------------
  // Handle Voice configuration characteristics.
  switch (data->characteristic) {
    case gattdb_mode: {
      uint8_t value = user_config_nvm3_get_mode();
      app_log("GATT: read: mode: %u\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value),
        (uint8_t *)&value,
        NULL);
      app_assert_status(sc);

      break;
    }
    case gattdb_setpoint: {
      int16_t value = user_config_nvm3_get_setpoint();
      int16_t value_to_send = (value >> 8) | (value << 8);
      app_log("GATT: read: setpoint: %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value_to_send),
        (uint8_t *)&value_to_send,
        NULL);
      app_assert_status(sc);

      break;
    }
    case gattdb_temperature: {
      int16_t value = thermostat_app_get_temperature();
      int16_t value_to_send = (value >> 8) | (value << 8);
      app_log("GATT: current temperature : %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value_to_send),
        (uint8_t *)&value_to_send,
        NULL);
      app_assert_status(sc);

      break;
    }
    case gattdb_humidity: {
      uint16_t value = thermostat_app_get_humidity();
      uint16_t value_to_send = (value >> 8) | (value << 8);
      app_log("GATT: current humidity : %u\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value_to_send),
        (uint8_t *)&value_to_send,
        NULL);
      app_assert_status(sc);

      break;
    }
    case gattdb_hysteresis: {
      uint16_t value = user_config_nvm3_get_hysteresis();
      uint16_t value_to_send = (value >> 8) | (value << 8);
      app_log("GATT: hysteresis : %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value_to_send),
        (uint8_t *)&value_to_send,
        NULL);
      app_assert_status(sc);

      break;
    }
    case gattdb_lower_threshold: {
      int16_t value = user_config_nvm3_get_lower_threshold();
      int16_t value_to_send = (value >> 8) | (value << 8);
      app_log("GATT: lower_threshold : %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value_to_send),
        (uint8_t *)&value_to_send,
        NULL);
      app_assert_status(sc);

      break;
    }
    case gattdb_upper_threshold: {
      int16_t value = user_config_nvm3_get_uppper_threshold();
      int16_t value_to_send = (value >> 8) | (value << 8);
      app_log("GATT: upper threshold : %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value_to_send),
        (uint8_t *)&value_to_send,
        NULL);
      app_assert_status(sc);

      break;
    }
    case gattdb_threshold_alarm_status: {
      uint8_t value = thermostat_app_get_threshold_alarm_status();
      app_log("GATT: threshold alarm status : %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value),
        (uint8_t *)&value,
        NULL);
      app_assert_status(sc);
      break;
    }
  }
}

static void buzzer_activate(void)
{
  printf("Buzzer start\n");
  mikroe_cmt_8540s_smt_pwm_start();
}

static void buzzer_deactivate(void)
{
  printf("Buzzer stop\n");
  mikroe_cmt_8540s_smt_pwm_stop();
}

/***************************************************************************//**
 * Thermostat Application Process External Signal.
 ******************************************************************************/
void thermostat_process_evt_external_signal(uint32_t extsignals)
{
  if (extsignals & THERMOSTAT_TIMER_EVENT) {
    thermostat_timer_event_handler();
  }

  if (extsignals & THERMOSTAT_BUTTON_EVENT) {
    thermostat_button_event_handler();
  }
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
static float thermostat_calculate_average(float *data)
{
  float sum = 0;
  float min, max;
  uint8_t i, max_count;

  // Calculate average value
  min = max = data[0];
  max_count = buffer_full ? DATA_BUFFER_SIZE:samples_counter;
  for (i = 0; i < max_count; i++) {
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

  return (float) (sum / i);
}

/***************************************************************************//**
 * Callback on timer period.
 ******************************************************************************/
static void thermostat_periodic_timer_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(THERMOSTAT_TIMER_EVENT);
}

static void thermostat_timer_event_handler(void)
{
  mikroe_shtc3_send_command(MIKROE_SHTC3_CMD_WAKEUP);
  sl_udelay_wait(100);
// sl_sleeptimer_delay_millisecond(1);
  mikroe_shtc3_get_temperature_and_humidity(SHTC3_DATA_MODE_NORMAL,
                                            &measurement_value);

  if ((measurement_value.temperature < (-40))
      || (measurement_value.temperature > 125)
      || (measurement_value.humidity < 0)
      || (measurement_value.humidity > 100)) {
    // error flag
    printf("invalid data\n");
    is_invalid_value = true;
    thermostat_output_control(ACTUATOR_OFF);
    if (is_alarm_enable == true) {
      is_alarm_active = true;
      // start buzzer.
      buzzer_activate();
    }
    return;
  } else {
    is_invalid_value = false;
    printf(">> Current Temp: %.2f °C Current RH: %.2f %%\n",
           measurement_value.temperature,
           measurement_value.humidity);
  }

  uint8_t index = samples_counter % DATA_BUFFER_SIZE;
  temperature_buffer[index] = measurement_value.temperature;
  humidity_buffer[index] = measurement_value.humidity;
  samples_counter++;
  if (samples_counter >= DATA_BUFFER_SIZE) {
    buffer_full = true;
  }

  thermostat_data_process();

  mikroe_shtc3_send_command(MIKROE_SHTC3_CMD_SLEEP);
}

static void thermostat_data_process(void)
{
  temperature = thermostat_calculate_average(temperature_buffer);
  humidity = thermostat_calculate_average(humidity_buffer);

  printf(">> Average Temp: %.2f °C Average RH: %.2f %%\n", temperature,
         humidity);

  oled_update(temperature, humidity, setpoint, hysteresis);

  if (is_actuator_enable == true) {
    if (mode == HEAT) {
      if (temperature >= setpoint) {
        // disable actuator
        is_actuator_enable = false;
        thermostat_output_control(ACTUATOR_OFF);
      }
    } else if (mode == COOL) {
      if (temperature <= setpoint) {
        // disable actuator
        is_actuator_enable = false;
        thermostat_output_control(ACTUATOR_OFF);
      }
    }
  } else {
    if (mode == HEAT) {
      if (temperature < (setpoint - hysteresis)) {
        // enable actuator
        is_actuator_enable = true;
        thermostat_output_control(ACTUATOR_ON);
      }
    } else if (mode == COOL) {
      if (temperature > (setpoint + hysteresis)) {
        // enable actuator
        is_actuator_enable = true;
        thermostat_output_control(ACTUATOR_ON);
      }
    }
  }

  if (is_alarm_enable == true) {
    if ((temperature > upper_threshold)
        || (temperature < lower_threshold) || (is_invalid_value == true)) {
      // outside => activate
      is_alarm_active = true;
      buzzer_activate();
    } else {
      is_alarm_active = false;
      buzzer_deactivate();
    }
  }
}

static void thermostat_button_event_handler(void)
{
  if (is_alarm_enable == true) {
    // disable notification
    is_alarm_enable = false;
    user_config_nvm3_set_alarm_status(false);

    if (is_alarm_active == true) {
      // deactivate buzzer
      is_alarm_active = false;
      mikroe_cmt_8540s_smt_pwm_stop();
    }
  } else {
    // enable notification
    is_alarm_enable = true;
    user_config_nvm3_set_alarm_status(true);

    mikroe_cmt_8540s_smt_pwm_stop();
  }
}

static int16_t thermostat_app_get_temperature(void)
{
  return (int16_t)(measurement_value.temperature * 100);
}

static uint16_t thermostat_app_get_humidity(void)
{
  return (uint16_t)(measurement_value.humidity * 100);
}

static uint8_t thermostat_app_get_threshold_alarm_status(void)
{
  if (is_alarm_active == true) {
    return 2;
  }

  return (uint8_t)(is_alarm_enable);
}

static void thermostat_output_control(output_control_t output_control)
{
  if (output_control == ACTUATOR_ON) {
    sl_led_turn_on(SL_SIMPLE_LED_INSTANCE(0));
  } else {
    sl_led_turn_off(SL_SIMPLE_LED_INSTANCE(0));
  }
}

/***************************************************************************//**
 * Callback on button change.
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      sl_bt_external_signal(THERMOSTAT_BUTTON_EVENT);
    }
  }
}

/** @} (end group thermostat_app) */
