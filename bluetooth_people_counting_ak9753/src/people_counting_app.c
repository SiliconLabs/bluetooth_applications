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

/***************************************************************************//**
 * People Counting Application Process GATT Server User Write request.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "app_assert.h"
#include "app_log.h"

#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "sl_simple_button_instances.h"

#include "user_config_nvm3.h"
#include "ak9753_app.h"
#include "ssd1306_app.h"
#include "people_counting_app.h"

#define INVALID_BT_HANDLE               (0xff)

// Event ID
#define PEOPLE_COUNTING_EVENT           (1 << 0)
#define PEOPLE_COUNTING_BUTTON_EVENT    (1 << 1)
#define PEOPLE_COUNTING_SAMPLING_EVENT  (1 << 2)

// Local variable
static uint16_t last_people_count = (uint16_t)-1;
static uint32_t last_people_entered_so_far = (uint32_t)-1;

static uint8_t bt_connection_handle = INVALID_BT_HANDLE;
static sl_sleeptimer_timer_handle_t oled_timer;
static sl_sleeptimer_timer_handle_t people_counting_timer;

// Local function declaration
static void people_counting_sensor_sampling_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data);
static void send_notification_data_u16(uint16_t characteristic, uint16_t data);
static void people_counting_oled_display_callback(
  sl_sleeptimer_timer_handle_t *timer, void *data);

/***************************************************************************//**
 * People Counting Application Init.
 ******************************************************************************/
void people_counting_app_init(void)
{
  user_config_nvm3_init();

  ak9753_app_init();

  ssd1306_app_init();

  // Create oled display periodic timer
  sl_sleeptimer_start_periodic_timer_ms(&oled_timer,
                                        1000,
                                        people_counting_oled_display_callback,
                                        NULL,
                                        0,
                                        0);

  // Create sampling and calculate people count periodic timer
  sl_sleeptimer_start_periodic_timer_ms(&people_counting_timer,
                                        2,
                                        people_counting_sensor_sampling_callback,
                                        NULL,
                                        0,
                                        0);
}

/***************************************************************************//**
 * People Counting Application Process GATT Server User Write request.
 ******************************************************************************/
