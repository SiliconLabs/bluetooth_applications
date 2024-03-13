/***************************************************************************//**
 * @file output_handler.cc
 * @brief output handler file.
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
 * 1. The origin of this software must not be misrepresented{} you must not
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
#include "sl_led.h"
#include "sl_simple_led_instances.h"

#include "gatt_db.h"

#include "app_log.h"
#include "app_assert.h"

#include "hand_signal_app.h"
#include "hand_signal_display.h"
#include "output_handler.h"

static bool notifications_enabled = false;
// The format for hand_signal characteristic: <detect_class_id> <score>
static uint8_t data[2];

/***************************************************************************//**
 * Handle inference result
 ******************************************************************************/
void handle_result(int32_t current_time,
                   uint8_t result,
                   uint8_t score,
                   bool is_new_command)
{
  sl_status_t sc;
  const char *label = get_category_label(result);

  sl_led_toggle(&sl_led_led0);

  if (notifications_enabled) {
    sc = sl_bt_gatt_server_notify_all(gattdb_hand_signal,
                                      sizeof(data),
                                      data);
    app_assert_status(sc);
  }

  if (is_new_command) {
    // Update data for ble
    data[0] = result;
    data[1] = score;
    hand_signal_display_update(result, score);
    app_log("Detected class=%d label=%s score=%d @%ldms\n",
            result,
            label,
            score,
            current_time);
  }
}

/*******************************************************************************
 * @brief
 *   Function to handle evt_gatt_server_characteristic_status_id event
 ******************************************************************************/
void hand_signal_characteristic_status(sl_bt_msg_t *evt)
{
  // Notification or Indication status changed for Hand signal
  if (evt->data.evt_gatt_server_characteristic_status.status_flags
      == sl_bt_gatt_server_client_config) {
    notifications_enabled
      = evt->data.evt_gatt_server_characteristic_status.client_config_flags;
  }
}

/*******************************************************************************
 * @brief
 *   Function to handle sl_bt_evt_gatt_server_user_read_request_id event
 ******************************************************************************/
void hand_signal_read_requests(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint16_t data_len;

  if (evt->data.evt_gatt_server_user_read_request.characteristic
      == gattdb_hand_signal) {
    // Send response
    sc = sl_bt_gatt_server_send_user_read_response(
      evt->data.evt_gatt_server_user_read_request.connection,
      evt->data.evt_gatt_server_user_read_request.characteristic,
      SL_STATUS_OK, /* SUCCESS */
      sizeof(data), data, &data_len);
    app_assert_status(sc);
  }
}

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 ******************************************************************************/
void hand_signal_disconnect_event(sl_bt_msg_t *evt)
{
  (void)evt;

  notifications_enabled = false;
}
