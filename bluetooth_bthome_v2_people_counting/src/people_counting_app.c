/***************************************************************************//**
 * @file people_counting_app.c
 * @brief People counting application code
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
#include "sl_simple_led_instances.h"
#endif
#include "sl_simple_button_instances.h"
#include "user_config_nvm3.h"
#include "oled.h"
#include "app.h"
#include "vl53l1x_app.h"
#include "people_counting_app.h"

// -----------------------------------------------------------------------------
// Logging
#define TAG "people_counting"
// use applog for the log printing
#if defined(SL_CATALOG_APP_LOG_PRESENT) && APP_LOG_ENABLE
#include "app_log.h"
#define log_info(fmt, ...)   app_log_info("[" TAG "] " fmt, ## __VA_ARGS__)
#define log_error(fmt, ...)  app_log_error("[" TAG "] " fmt, ## __VA_ARGS__)
// use stdio printf for the log printing
#elif defined(SL_CATALOG_RETARGET_STDIO_PRESENT)
#define log_info(fmt, ...)   printf("[" TAG "] " fmt, ## __VA_ARGS__)
#define log_error(fmt, ...)  printf("[" TAG "] " fmt, ## __VA_ARGS__)
#else // the logging is disabled
#define log_info(...)
#define log_error(...)
#endif // #if defined(SL_CATALOG_APP_LOG_PRESENT)

// -----------------------------------------------------------------------------
// Led
#if defined(SL_CATALOG_SIMPLE_LED_PRESENT) && defined(SL_CATALOG_LED0_PRESENT)
#include "sl_simple_led_instances.h"
#define led0_on()   sl_led_turn_on(&sl_led_led0);
#define led0_off()  sl_led_turn_off(&sl_led_led0);
#else
#define led0_on()
#define led0_off()
#endif

/***************************************************************************//**
 * @addtogroup people_counting_app
 * @brief  People counting application.
 * @details
 * @{
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Defines
#define INVALID_BT_HANDLE               (0xff)

// -----------------------------------------------------------------------------
// Private variables

static uint8_t bt_connection_handle = INVALID_BT_HANDLE;

static sl_sleeptimer_timer_handle_t oled_timer;
static sl_sleeptimer_timer_handle_t people_counting_timer;
static bool notification_status = false;

static uint16_t last_people_count = (uint16_t)-1;
static uint32_t last_people_entered_so_far = (uint32_t)-1;

// -----------------------------------------------------------------------------
// Private function declarations

static void people_counting_event_handler(void);
static void people_counting_button_handler(void);
static void people_counting_oled_display_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data);
static void people_counting_sensor_sampling_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data);

// -----------------------------------------------------------------------------
// Public function definitions

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
void people_counting_app_init(void)
{
  // Load configuration from NVM
  user_config_nvm3_init();

  oled_app_init();

  vl53l1x_app_init();

  // Create oled display periodic timer
  sl_sleeptimer_start_periodic_timer_ms(&oled_timer,
                                        1000,
                                        people_counting_oled_display_callback,
                                        NULL,
                                        0,
                                        0);

  // Create sampling and calculate people count periodic timer
  sl_sleeptimer_start_periodic_timer_ms(&people_counting_timer,
                                        10,
                                        people_counting_sensor_sampling_callback,
                                        NULL,
                                        0,
                                        0);
}

/***************************************************************************//**
 * People Counting Application Process External Signal.
 ******************************************************************************/
void people_counting_process_evt_external_signal(uint32_t extsignals)
{
  if (extsignals & PEOPLE_COUNTING_BUTTON_EVENT) {
    people_counting_button_handler();
  }

  if (extsignals & PEOPLE_COUNTING_EVENT) {
    people_counting_event_handler();
  }

  if (extsignals & PEOPLE_COUNTING_SAMPLING_EVENT) {
    vl53l1x_app_process_action();
  }
}

/***************************************************************************//**
 * People Counting Application Process GATT Server User Write request.
 ******************************************************************************/
