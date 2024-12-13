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
#include <stdio.h>
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "user_config_nvm3.h"
#include "sl_simple_button_instances.h"
#include "people_counting_app.h"
#include "vl53l1x_app.h"
#include "oled.h"
#include "app.h"
#include "bthome_v2.h"

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

// -----------------------------------------------------------------------------
// Defines

// -------------------------------
// Advertising flags (common)

#define ADVERTISE_FLAGS_LENGTH                      2
#define ADVERTISE_FLAGS_TYPE                        0x01

/** Bit mask for flags advertising data type. */
#define ADVERTISE_FLAGS_LE_LIMITED_DISCOVERABLE     0x01
#define ADVERTISE_FLAGS_LE_GENERAL_DISCOVERABLE     0x02
#define ADVERTISE_FLAGS_BR_EDR_NOT_SUPPORTED        0x04

// -------------------------------
// Scan Response

#define ADVERTISE_MANDATORY_DATA_LENGTH             5
#define ADVERTISE_MANDATORY_DATA_TYPE_MANUFACTURER  0xFF

#define ADVERTISE_COMPANY_ID                        0x0047 /* Silicon Labs */
#define ADVERTISE_FIRMWARE_ID                       0x0000

/** Complete local name. */
#define ADVERTISE_TYPE_LOCAL_NAME                   0x09
#define ADVERTISE_DEVICE_NAME_LEN                   15
#define ADVERTISE_DEVICE_NAME                       "People Counting"

/** Helper macro */
#define UINT16_TO_BYTES(x) { (uint8_t)(x), (uint8_t)((x) >> 8) }

// -----------------------------------------------------------------------------
// Local variables
static uint8_t name[] = "BGM220P";
static uint8_t key[] = "11112222333344445555666677778888";

// -------------------------------
// Structure that holds Scan Response data

typedef struct {
  uint8_t flags_length;          /**< Length of the Flags field. */
  uint8_t flags_type;            /**< Type of the Flags field. */
  uint8_t flags;                 /**< Flags field. */
  uint8_t mandatory_data_length; /**< Length of the mandata field. */
  uint8_t mandatory_data_type;   /**< Type of the mandata field. */
  uint8_t company_id[2];         /**< Company ID. */
  uint8_t firmware_id[2];        /**< Firmware ID */
  uint8_t local_name_length;     /**< Length of the local name field. */
  uint8_t local_name_type;       /**< Type of the local name field. */
  uint8_t local_name[ADVERTISE_DEVICE_NAME_LEN]; /**< Local name field. */
} advertise_scan_response_t;

#define ADVERTISE_SCAN_RESPONSE_DEFAULT                                \
  {                                                                    \
    .flags_length = ADVERTISE_FLAGS_LENGTH,                            \
    .flags_type = ADVERTISE_FLAGS_TYPE,                                \
    .flags = ADVERTISE_FLAGS_LE_GENERAL_DISCOVERABLE                   \
             | ADVERTISE_FLAGS_BR_EDR_NOT_SUPPORTED,                   \
    .mandatory_data_length = ADVERTISE_MANDATORY_DATA_LENGTH,          \
    .mandatory_data_type = ADVERTISE_MANDATORY_DATA_TYPE_MANUFACTURER, \
    .company_id = UINT16_TO_BYTES(ADVERTISE_COMPANY_ID),               \
    .firmware_id = UINT16_TO_BYTES(ADVERTISE_FIRMWARE_ID),             \
    .local_name_length = ADVERTISE_DEVICE_NAME_LEN + 1,                \
    .local_name_type = ADVERTISE_TYPE_LOCAL_NAME,                      \
    .local_name = ADVERTISE_DEVICE_NAME                                \
  }

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static advertise_scan_response_t adv_scan_response
  = ADVERTISE_SCAN_RESPONSE_DEFAULT;

// The bonding handle allocated from Bluetooth stack.
static uint8_t ble_bonding_handle = 0xFF;

// -----------------------------------------------------------------------------
// Public function definitions

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
void app_init(void)
{
  app_log("============= Application Initialization =============\n");
  people_counting_app_init();
}

/***************************************************************************//**
 * Application Process Action.
 ******************************************************************************/
void app_process_action(void)
{
  // application code
}

