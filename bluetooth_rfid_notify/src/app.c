/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sparkfun_rfid_id12la.h"
#include "gatt_db.h"
#include "sl_i2cspm_instances.h"
#include "app_assert.h"
#include "sl_sleeptimer.h"

#define READING_INTERVAL_MSEC 10
#define CARD_ID_SIZE          6

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static volatile bool app_timer_expire = false;
static id12la_tag_list_t id12la_all_tag;
static uint8_t count_tag = 0;

static sl_sleeptimer_timer_handle_t app_timer;
static void app_timer_cb(sl_sleeptimer_timer_handle_t *timer, void *data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  app_log("=== Bluetooth LE RFID Notify Application ===\n");
  app_log("======== Application Initialization ========\n");

  sc = sparkfun_id12la_init(sl_i2cspm_rfid);
  if (sc == SL_STATUS_OK) {
    app_log("[RFID]: RFID inits successfully, ready scans some tags\n");
  } else {
    app_log("[RFID]: I2C address has been changed before\n");
    sc = sparkfun_id12la_scan_address();
    if (sc == SL_STATUS_OK) {
      app_log("[RFID]: I2C address is: 0x%02X\n",
              sparkfun_id12la_get_i2c_address());
      app_log("[RFID]: RFID begins successfully, ready to scan some tags\n");
    } else {
      app_log("[RFID]: I2C address scanning failed.\n");
    }
  }

  sl_sleeptimer_restart_periodic_timer_ms(&app_timer,
                                          READING_INTERVAL_MSEC,
                                          app_timer_cb,
                                          NULL,
                                          0,
                                          0);
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
  sl_status_t sc;

  if (app_timer_expire) {
    sc = sparkfun_id12la_get_all_tag(&id12la_all_tag, &count_tag);
    if (sc != SL_STATUS_OK) {
      app_log("[Error]: Scanning tags, check the connection.\n");
    }
    if (count_tag > 0) {
      app_log("[RFID]: Count tag: %d\n", count_tag);
      for (uint8_t i = 0; i < count_tag; i++) {
        if (id12la_all_tag.id12la_data[i].checksum_valid) {
          app_log("[RFID]: Read a new card.\n");
          app_log(
            "[RFID]: Card ID: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
            id12la_all_tag.id12la_data[i].id_tag[0], \
            id12la_all_tag.id12la_data[i].id_tag[1], \
            id12la_all_tag.id12la_data[i].id_tag[2], \
            id12la_all_tag.id12la_data[i].id_tag[3], \
            id12la_all_tag.id12la_data[i].id_tag[4], \
            id12la_all_tag.id12la_data[i].id_tag[5]);
          sc = sl_bt_gatt_server_notify_all(gattdb_card_uid,
                                            CARD_ID_SIZE,
                                            (const uint8_t *)id12la_all_tag. \
                                            id12la_data[i].id_tag);
          app_log("[RFID]: Ready to scan other cards.\n");
        } else {
          app_log("[RFID]: Checksum error!\n");
        }
      }
    }
    app_timer_expire = false;
  }
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
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to create advertising set\n",
                 (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_log("[BLE]: Start advertising ...\n");
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising\n",
                 (int)sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("[BLE]: Connection opened.\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("[BLE]: Connection closed.\n");
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to create advertising set\n",
                 (int)sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_log("[BLE]: Start advertising ...\n");
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising\n",
                 (int)sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static void app_timer_cb(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void)timer;
  (void)data;

  app_timer_expire = true;
}
