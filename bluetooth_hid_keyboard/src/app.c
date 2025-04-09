/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "app_assert.h"
#include "gatt_db.h"
#include "sl_simple_button_instances.h"

#define KEY_ARRAY_SIZE         25
#define MODIFIER_INDEX         0
#define DATA_INDEX             2

#define CAPSLOCK_KEY_OFF       0x00
#define CAPSLOCK_KEY_ON        0x02

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint8_t notification_enabled = 0;

static uint8_t input_report_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t actual_key;
static uint8_t counter = 0;

static const uint8_t reduced_key_array[] = {
  0x04, /* a */
  0x05,   /* b */
  0x06,   /* c */
  0x07,   /* d */
  0x08,   /* e */
  0x09,   /* f */
  0x0a,   /* g */
  0x0b,   /* h */
  0x0c,   /* i */
  0x0d,   /* j */
  0x0e,   /* k */
  0x0f,   /* l */
  0x10,   /* m */
  0x11,   /* n */
  0x12,   /* o */
  0x13,   /* p */
  0x14,   /* q */
  0x15,   /* r */
  0x16,   /* s */
  0x17,   /* t */
  0x18,   /* u */
  0x19,   /* v */
  0x1a,   /* w */
  0x1b,   /* x */
  0x1c,   /* y */
  0x1d,   /* z */
};

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      app_log("boot event - starting advertising\r\n");

      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("connection opened\r\n");
      sc =
        sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed, reason: 0x%2.2x\r\n",
              evt->data.evt_connection_closed.reason);
      notification_enabled = 0;
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_sm_bonded_id:
      app_log("successful bonding\r\n");
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      app_log("bonding failed, reason 0x%2X\r\n",
              evt->data.evt_sm_bonding_failed.reason);

      /* Previous bond is broken, delete it and close connection,
       *  host must retry at least once */
      sc = sl_bt_sm_delete_bondings();
      app_assert_status(sc);
      sc = sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);
      app_assert_status(sc);
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_report) {
        // client characteristic configuration changed by remote GATT client
        if (evt->data.evt_gatt_server_characteristic_status.status_flags
            == sl_bt_gatt_server_client_config) {
          if (evt->data.evt_gatt_server_characteristic_status.
              client_config_flags == sl_bt_gatt_server_notification) {
            notification_enabled = 1;
          } else {
            notification_enabled = 0;
          }
        }
      }
      break;
    case  sl_bt_evt_system_external_signal_id:
      if (notification_enabled == 1) {
        memset(input_report_data, 0, sizeof(input_report_data));

        input_report_data[MODIFIER_INDEX] = CAPSLOCK_KEY_OFF;
        input_report_data[DATA_INDEX] = actual_key;

        sc = sl_bt_gatt_server_notify_all(gattdb_report,
                                          sizeof(input_report_data),
                                          input_report_data);
        app_assert_status(sc);

        app_log("Key report was sent\r\n");
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  if (&sl_button_btn0 == handle) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
      actual_key = reduced_key_array[counter];
      app_log("Button pushed - callback\r\n");
    } else {
      if (KEY_ARRAY_SIZE == counter) {
        counter = 0;
      } else {
        counter++;
      }

      actual_key = 0;
      app_log("Button released - callback \r\n");
    }
  }

  sl_bt_external_signal(1);
}
