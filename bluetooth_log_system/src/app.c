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
#include "sl_common.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"

#include "app_assert.h"

#include "app.h"
#include "log.h"

/* System */
#define SYSTEM_BOOT_EVT                     SURROUNDING("Boot")
#define EXSYSTEM_TERNAL_SIGNAL_EVT          SURROUNDING("External signal")

/* Bluetooth LE Connection */
#define LE_CONNECTION_OPEN_EVT              SURROUNDING("Opened")
#define LE_CONNECTION_CLOSED_EVT            SURROUNDING("Closed")
#define LE_CONNECTION_UPDATE_EVT            SURROUNDING("Parameters Updated")
#define LE_CONNECTION_RSSI                  SURROUNDING("RSSI")

/* Gatt Server */
#define GATT_SERVER_CHARACTERISTIC_STATUS   SURROUNDING("Characteristic Status")
#define GATT_SERVER_ATT_VALUE               SURROUNDING("Attribute Value")
#define GATT_SERVER_READ_REQUEST            SURROUNDING("Read Request")

/* DTM */
#define TEST_DTM_COMPLETED                  SURROUNDING("DTM Completed")

#define COMMANDS_NOT_ADDED                  SURROUNDING("Command not added")

#define NO_INFO                             ""
#define FOLLOWINGS                          "   |-->>"