static void bt_system_boot(void)
{
  sl_status_t sc;

  // Create an advertising set.
  sc = sl_bt_advertiser_create_set(&advertising_set_handle);
  app_assert_status(sc);

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

  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_scan_response_packet,
                                        sizeof(adv_scan_response),
                                        (uint8_t *)&adv_scan_response);
  app_assert_status(sc);

  // Start general advertising and enable connections.
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);
  app_log("[BLE]: Bluetooth Stack: Start advertising ...\n");

  // Maximum allowed bonding count: 8
  // New bonding will overwrite the bonding that was used the longest time ago
  sc = sl_bt_sm_store_bonding_configuration(8, 0x2);
  app_assert_status(sc);

  // Capabilities: No Input and No Output
  sc = sl_bt_sm_configure(0x8, sl_bt_sm_io_capability_noinputnooutput);
  app_assert_status(sc);

  // Allow bondings
  sc = sl_bt_sm_set_bondable_mode(1);
  app_assert_status(sc);

  bthome_v2_init(name, true, key, false);
}

// Service the connection opened event
static void connection_opened_handler(sl_bt_msg_t *evt)
{
  (void) evt;
  sl_status_t sc;

  app_log("[BLE]: Bluetooth Stack Event: CONNECTION OPENED\r\n");

  sc = sl_bt_advertiser_stop(advertising_set_handle);
  app_assert_status(sc);

#if ALWAYS_INCRASE_SECURITY
  active_connection_id = evt->data.evt_connection_opened.connection;
  if (ble_bonding_handle == 0xFF) {
    app_log("[BLE]: + Increasing security\r\n");

    sc = sl_bt_sm_increase_security(active_connection_id);
    app_assert_status(sc);
  } else {
    app_log("[BLE]: + Already Bonded (ID: %d)\r\n", ble_bonding_handle);
  }
#endif
}

// Service the connection closed event
static void connection_closed_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  // reset bonding handle variable to avoid deleting wrong bonding info
  ble_bonding_handle = 0xFF;

  app_log(
    "[BLE]: Bluetooth Stack Event : CONNECTION CLOSED (reason: 0x%04X)\r\n",
    evt->data.evt_connection_closed.reason);

  sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                             sl_bt_advertiser_general_discoverable);
  app_assert_status(sc);

  // Restart advertising after client has disconnected.
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);
  app_log("[BLE]: Bluetooth Stack: Start advertising ...\n");
}

#if ALWAYS_INCRASE_SECURITY
// Service the parameters event
static void connection_parameters_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t connection_handle = evt->data.evt_connection_parameters.connection;
  uint8_t security_level = evt->data.evt_connection_parameters.security_mode
                           + 1;
  uint16_t tx_size = evt->data.evt_connection_parameters.txsize;
  uint16_t timeout = evt->data.evt_connection_parameters.timeout;

  app_log("[BLE]: Bluetooth Stack Event : CONNECTION Parameters ID\r\n");

  // If security is less than 2 increase so devices can bond
  if (security_level == 0) {
    app_log("[BLE]: Bluetooth Stack Event : CONNECTION PARAMETERS : MTU = %d, \
              SecLvl : %d, timeout : %d\r\n",
            tx_size,
            security_level,
            timeout);
    app_log("[BLE]: + Bonding Handle is: 0x%04X\r\n", ble_bonding_handle);

    if (ble_bonding_handle == 0xFF) {
      app_log("[BLE]: + Increasing security.\r\n");

      sc = sl_bt_sm_increase_security(connection_handle);
      app_assert_status(sc);
      // start timer.
    } else {
      app_log("[BLE]: + Increasing security..\r\n");

      sc = sl_bt_sm_increase_security(connection_handle);
      app_assert_status(sc);
    }
  } else {
    app_log("[BLE]: Bluetooth Stack Event : [OK] CONNECTION PARAMETERS : \
              MTU = %d, SecLvl : %d, Timeout : %d\r\n",
            tx_size,
            security_level,
            timeout);
  }
}

#endif