void people_counting_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data)
{
  sl_status_t sc = SL_STATUS_NOT_SUPPORTED;

  switch (data->characteristic) {
    case gattdb_room_capacity:
      if (data->value.len <= 5) {
        uint16_t temp_value = (uint16_t)atoi((char *) data->value.data);
        app_log("\rGATT: write: room capacity: %d\n", temp_value);
        if (user_config_nvm3_set_room_capacity(temp_value) == SL_STATUS_OK) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;

    case gattdb_people_entered_so_far:
      app_log("\rGATT: write: clear people enter so far\n");
      ak9753_app_clear_people_entered_so_far();
      sc = SL_STATUS_OK;
      break;

    case gattdb_people_count:
      app_log("\rGATT: write: clear people count\n");
      ak9753_app_clear_people_count();
      sc = SL_STATUS_OK;
      break;

    case gattdb_lower_threshold:
      if (data->value.len <= 5) {
        int16_t temp_value = (int16_t)atoi((char *) data->value.data);
        app_log("\rGATT: write: lower threshold: %d\n", temp_value);
        if (user_config_nvm3_set_lower_threshold(temp_value) == SL_STATUS_OK) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;

    case gattdb_upper_threshold:
      if (data->value.len <= 5) {
        int16_t temp_value = (int16_t)atoi((char *) data->value.data);
        app_log("\rGATT: write: upper_threshold: %d\n", temp_value);
        if (user_config_nvm3_set_upper_threshold(temp_value) == SL_STATUS_OK) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;

    case gattdb_ir_threshold:
      if (data->value.len <= 5) {
        int16_t temp_value = (int16_t)atoi((char *) data->value.data);
        app_log("\rGATT: write: ir_threshold: %d\n", temp_value);
        if (user_config_nvm3_set_ir_threshold(temp_value) == SL_STATUS_OK) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;

    case gattdb_hysteresis:
      if (data->value.len <= 5) {
        int16_t temp_value = (int16_t)atoi((char *) data->value.data);
        app_log("\rGATT: write: hysteresis: %d\n", temp_value);
        if (user_config_nvm3_set_hysteresis(temp_value) == SL_STATUS_OK) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;

    case gattdb_notification_status:
      if (data->value.len <= 1) {
        uint8_t temp_value = (uint8_t)atoi((char *) data->value.data);
        app_log("\rGATT: write: notification_status: %d\n", temp_value);
        if (user_config_nvm3_set_notification_status(temp_value)
            == SL_STATUS_OK) {
          sc = SL_STATUS_OK;
        } else {
          sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        }
      } else {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      }
      break;

    default:
      break;
  }

  sc = sl_bt_gatt_server_send_user_write_response(data->connection,
                                                  data->characteristic,
                                                  sc);
  app_assert_status(sc);
}

/***************************************************************************//**
 * People Counting Application Process GATT Server User Read request.
 ******************************************************************************/
void people_counting_process_evt_gatt_server_user_read_request(
  sl_bt_evt_gatt_server_user_read_request_t *data)
{
  sl_status_t sc;
  char send_data[50];
  uint8_t len;

  switch (data->characteristic) {
    case gattdb_room_capacity: {
      uint16_t value = user_config_nvm3_get_room_capacity();
      app_log("\rGATT: read: room capacity: %d\n", value);
      len = snprintf(send_data, sizeof(send_data), "%d", value);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }

    case gattdb_people_entered_so_far: {
      uint16_t value = user_config_nvm3_get_people_entered_so_far();
      app_log("\rGATT: read: people entered so far: %d\n", value);
      len = snprintf(send_data, sizeof(send_data), "%d", value);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }

    case gattdb_people_count: {
      uint16_t value = ak9753_app_get_people_count();
      app_log("\rGATT: read: people count: %d\n", value);
      len = snprintf(send_data, sizeof(send_data), "%d", value);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }

    case gattdb_lower_threshold: {
      int16_t threshold = user_config_nvm3_get_lower_threshold();
      app_log("\rGATT: read: lower threshold: %d\n", threshold);
      len = snprintf(send_data, sizeof(send_data), "%d", threshold);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }

    case gattdb_upper_threshold: {
      int16_t threshold = user_config_nvm3_get_upper_threshold();
      app_log("\rGATT: read: upper threshold: %d\n", threshold);
      len = snprintf(send_data, sizeof(send_data), "%d", threshold);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }

    case gattdb_ir_threshold: {
      int16_t threshold = user_config_nvm3_get_ir_threshold();
      app_log("\rGATT: read: ir threshold: %d\n", threshold);
      len = snprintf(send_data, sizeof(send_data), "%d", threshold);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      break;
    }

    case gattdb_hysteresis: {
      uint16_t hysteresis = user_config_nvm3_get_hysteresis();
      app_log("\rGATT: read: hysteresis: %d\n", hysteresis);
      len = snprintf(send_data, sizeof(send_data), "%d", hysteresis);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }

    case gattdb_notification_status: {
      bool status = user_config_nvm3_get_notification_status();
      app_log("\rGATT: read: Notification status: %d\n", (uint8_t)status);
      len = snprintf(send_data, sizeof(send_data), "%d", status);
      sc = sl_bt_gatt_server_send_user_read_response(data->connection,
                                                     data->characteristic,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)send_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }

    default:
      break;
  }
}

/***************************************************************************//**
 * People Counting Application Process GATT Server Characteristic Status.
 ******************************************************************************/
void people_counting_process_evt_gatt_server_characteristic_status(
  sl_bt_evt_gatt_server_characteristic_status_t *data)
{
  if (data->characteristic == gattdb_notification_status) {
    app_log("\rchange\n");
  }
}

/***************************************************************************//**
 * Callback on timer period.
 ******************************************************************************/
void people_counting_oled_display_callback(
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
 * People Counting Application Process External Signal.
 ******************************************************************************/
void people_counting_process_evt_external_signal(uint32_t extsignals)
{
  if (extsignals & PEOPLE_COUNTING_BUTTON_EVENT) {
    ak9753_app_clear_people_count();
  }

  if (extsignals & PEOPLE_COUNTING_EVENT) {
    uint16_t people_count = ak9753_app_get_people_count();
    uint16_t people_entered_so_far = ak9753_app_get_people_entered_so_far();

    // Only display & notify people count when their value is changed
    if ((people_count != last_people_count)
        || (last_people_entered_so_far != people_entered_so_far)) {
      last_people_count = people_count;
      last_people_entered_so_far = people_entered_so_far;

      // Display people count on oled screen
      ssd1306_show_people_count(people_count, people_entered_so_far);

      if (user_config_nvm3_get_notification_status()
          && (bt_connection_handle != INVALID_BT_HANDLE)) {
        if (people_count == 0) {
          send_notification_data_u16(gattdb_room_capacity, 0);
        }
        if (people_count > user_config_nvm3_get_room_capacity()) {
          send_notification_data_u16(gattdb_room_capacity, 1);
        }
      }
    }
  }

  if (extsignals & PEOPLE_COUNTING_SAMPLING_EVENT) {
    ak9753_app_process_action();
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

/***************************************************************************//**
 * Send notification data to GATT client.
 ******************************************************************************/
void send_notification_data_u16(uint16_t characteristic, uint16_t data)
{
  uint8_t notification_data[2];

  notification_data[0] = data & 0xff;
  notification_data[1] = data >> 8;
  sl_bt_gatt_server_send_notification(bt_connection_handle,
                                      characteristic,
                                      sizeof(notification_data),
                                      notification_data);
}

/***************************************************************************//**
 * Callback on button change.
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      sl_bt_external_signal(PEOPLE_COUNTING_BUTTON_EVENT);
    }
  }
}