static void log_out(uint8_t direct, const char *msg, sl_status_t sc);

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

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

  log_events(evt);

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      LOGE("This is an ERROR message\r\n");
      LOGW("This is a WARNING message\r\n");
      LOGD("This is a DEBUG message\r\n");
      LOGV("This is a VERBOSE message\r\n");
      LOGI("This is an INFORMATION message\r\n\r\n");

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
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      LOGD("Starting advertising\r\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      LOGD("Connection opened\r\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      LOGD("Connection closed, reason: 0x%2.2x\r\n",
           evt->data.evt_connection_closed.reason);
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
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

void log_events(sl_bt_msg_t *evt)
{
  /* Handle events */
  switch (SL_BT_MSG_ID(evt->header)) {
#if (SYSTEM == 1)
#undef EVT_CATEGORY
#define EVT_CATEGORY  "[SYSTEM]: "
    case sl_bt_evt_system_boot_id:
      EVT_LOG_C(SYSTEM_BOOT_EVT, NO_INFO);
      LOGN();
      EVT_LOG_V(FOLLOWINGS,
                "Major = 0x%04x, Minor = 0x%04x, Patch = 0x%04x, Build = %d, Bootloader = 0x%08lx, Hw = 0x%04x, Hash = 0x%08lx",
                evt->data.evt_system_boot.major,
                evt->data.evt_system_boot.minor,
                evt->data.evt_system_boot.patch,
                evt->data.evt_system_boot.build,
                evt->data.evt_system_boot.bootloader,
                evt->data.evt_system_boot.hw,
                evt->data.evt_system_boot.hash);
      LOGN();
      break;

    case sl_bt_evt_system_external_signal_id:
      EVT_LOG_I(EXSYSTEM_TERNAL_SIGNAL_EVT, "External signals = 0x%08lX",
                evt->data.evt_system_external_signal.extsignals);
      LOGN();
      break;
#endif

#if (LE_CONNECTION == 1)
#undef EVT_CATEGORY
#define EVT_CATEGORY  "[LE_CONNECTION]: "
    case sl_bt_evt_connection_opened_id:
      EVT_LOG_C(LE_CONNECTION_OPEN_EVT, "Connection Handle = 0x%02x",
                evt->data.evt_connection_opened.connection);
      LOGN();
      EVT_LOG_I(FOLLOWINGS, "Peer address = ");
      UINT8_ARRAY_DUMP(evt->data.evt_connection_opened.address.addr, 6);
      LOGN();
      EVT_LOG_V(FOLLOWINGS,
                "Role = %s, Bonding handle = %d, advertiser = %d",
                evt->data.evt_connection_opened.role == sl_bt_connection_role_central ? "Master" : "Slave",
                evt->data.evt_connection_opened.bonding,
                evt->data.evt_connection_opened.advertiser);
      LOGN();
      break;
    case sl_bt_evt_connection_closed_id:
      EVT_LOG_C(LE_CONNECTION_CLOSED_EVT, "Handle = 0x%02x, Reason = ",
                evt->data.evt_connection_closed.connection);
      error_checking(evt->data.evt_connection_closed.reason, 1);
      LOGN();
      break;
    case sl_bt_evt_connection_parameters_id:
      // Why float can't be printed while "print float" has been checked????
      EVT_LOG_I(LE_CONNECTION_UPDATE_EVT,
                "Connection Handle = 0x%02x, Interval = %d*1.25ms, Latency = %d, Timeout = %dms",
                evt->data.evt_connection_parameters.connection,
                evt->data.evt_connection_parameters.interval,
                evt->data.evt_connection_parameters.latency,
                evt->data.evt_connection_parameters.timeout * 10);

      /* TODO: There are still 2 parameters may need to be handled */
      LOGN();
      break;
    case sl_bt_evt_connection_phy_status_id:
      break;
#endif

#if (GATT_SERVER == 1)
#undef EVT_CATEGORY
#define EVT_CATEGORY  "[GATT_SERVER]: "
    case sl_bt_evt_gatt_server_characteristic_status_id:
      EVT_LOG_I(GATT_SERVER_CHARACTERISTIC_STATUS,
                "Connection Handle = 0x%02x, Characteristic = 0x%04x, Type = %s, Value = 0x%04x",
                evt->data.evt_gatt_server_characteristic_status.connection,
                evt->data.evt_gatt_server_characteristic_status.characteristic,
                evt->data.evt_gatt_server_characteristic_status.status_flags == 1 ? "Gatt server client config" : "Confirmation",
                evt->data.evt_gatt_server_characteristic_status.client_config_flags);
      LOGN();
      break;
    case sl_bt_evt_gatt_server_attribute_value_id:
      EVT_LOG_I(GATT_SERVER_ATT_VALUE,
                "Connection Handle = 0x%02x, Attribute = 0x%04x, Op_Code = 0x%02x, Offset = 0x%04x, Value = ",
                evt->data.evt_gatt_server_attribute_value.connection,
                evt->data.evt_gatt_server_attribute_value.attribute,
                evt->data.evt_gatt_server_attribute_value.att_opcode,
                evt->data.evt_gatt_server_attribute_value.offset);
      UINT8_ARRAY_DUMP(evt->data.evt_gatt_server_attribute_value.value.data,
                       evt->data.evt_gatt_server_attribute_value.value.len);
      LOGN();
      break;
    case sl_bt_evt_gatt_server_execute_write_completed_id:
      break;
    case sl_bt_evt_gatt_server_user_read_request_id:
      EVT_LOG_I(GATT_SERVER_READ_REQUEST,
                "Connection Handle = 0x%02x, Characteristic = 0x%04x, Op_Code = 0x%02x, Offset = 0x%04x",
                evt->data.evt_gatt_server_user_read_request.connection,
                evt->data.evt_gatt_server_user_read_request.characteristic,
                evt->data.evt_gatt_server_user_read_request.att_opcode,
                evt->data.evt_gatt_server_user_read_request.offset);
      LOGN();
      break;
    case sl_bt_evt_gatt_server_user_write_request_id:
      EVT_LOG_I(GATT_SERVER_ATT_VALUE,
                "Connection Handle = 0x%02x, Characteristic = 0x%04x, Op_Code = 0x%02x, Offset = 0x%04x, Value = ",
                evt->data.evt_gatt_server_user_write_request.connection,
                evt->data.evt_gatt_server_user_write_request.characteristic,
                evt->data.evt_gatt_server_user_write_request.att_opcode,
                evt->data.evt_gatt_server_user_write_request.offset);
      UINT8_ARRAY_DUMP(evt->data.evt_gatt_server_user_write_request.value.data,
                       evt->data.evt_gatt_server_user_write_request.value.len);
      LOGN();
      break;
#endif

#if (HARDWARE == 1)
#undef EVT_CATEGORY
#define EVT_CATEGORY  "[HARDWARE]: "
    case sl_bt_evt_system_soft_timer_id:
      // TODO
      break;
#endif

#if (TEST == 1)
#undef EVT_CATEGORY
#define EVT_CATEGORY  "[TEST]: "
    case sl_bt_evt_test_dtm_completed_id:
      EVT_LOG_I(TEST_DTM_COMPLETED, "Result = ");
      error_checking(evt->data.evt_test_dtm_completed.result, 1);
      LOGN();
      EVT_LOG_I(FOLLOWINGS,
                "Number of Packets = %d",
                evt->data.evt_test_dtm_completed.number_of_packets);
      LOGN();
      break;
#endif

    default:
      EVT_LOG_V(COMMANDS_NOT_ADDED,
                "Header = 0x%08lx",
                SL_BT_MSG_ID(evt->header));
      LOGN();
      break;
  }
}

static void log_out(uint8_t direct, const char *msg, sl_status_t sc)
{
  if (direct) {
    LOG_DIRECT_ERR("0x%04lX - %s", sc, msg);
  } else {
    LOGE("0x%04lX - %s", sc, msg);
    LOGN();
  }
}

sl_status_t error_checking(sl_status_t sc, uint8_t directly)
{
  if (sc == SL_STATUS_OK) {
    if (directly) {
      LOG("Success");
    }
    return sc;
  }

  switch (sc) {
    case SL_STATUS_BT_PS_STORE_FULL:
      log_out(directly, "Flash reserved for PS store is full", sc);
      break;
    case SL_STATUS_BT_PS_KEY_NOT_FOUND:
      log_out(directly, "PS key not found", sc);
      break;
    case SL_STATUS_INVALID_HANDLE:
      log_out(directly, "Invalid handle", sc);
      break;
    case SL_STATUS_BT_CTRL_CONNECTION_TIMEOUT:
      log_out(directly, "Link supervision timeout has expired", sc);
      break;
    case SL_STATUS_BT_CTRL_INVALID_COMMAND_PARAMETERS:
      log_out(directly, "Command contained invalid parameter", sc);
      break;
    case SL_STATUS_BT_L2CAP_WRONG_STATE:
      log_out(directly, "Device is in wrong state to receive command", sc);
      break;
    case SL_STATUS_BT_CTRL_MEMORY_CAPACITY_EXCEEDED:
      log_out(directly, "Controller is out of memory", sc);
      break;
    case SL_STATUS_BT_CTRL_REMOTE_USER_TERMINATED:
      log_out(directly,
              "User on the remote device terminated the connection",
              sc);
      break;
    case
      SL_STATUS_BT_CTRL_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES
      :
      log_out(directly,
              "The remote device terminated the connection because of low resources",
              sc);
      break;
    case SL_STATUS_BT_CTRL_REMOTE_POWERING_OFF:
      log_out(directly,
              "Remote Device Terminated Connection due to Power Off",
              sc);
      break;
    case SL_STATUS_BT_CTRL_CONNECTION_TERMINATED_BY_LOCAL_HOST:
      log_out(directly, "Local device terminated the connection", sc);
      break;
    case SL_STATUS_BT_CTRL_PAIRING_NOT_ALLOWED:
      log_out(directly, "The device does not allow pairing", sc);
      break;
    case SL_STATUS_BT_CTRL_LL_RESPONSE_TIMEOUT:
      log_out(directly,
              "Connection terminated due to link-layer procedure timeout",
              sc);
      break;
    case SL_STATUS_BT_ATT_INVALID_HANDLE:
      log_out(directly,
              "The attribute handle given was not valid on this server",
              sc);
      break;
    case SL_STATUS_BT_ATT_READ_NOT_PERMITTED:
      log_out(directly, "The attribute cannot be read", sc);
      break;
    case SL_STATUS_BT_ATT_WRITE_NOT_PERMITTED:
      log_out(directly, "The attribute cannot be written", sc);
      break;
    case SL_STATUS_BT_ATT_INSUFFICIENT_AUTHENTICATION:
      log_out(directly,
              "The attribute requires authentication before it can be read or written",
              sc);
      break;
    case SL_STATUS_BT_ATT_INVALID_OFFSET:
      log_out(directly,
              "Offset specified was past the end of the attribute",
              sc);
      break;
    case SL_STATUS_BT_ATT_ATT_NOT_FOUND:
      log_out(directly,
              "No attribute found within the given attribute handle range",
              sc);
      break;
    default:
      log_out(directly, "Error needs to be added", sc);
      LOG("Error code = 0x%04X", (int)sc);
      break;
  }
  return sc;
}