void people_counting_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data)
{
  sl_status_t sc = SL_STATUS_NOT_SUPPORTED;
  uint16_t value;
  uint8_t notification;

  // -------------------------------
  // Handle Voice configuration characteristics.
  switch (data->characteristic) {
    case gattdb_people_entered_so_far: {
      value = (uint8_t)(atoi((char *)data->value.data));
      if (value == 0) {
        app_log("[BLE_GATT]: Write: Clear people entered so far counter");
        vl53l1x_app_clear_people_entered_so_far();
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_VALUE_NOT_ALLOWED;
        app_log("[BLE_GATT]: Write 0 to clear people entered.\n");
      }

      break;
    }
    case gattdb_people_count: {
      value = (uint8_t)(atoi((char *)data->value.data));
      if (value == 0) {
        log_info("[BLE_GATT]: Write: Clear people counter\r\n");
        vl53l1x_app_clear_people_count();
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_VALUE_NOT_ALLOWED;
        app_log("[BLE_GATT]: Write 0 to clear people counter.\n");
      }
      break;
    }
    case gattdb_min_distance:
      value = (uint16_t)(atoi((char *)data->value.data));
      log_info("[BLE_GATT]: Write:  min_distance: %d\r\n", value);
      if (SL_STATUS_OK == user_config_nvm3_set_min_distance(value)) {
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }

      break;
    case gattdb_max_distance:
      value = (uint16_t)(atoi((char *)data->value.data));
      log_info("[BLE_GATT]: Write: max_distance: %d\r\n", value);
      if (SL_STATUS_OK == user_config_nvm3_set_max_distance(value)) {
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;
    case gattdb_distance_threshold:
      value = (uint16_t)(atoi((char *)data->value.data));
      log_info("[BLE_GATT]: Write: distance_threshold: %d\r\n", value);
      if (SL_STATUS_OK == user_config_nvm3_set_distance_threshold(value)) {
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;
    case gattdb_timing_budget:
      value = (uint16_t)(atoi((char *)data->value.data));
      log_info("[BLE_GATT]: Write: timing_budget: %d\r\n", value);
      if (SL_STATUS_OK == vl53l1x_app_change_timing_budget_in_ms(value)) {
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;

    case gattdb_notification_status:
      notification = (uint8_t)(atoi((char *)data->value.data));
      log_info("[BLE_GATT]: Write: notification_status: %d\r\n", notification);
      sc = user_config_nvm3_set_notification_status(notification);
      if ((SL_STATUS_OK == sc) && notification) {
        notification_status = true;
      } else {
        notification_status = false;
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;
    case gattdb_room_capacity:
      value = (uint16_t)(atoi((char *)data->value.data));
      log_info("[BLE_GATT]: Write: room capacity: %d\r\n", value);
      if (SL_STATUS_OK == user_config_nvm3_set_room_capacity(value)) {
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;
    default:
      break;
  }
  // Send write response.
  sc = sl_bt_gatt_server_send_user_write_response(
    data->connection,
    data->characteristic,
    (uint8_t)sc);
  app_assert_status(sc);
}

/***************************************************************************//**
 * People Counting Application Process GATT Server User Read request.
 ******************************************************************************/
void people_counting_process_evt_gatt_server_user_read_request(
  sl_bt_evt_gatt_server_user_read_request_t *data)
{
  sl_status_t sc;
  int len;
  static char string_temp[20];
  // -------------------------------
  // Handle Voice configuration characteristics.
  switch (data->characteristic) {
    case gattdb_people_entered_so_far: {
      uint32_t value = vl53l1x_app_get_people_entered_so_far();
      len = snprintf(string_temp, sizeof(string_temp), "%ld", value);
      app_log("[BLE_GATT]: Read: people_entered_so_far: %lu\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_people_count: {
      uint16_t value = vl53l1x_app_get_people_count();
      len = snprintf(string_temp, sizeof(string_temp), "%d", (uint16_t)value);
      app_log("[BLE_GATT]: Read: people_count: %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_min_distance: {
      uint16_t value = user_config_nvm3_get_min_distance();
      len = snprintf(string_temp, sizeof(string_temp), "%d", (uint16_t)value);
      app_log("[BLE_GATT]: Read: min_distance: %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_max_distance: {
      uint16_t value = user_config_nvm3_get_max_distance();
      len = snprintf(string_temp, sizeof(string_temp), "%d", (uint16_t)value);
      app_log("[BLE_GATT]: Read: max_distance: %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_distance_threshold: {
      uint16_t value = user_config_nvm3_get_distance_threshold();
      len = snprintf(string_temp, sizeof(string_temp), "%d", (uint16_t)value);
      app_log("[BLE_GATT]: Read: distance_threshold: %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_timing_budget: {
      uint16_t value = user_config_nvm3_get_timing_budget();
      len = snprintf(string_temp, sizeof(string_temp), "%d", (uint16_t)value);
      app_log("[BLE_GATT]: Read: timing_budget: %d\r\n", value);
      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_notification_status: {
      uint8_t value = user_config_nvm3_get_notification_status();
      len = snprintf(string_temp, sizeof(string_temp), "%d", (uint8_t)value);
      app_log("[BLE_GATT]: Read: notification_status: %d\r\n", value);

      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_room_capacity: {
      uint16_t value = user_config_nvm3_get_room_capacity();
      len = snprintf(string_temp, sizeof(string_temp), "%d", (uint16_t)value);
      app_log("[BLE_GATT]: Read: room_capacity: %d\r\n", value);

      // Send gatt response.
      sc = sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)string_temp,
        NULL);
      app_assert_status(sc);
      break;
    }
    default:
      break;
  }
}

/***************************************************************************//**
 * People Counting Application Process GATT Server User Read request.
 ******************************************************************************/
void people_counting_process_evt_gatt_server_characteristic_status(
  sl_bt_evt_gatt_server_characteristic_status_t *data)
{
  if (sl_bt_gatt_server_client_config
      != (sl_bt_gatt_server_characteristic_status_flag_t)data->status_flags) {
    return;
  }
  switch (data->characteristic) {
    case gattdb_notification_status:
      if (sl_bt_gatt_notification == data->client_config_flags) {
        // notification enabled.
        notification_status = true;
      } else {
        // notification disabled.
        notification_status = false;
      }
      break;

    default:
      break;
  }
}

/***************************************************************************//**
 * People Counting Application Set Current Bluetooth Connection Handle.
 ******************************************************************************/
void people_counting_set_bt_connection_handle(uint8_t connection)
{
  bt_connection_handle = connection;
}

/***************************************************************************//**
 * People Counting Application Reset Current Bluetooth Connection Handle.
 ******************************************************************************/
void people_counting_reset_bt_connection_handle(void)
{
  bt_connection_handle = INVALID_BT_HANDLE;
}

// -----------------------------------------------------------------------------
// Private function

static void send_notification_data_u16(uint16_t characteristic, uint16_t data)
{
  uint8_t notification_data[2];

  notification_data[0] = data & 0xff;
  notification_data[1] = data >> 8;
  sl_bt_gatt_server_send_notification(bt_connection_handle,
                                      characteristic,
                                      sizeof(notification_data),
                                      notification_data);
}

static void people_counting_event_handler(void)
{
  uint16_t people_count = vl53l1x_app_get_people_count();
  uint32_t people_entered_so_far = vl53l1x_app_get_people_entered_so_far();

  // Only display & notify people count when their value is changed
  if ((people_count != last_people_count)
      || (last_people_entered_so_far != people_entered_so_far)) {
    last_people_count = people_count;
    last_people_entered_so_far = people_entered_so_far;

    // Display people count on oled screen
    oled_show_people_count(people_count, people_entered_so_far);
    sl_bt_external_signal(8);

    if (user_config_nvm3_get_notification_status()
        && (bt_connection_handle != INVALID_BT_HANDLE)) {
      if (people_count == 0) {
        // notify room is empty
        send_notification_data_u16(gattdb_room_capacity, 0);
      }

      if (people_count > user_config_nvm3_get_room_capacity()) {
        send_notification_data_u16(gattdb_room_capacity, 1);
      }
    }
  }
}

static void people_counting_button_handler(void)
{
  vl53l1x_app_clear_people_count();
}

/***************************************************************************//**
 * Callback on timer period.
 ******************************************************************************/
static void people_counting_oled_display_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(PEOPLE_COUNTING_EVENT);
}

/***************************************************************************//**
 * Callback on timer period.
 ******************************************************************************/
static void people_counting_sensor_sampling_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(PEOPLE_COUNTING_SAMPLING_EVENT);
}

/***************************************************************************//**
 * Callback on button change.
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if ((sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED)
      && (&sl_button_btn0 == handle)) {
    sl_bt_external_signal(PEOPLE_COUNTING_BUTTON_EVENT);
  }
}

/** @} (end group people_counting_app) */
