/***************************************************************************//**
 * @file app_events.c
 * @brief Application Events Source File
 * @version 1.0.0
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
#include <stdbool.h>

#include "sl_bt_api.h"
#include "app_log.h"
#include "app_assert.h"
#include "gatt_db.h"

#include "app_events.h"
#include "app_callbacks.h"
#include "app_logic_md.h"
#include "app_config.h"
#include "app_ble_events.h"

static void convert_integer_to_ascii(
  uint8_t *string,
  int input,
  uint16_t *len);

/***************************************************************************//**
 * Handles BLE user read/write requests for the configured characteristics.
 ******************************************************************************/
void app_event_handler_on_char_requests(uint8_t access_type, sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint16_t length = 0;
  uint8_t ascii_buffer[16];
  int temp_int = 0;
  md_feature_t *feature = NULL;

  // Find application feature-set for the requested BLE characteristic
  for (int i = 0; i < MD_BLE_FEATURE_LENGTH; i++) {
    if (((BLE_CHAR_ACCESS_TYPE_READ == access_type)
         && (evt->data.evt_gatt_server_user_read_request.characteristic
             == md_features[i].char_id))
        || ((BLE_CHAR_ACCESS_TYPE_WRITE == access_type)
            && (evt->data.evt_gatt_server_user_write_request.characteristic
                == md_features[i].char_id))) {
      // Set current feature
      feature = &md_features[i];
      break;
    }
  }

  app_log("FEATURE ID: %d \n", feature->char_id);

  if (NULL != feature) {
    if (BLE_CHAR_ACCESS_TYPE_READ == access_type) {
      app_log("Read characteristic, ID: %x, value: %d\n",
              evt->data.evt_gatt_server_user_read_request.characteristic,
              (sizeof(uint8_t) == feature->data_length
               ?*((uint8_t * )(feature->data)) : *((uint16_t * )(feature->data))));

      // Convert integers to ASCII string
      convert_integer_to_ascii(
        ascii_buffer,
        (sizeof(uint8_t) == feature->data_length
         ?*((uint8_t *) (feature->data)) : *((uint16_t *) (feature->data))),
        &length);

      // Send response
      sl_bt_gatt_server_send_user_read_response(
        evt->data.evt_gatt_server_user_read_request.connection,
        evt->data.evt_gatt_server_user_read_request.characteristic,
        SL_STATUS_OK, /* SUCCESS */
        length,
        ascii_buffer,
        &length);
    } else {
      // Check value length
      if (evt->data.evt_gatt_server_user_write_request.value.len
          > sizeof(ascii_buffer)) {
        // Send invalid attribute value length error
        sl_bt_gatt_server_send_user_write_response(
          evt->data.evt_gatt_server_user_read_request.connection,
          evt->data.evt_gatt_server_user_read_request.characteristic,
          (uint8_t) SL_STATUS_BT_ATT_INVALID_ATT_LENGTH);
      } else {
        // Copy received data into a buffer
        memcpy(
          ascii_buffer,
          evt->data.evt_gatt_server_user_write_request.value.data,
          evt->data.evt_gatt_server_user_write_request.value.len);

        // Convert ASCII to integer
        temp_int = atoi(
          (char *) evt->data.evt_gatt_server_user_write_request.value.data);

        // Check value range
        if ((temp_int >= feature->value_min)
            && (temp_int <= feature->value_max)) {
          app_log(
            "Write characteristic, ID: %x, value: %d\n",
            evt->data.evt_gatt_server_user_read_request.characteristic,
            temp_int);

          // Store value in the runtime configuration structure
          if (sizeof(uint8_t) == feature->data_length) {
            *((uint8_t *) feature->data) = (uint8_t) temp_int;
          } else {
            *((uint16_t *) feature->data) = (uint16_t) temp_int;
          }

          // Store value in NVM
          sc = sl_bt_nvm_erase(feature->nvm_key);
          app_assert(
            (sc == SL_STATUS_OK) || (sc == SL_STATUS_BT_PS_KEY_NOT_FOUND),
            "[E: 0x%04x] Failed to Erase NVM\n",
            (int ) sc);

          sc = sl_bt_nvm_save(feature->nvm_key,
                              feature->data_length,
                              feature->data);
          app_assert(sc == SL_STATUS_OK,
                     "[E: 0x%04x] Failed to write NVM\n",
                     (int ) sc);

          // Send write operation successful status
          sl_bt_gatt_server_send_user_write_response(
            evt->data.evt_gatt_server_user_read_request.connection,
            evt->data.evt_gatt_server_user_read_request.characteristic,
            SL_STATUS_OK /* SUCCESS */);
        } else {
          // Send value not allowed error
          sl_bt_gatt_server_send_user_write_response(
            evt->data.evt_gatt_server_user_read_request.connection,
            evt->data.evt_gatt_server_user_read_request.characteristic,
            (uint8_t) SL_STATUS_BT_ATT_VALUE_NOT_ALLOWED);
        }
      }
    }
  } else {
    // Send request not supported
    if (BLE_CHAR_ACCESS_TYPE_READ == access_type) {
      sl_bt_gatt_server_send_user_read_response(
        evt->data.evt_gatt_server_user_read_request.connection,
        evt->data.evt_gatt_server_user_read_request.characteristic,
        (uint8_t) SL_STATUS_BT_ATT_REQUEST_NOT_SUPPORTED, 0, NULL,
        NULL);
    } else {
      sl_bt_gatt_server_send_user_write_response(
        evt->data.evt_gatt_server_user_read_request.connection,
        evt->data.evt_gatt_server_user_read_request.characteristic,
        (uint8_t) SL_STATUS_BT_ATT_REQUEST_NOT_SUPPORTED);
    }
  }

  // Reset last request timeout timer.
  app_logic_md_reset_last_req_conf_timer();
}

/***************************************************************************//**
 * Handles external events, like expiring timers, pressing buttons.
 ******************************************************************************/
void app_event_handler_on_external_event(sl_bt_msg_t *evt)
{
  if (evt->data.evt_system_external_signal.extsignals
      & MD_LAST_REQ_TIMEOUT_TIMER_EVENT) {
    // Restart the device.
    sl_bt_system_reset(0);
  } else if (evt->data.evt_system_external_signal.extsignals
             & MD_ACC_WAKEUP_EVENT) {
    app_logic_md_handle_acc_wakeup_evt();
  } else if (evt->data.evt_system_external_signal.extsignals
             & MD_WAKEUP_PERIOD_TIMER_EVENT) {
    app_logic_md_handle_wakeup_time_period_evt();
  } else if (evt->data.evt_system_external_signal.extsignals
             & MD_NOTIFY_TIMER_EVENT) {
    app_logic_md_handle_notify_timer();
  } else if (evt->data.evt_system_external_signal.extsignals
             & MD_NOTIFY_BREAK_TIMER_EVENT) {
    app_logic_md_handle_notify_break_timer();
  }
}

static void convert_integer_to_ascii(
  uint8_t *string,
  int input,
  uint16_t *len)
{
  sprintf((char *) string, "%d", input);
  *len = strlen((const char *) string);
}