// Service the security management bonding failed event
static void sm_bonding_failed_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  uint8_t connection_handle = evt->data.evt_sm_bonding_failed.connection;
  uint16_t reason = evt->data.evt_sm_bonding_failed.reason;

  app_log("[BLE]: Bluetooth Stack Event : BONDING FAILED (connection: %d, \
          reason: 0x%04X, bondingHandle: 0x%04X)\r\n",
          connection_handle,
          reason,
          ble_bonding_handle);

  if ((reason == SL_STATUS_BT_SMP_PASSKEY_ENTRY_FAILED)
      || (reason == SL_STATUS_TIMEOUT)) {
    app_log("[BLE]: + Increasing security... because reason is 0x%04x\r\n",
            reason);

    sc = sl_bt_sm_increase_security(connection_handle);
    app_assert_status(sc);
  } else if ((reason == SL_STATUS_BT_SMP_PAIRING_NOT_SUPPORTED)
             || (reason == SL_STATUS_BT_CTRL_PIN_OR_KEY_MISSING)) {
    if (ble_bonding_handle != 0xFF) {
      app_log("[BLE]: + Broken bond, deleting ID:%d...\r\n",
              ble_bonding_handle);

      sc = sl_bt_sm_delete_bonding(ble_bonding_handle);
      app_assert_status(sc);

      sc = sl_bt_sm_increase_security(
        evt->data.evt_connection_opened.connection);
      app_assert_status(sc);

      ble_bonding_handle = 0xFF;
    } else {
      app_log("[BLE]: + Increasing security in one second...\r\n");

      sc = sl_bt_sm_increase_security(connection_handle);
      app_log("[BLE]: Result... = 0x%04X\r\n", (int)sc);
      app_assert_status(sc);

      if (sc == SL_STATUS_INVALID_STATE) {
        app_log("[BLE]: + Trying to increase security again");

        sc = sl_bt_sm_increase_security(connection_handle);
        app_assert_status(sc);
      }
    }
  } else if (reason == SL_STATUS_BT_SMP_UNSPECIFIED_REASON) {
    app_log("[BLE]: + Increasing security... because reason is 0x0308\r\n");

    sc = sl_bt_sm_increase_security(connection_handle);
    app_assert_status(sc);
  } else {
    app_log("[BLE]: + Close connection : %d",
            evt->data.evt_sm_bonding_failed.connection);

    sc = sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);
    app_assert_status(sc);
  }
}

// Service the security management confirm bonding event
static void sm_confirm_bonding_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t connection_handle = evt->data.evt_sm_confirm_bonding.connection;

  app_log("[BLE]: Bluetooth Stack Event : CONFIRM BONDING\r\n");

  ble_bonding_handle = evt->data.evt_sm_confirm_bonding.bonding_handle;
  sc = sl_bt_sm_bonding_confirm(connection_handle, 1);
  app_assert_status(sc);
}

/***************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 ******************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  bthome_v2_bt_on_event(evt);

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      bt_system_boot();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection_opened_handler(evt);
      people_counting_set_bt_connection_handle(
        evt->data.evt_connection_opened.connection);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      connection_closed_handler(evt);
      people_counting_reset_bt_connection_handle();
      break;

#if ALWAYS_INCRASE_SECURITY
    // -------------------------------
    // The parameters event
    case sl_bt_evt_connection_parameters_id:
      connection_parameters_handler(evt);
      break;
#endif

    // -------------------------------
    // The confirm_bonding event
    case sl_bt_evt_sm_confirm_bonding_id:
      sm_confirm_bonding_handler(evt);
      break;

    // -------------------------------
    // This event triggered after the pairing or bonding procedure is
    // successfully completed.
    case sl_bt_evt_sm_bonded_id:
      ble_bonding_handle = evt->data.evt_sm_bonded.bonding;
      app_log("[BLE]: Bluetooth Stack Event : BONDED\r\n");
      break;

    // -------------------------------
    // This event is triggered if the pairing or bonding procedure fails.
    case sl_bt_evt_sm_bonding_failed_id:
      sm_bonding_failed_handler(evt);
      break;

    // -------------------------------
    // Handle configuration characteristics.
    case sl_bt_evt_gatt_server_attribute_value_id:
      break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      people_counting_process_evt_gatt_server_user_write_request(
        &(evt->data.evt_gatt_server_user_write_request));
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      people_counting_process_evt_gatt_server_user_read_request(
        &(evt->data.evt_gatt_server_user_read_request));
      break;

    // -------------------------------
    // When the remote device subscribes for notification,
    // start a timer to send out notifications periodically
    case sl_bt_evt_gatt_server_characteristic_status_id:
      people_counting_process_evt_gatt_server_characteristic_status(
        &(evt->data.evt_gatt_server_characteristic_status));
      break;

    case sl_bt_evt_system_external_signal_id:
      people_counting_process_evt_external_signal(
        evt->data.evt_system_external_signal.extsignals);

      if (evt->data.evt_system_external_signal.extsignals
          & PEOPLE_COUNTING_BTHOME_EVENT) {
        uint16_t people_count = vl53l1x_app_get_people_count();
        bthome_v2_reset_measurement();
        bthome_v2_add_measurement(ID_COUNT2, people_count);
        bthome_v2_send_packet();
        app_log("[BTHOME]: People count : %d\r\n", people_count);
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
