/***************************************************************************//**
 * @file app_ble_events.c
 * @brief Application BLE Events Source File
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

#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"

#include "app_config.h"
#include "app_ble_events.h"
#include "app_logic.h"
#include "app_events.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// The bonding set handle allocated from Bluetooth stack.
static uint8_t ble_bonding_handle = 0xff;

// BLE Stack Internal Event handlers.
static void ble_connection_parameters_handler(sl_bt_msg_t *evt);
static void ble_connection_opened_handler(sl_bt_msg_t *evt);
static void ble_connection_closed_handler(sl_bt_msg_t *evt);
static void ble_sm_bonding_failed_handler(sl_bt_msg_t *evt);
static void ble_sm_bonding_confirm_handler(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      // Print Bluetooth address
      app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
              address_type ? "static random" : "public device",
              address.addr[5],
              address.addr[4],
              address.addr[3],
              address.addr[2],
              address.addr[1],
              address.addr[0]);

      // Pad and reverse unique ID to get System ID.
      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id, 0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert_status(sc);

      ////////////////////////////////////////////////////////////////////////
      // Bonding                                                            //
      ////////////////////////////////////////////////////////////////////////

      sc = sl_bt_sm_configure(0x0B, sl_bt_sm_io_capability_displayonly);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to configure security\r\n", (int )sc);

      sc = sl_bt_sm_store_bonding_configuration(1, 2);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set the maximum allowed bonding count \
	                 and bonding policy\r\n",
                 (int )sc);

      sc = sl_bt_sm_set_passkey(DISTANCE_MONITOR_PASSKEY);
      app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to set passkey\r\n",
                 (int )sc);

      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set bondable mode\r\n", (int )sc);

      ////////////////////////////////////////////////////////////////////////

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
      // Start general advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("Start advertising...\n");
      break;

    ////////////////////////////////////////////////////////////////////////
    // Application specific handlers                                      //
    ////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      ble_connection_opened_handler(evt);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      ble_connection_closed_handler(evt);
      break;

    // -------------------------------
    // The parameters event
    case sl_bt_evt_connection_parameters_id:
      ble_connection_parameters_handler(evt);
      break;

    // -------------------------------
    // The confirm_bonding event
    case sl_bt_evt_sm_confirm_bonding_id:
      ble_sm_bonding_confirm_handler(evt);
      break;

    // -------------------------------
    // This event triggered after the pairing or bonding procedure is
    // successfully completed.
    case sl_bt_evt_sm_bonded_id:
      app_log("BLE Stack Event : BONDED\r\n");
      break;

    // -------------------------------
    // This event is triggered if the pairing or bonding procedure fails.
    case sl_bt_evt_sm_bonding_failed_id:
      ble_sm_bonding_failed_handler(evt);
      break;

    // -------------------------------
    // This event is triggered if client is attempting to read a value of an
    // attribute of the local GATT database.
    case sl_bt_evt_gatt_server_user_read_request_id:
      app_event_handler_on_char_requests(BLE_CHAR_ACCESS_TYPE_READ, evt);
      break;

    // -------------------------------
    // This event is triggered if client is attempting to write a value of an
    // attribute into the local GATT database.
    case sl_bt_evt_gatt_server_user_write_request_id:
      app_event_handler_on_char_requests(BLE_CHAR_ACCESS_TYPE_WRITE, evt);
      break;

    // -------------------------------
    // This event is triggered if the value of an attribute in the local GATT
    // database was changed by a remote GATT client
    case sl_bt_evt_gatt_server_attribute_value_id:
      break;

    // External event indication(raised from interrupt and timer handlers)
    case sl_bt_evt_system_external_signal_id:
      app_event_handler_on_external_event(evt);
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

////////////////////////////////////////////////////////////////////////
// BLE Event Handlers                                                 //
////////////////////////////////////////////////////////////////////////

// Connection
static void ble_connection_opened_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t active_connection_id = 0;

  app_log("BLE Stack Event : CONNECTION OPENED\r\n");

  // Stop advertisement
  sc = sl_bt_advertiser_stop(advertising_set_handle);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to stop the advertising\r\n", (int )sc);

  active_connection_id = evt->data.evt_connection_opened.connection;
  ble_bonding_handle = evt->data.evt_connection_opened.bonding;

  if (ble_bonding_handle == 0xFF) {
    app_log("+ Increasing security\r\n");
    sc = sl_bt_sm_increase_security(active_connection_id);
    if (sc == SL_STATUS_INVALID_STATE) {
      app_log("+ Trying to increase security again");

      sc = sl_bt_sm_increase_security(active_connection_id);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to enhance the security\r\n", (int )sc);
    }
  } else {
    app_log("+ Already Bonded (ID: %d)\r\n", ble_bonding_handle);
  }
}

static void ble_connection_closed_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  // reset bonding handle variable to avoid deleting wrong bonding info
  ble_bonding_handle = 0xff;

  app_log("BLE Stack Event : CONNECTION CLOSED (reason: 0x%04X)\r\n",
          evt->data.evt_connection_closed.reason);

  // Restart advertising after client has disconnected.
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     advertiser_connectable_scannable);

  app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to start advertising\r\n",
             (int )sc);
}

static void ble_connection_parameters_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t connection_handle = evt->data.evt_connection_parameters.connection;
  uint8_t security_level = evt->data.evt_connection_parameters.security_mode
                           + 1;
  uint16_t tx_size = evt->data.evt_connection_parameters.txsize;
  uint16_t timeout = evt->data.evt_connection_parameters.timeout;

  app_log("BLE Stack Event : CONNECTION Parameters ID\r\n");

  // If security is less than 2 increase so devices can bond
  if (security_level <= 2) {
    app_log(
      "BLE Stack Event : CONNECTION PARAMETERS : MTU = %d, \
	              SecLvl : %d, timeout : %d\r\n",
      tx_size,
      security_level,
      timeout);
    app_log("+ Bonding Handle is: 0x%04X\r\n", ble_bonding_handle);

    if (0xff == ble_bonding_handle) {
      app_log("+ Increasing security.\r\n");

      sc = sl_bt_sm_increase_security(connection_handle);
      if (sc != SL_STATUS_OK) {
        app_log("[E: 0x%04x] Failed to enhance the security\r\n",
                (unsigned int)sc);
      }
      // start timer.
    } else {
      app_log("+ Increasing security..\r\n");

      sc = sl_bt_sm_increase_security(connection_handle);
      if (sc != SL_STATUS_OK) {
        app_log("[E: 0x%04x] Failed to enhance the security\r\n",
                (unsigned int)sc);
      }
    }
  } else {
    app_log(
      "[OK]      BLE Stack Event : CONNECTION PARAMETERS : \
	              MTU = %d, SecLvl : %d, Timeout : %d\r\n",
      tx_size,
      security_level,
      timeout);
  }
}

// Bonding
static void ble_sm_bonding_failed_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  uint8_t connection_handle = evt->data.evt_sm_bonding_failed.connection;
  uint16_t reason = evt->data.evt_sm_bonding_failed.reason;

  app_log(
    "BLE Stack Event : BONDING FAILED (connection: %d, \
	          reason: 0x%04X, bondingHandle: 0x%04X)\r\n",
    connection_handle,
    reason,
    ble_bonding_handle);

  switch (reason) {
    case SL_STATUS_BT_SMP_PASSKEY_ENTRY_FAILED:
    case SL_STATUS_TIMEOUT:
      app_log("+ Increasing security... because reason is 0x%04x\r\n", reason);

      sc = sl_bt_sm_increase_security(connection_handle);
      if (sc != SL_STATUS_OK) {
        app_log("[E: 0x%04x] Failed to enhance the security\r\n", (int )sc);
      }
      break;

    case SL_STATUS_BT_SMP_PAIRING_NOT_SUPPORTED:
    case SL_STATUS_BT_CTRL_PIN_OR_KEY_MISSING:
      if (0xff != ble_bonding_handle) {
        app_log("+ Broken bond, deleting ID:%d...\r\n", ble_bonding_handle);

        sc = sl_bt_sm_delete_bonding(ble_bonding_handle);
        app_assert(sc == SL_STATUS_OK,
                   "[E: 0x%04x] Failed to delete specified bonding \
		                     information or whitelist\r\n",
                   (int )sc);

        sc = sl_bt_sm_increase_security(
          evt->data.evt_connection_opened.connection);
        if (sc != SL_STATUS_OK) {
          app_log("+ Trying to increase security again");

          sc = sl_bt_sm_increase_security(connection_handle);
          app_assert(sc == SL_STATUS_OK,
                     "[E: 0x%04x] Failed to enhance the security\r\n",
                     (int )sc);
        }

        ble_bonding_handle = 0xff;
      } else {
        app_log("+ Increasing security in one second...\r\n");

        sc = sl_bt_sm_increase_security(connection_handle);
        app_log("Result... = 0x%04X\r\n", (unsigned int)sc);

        if (sc == SL_STATUS_INVALID_STATE) {
          app_log("+ Trying to increase security again");

          sc = sl_bt_sm_increase_security(connection_handle);
          if (sc != SL_STATUS_OK) {
            app_log("[E: 0x%04x] Failed to enhance the security\r\n", (int )sc);
          }
        }
      }
      break;

    case SL_STATUS_BT_SMP_UNSPECIFIED_REASON:
      app_log("+ Increasing security... because reason is 0x0308\r\n");

      sc = sl_bt_sm_increase_security(connection_handle);
      if (sc != SL_STATUS_OK) {
        app_log("+ Trying to increase security again");

        sc = sl_bt_sm_increase_security(connection_handle);
        if (sc != SL_STATUS_OK) {
          app_log("[E: 0x%04x] Failed to enhance the security\r\n", (int )sc);
        }
      }
      break;

    default:
      app_log("+ Close connection : %d",
              evt->data.evt_sm_bonding_failed.connection);

      sc = sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to close connection\r\n",
                 (int )sc);
      break;
  }
}

static void ble_sm_bonding_confirm_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t connection_handle = evt->data.evt_connection_parameters.connection;

  app_log("BLE Stack Event : CONFIRM BONDING\r\n");

  sc = sl_bt_sm_bonding_confirm(connection_handle, 1);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to confirm bonding requests\r\n", (int )sc);
}
