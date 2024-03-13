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
#include "gatt_db.h"
#include "app.h"
#include "app_log.h"
#include "string.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint8_t connecting_handle = 0x00;
static bd_addr new_device_id = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

static const uint8_t serviceUUID[2] = { 0x09, 0x18 }; // HTM service UUID :
                                                      // 0x1809
static const uint8_t characteristicUUID[2] = { 0x1c, 0x2A }; // Temperature
                                                             // Measurement
                                                             //
                                                             // characteristics
                                                             // : 0x2A1C

static uint32_t serviceHandle = 0xFFFFFFFF;
static uint16_t characteristicHandle = 0xFFFF;
static bool connecting = false;
static bool enabling_indications = false;
static bool discovering_service = false;
static bool discovering_characteristic = false;

// number of active connections <= MAX_CONNECTIONS
static uint8_t numOfActiveConn = 0;
static uint8_t CONNECT_TIMEOUT_SEC = 10;
static const char string_central[] = "CENTRAL";
static const char string_peripheral[] = "PERIPHERAL";

#define MAX_CONNECTIONS    8
#define CONNECTION_TIMEOUT 1

sl_sleeptimer_timer_handle_t connection_timeout_timer;
sl_bt_gap_phy_type_t scanning_phy = sl_bt_gap_1m_phy;
sl_bt_gap_phy_type_t connection_phy = sl_bt_gap_1m_phy;
uint8_t dev_index;

typedef enum
{
  // Connection States (CS)
  CS_CONNECTED,
  CS_CONNECTING,
  CS_CLOSED
} conn_state_t;

/*------------CHANGE THE ABOVE ENUM USING THIS ONE-----------------*/
typedef enum {
  scanning,
  opening,
  discover_services,
  discover_characteristics,
  enable_indication,
  running
} conn_state;

/*----------------------------------------------------------------*/

typedef enum
{
  // Connection Roles (CR)
  CR_PERIPHERAL,
  CR_CENTRAL
} conn_role_t;

/* Struct to store the connecting device address, our device role in the
 * connection, and connection state*/
typedef struct
{
  bd_addr address;
  uint8_t address_type;
  conn_role_t conn_role;
  conn_state_t conn_state;
  uint8_t conn_handle;
} device_info_t;

/**************************************************************************//**
 *    Callback for the sleeptimer. Since this function is called from interrupt
 *   context,
 *    only an external signal is set, as no other BGAPI calls are allowed in
 *   this case.
 *****************************************************************************/
void sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)data;
  (void)handle;
  sl_bt_external_signal(CONNECTION_TIMEOUT);
}

const char * get_conn_state(uint8_t state)
{
  switch (state) {
    case CS_CONNECTED:
      return "CONNECTED";
    case CS_CONNECTING:
      return "CONNECTING";
    case CS_CLOSED:
      return "CLOSED";
    default:
      return "UNKNOWN";
  }
}

static device_info_t device_list[MAX_CONNECTIONS];

/* returns true if the remote device address is found in the list of connected
 *   device list */
bool found_device(bd_addr bd_address)
{
  int i;

  for (i = 0; i < numOfActiveConn; i++) {
    if (memcmp(&device_list[i].address, &bd_address, sizeof(bd_addr)) == 0) {
      return true; // Found
    }
  }

  return false; // Not found
}

static bool htm_service_found(
  struct sl_bt_evt_scanner_legacy_advertisement_report_s *pResp)
{
  // decoding advertising
  int i = 0, j;
  int adv_len;
  int adv_type;

  while (i < pResp->data.len - 1) {
    adv_len = pResp->data.data[i];
    adv_type = pResp->data.data[i + 1];

    /* type 0x02 = Incomplete List of 16-bit Service Class UUIDs
    * type 0x03 = Complete List of 16-bit Service Class UUIDs */
    if ((adv_type == 0x02) || (adv_type == 0x03)) {
      // Look through all the UUIDs looking for HTM service
      j = i + 2; // j holds the index of the first data
      do {
        if (!memcmp(serviceUUID, &(pResp->data.data[j]), sizeof(serviceUUID))) {
          return true;
        }
        j = j + 2;
      }
      while (j < i + adv_len);
    }
    i = i + adv_len + 1;
  }
  return false;
}

uint8_t get_dev_index(uint8_t handle)
{
  uint8_t index;

  for (index = 0; index < numOfActiveConn; index++) {
    if (device_list[index].conn_handle == handle) {
      return index;
    }
  }
  return 0xFF;
}

/* print bd_addr */
void print_bd_addr(bd_addr bd_address)
{
  int i;

  for (i = 5; i >= 0; i--) {
    app_log("%02X", bd_address.addr[i]);

    if (i > 0) {
      app_log(":");
    }
  }
}

void sl_app_log_stats(void)
{
  app_log("\r\n--------------- LIST of CONNECTED DEVICES ----------------\r\n");
  app_log("==========================================================\r\n");
  static bool print_header = true;

  // print header
  if (print_header == true) {
    app_log("ADDRESS            ROLE          HANDLE        STATE\r\n");
  }
  app_log("==========================================================\r\n");

  int i;
  for (i = 0; i < numOfActiveConn; i++) {
    print_bd_addr(device_list[i].address);
    app_log("  %-14s%-14d%-10s\r\n",
            (device_list[i].conn_role == 0) ? string_peripheral : string_central,
            device_list[i].conn_handle,
            get_conn_state(device_list[i].conn_state));
  }
  app_log("\r\n");
}

static void get_stack_version(sl_bt_msg_t *evt)
{
  app_log("Stack version: v%d.%d.%d-b%d\r\n",
          evt->data.evt_system_boot.major,
          evt->data.evt_system_boot.minor,
          evt->data.evt_system_boot.patch,
          evt->data.evt_system_boot.build);
}

static void get_system_id(void)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  // Extract unique ID from BT Address.
  sc = sl_bt_system_get_identity_address(&address, &address_type);
  app_assert_status(sc);

  // Pad and reverse unique ID to get System ID.
  system_id[0] = address.addr[5];
  system_id[1] = address.addr[4];
  system_id[2] = address.addr[3];
  system_id[3] = 0xFF;
  system_id[4] = 0xFE;
  system_id[5] = address.addr[2];
  system_id[6] = address.addr[1];
  system_id[7] = address.addr[0];

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                               0,
                                               sizeof(system_id),
                                               system_id);
  app_assert_status(sc);

  app_log("Local BT %s address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
          address_type ? "static random" : "public device",
          address.addr[5],
          address.addr[4],
          address.addr[3],
          address.addr[2],
          address.addr[1],
          address.addr[0]);
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
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

      app_log(
        "\r\n*** MULTIPLE CENTRAL MULTIPLE PERIPHERAL DUAL TOPOLOGY EXAMPLE ***\r\n\n");
      get_stack_version(evt);
      get_system_id();

      sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,
                                        20,
                                        10);
      app_assert_status(sc);

      // Start scanning
      sc = sl_bt_scanner_start(sl_bt_gap_1m_phy,
                               sl_bt_scanner_discover_generic);
      app_assert_status_f(sc, "Failed to start discovery #1\n");

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

      break;

    // -------------------------------
    // This event is generated when an advertisement packet or a scan response
    // is received from a peripheral device
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      /* Exit event if max connection is reached */
      if (numOfActiveConn == MAX_CONNECTIONS) {
        break;
      }

      /* Exit if device is in connection process (processing another scan
       * response),
       * or service, or characterstics discovery */
      if (connecting || enabling_indications || discovering_service
          || discovering_characteristic) {
        break;
      }

      /* Exit event if service is not in the scan response*/
      if (!htm_service_found(
            &(evt->data.evt_scanner_legacy_advertisement_report))) {
        break;
      }

      app_log("founded device\r\n");

      /* Exit event if the scan response is triggered by a device already in the
       * connection list. */
      if (found_device(
            evt->data.evt_scanner_legacy_advertisement_report.address)) {
        break;
      }

      /* Max connection isn't reached, device is not in a connection process,
       * new HTM service is found.
       * Continue ...*/

      /* Initiate connection */
      connecting = true;
      sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                 evt->data.evt_scanner_scan_report.address_type,
                                 connection_phy,
                                 &connecting_handle);
      app_assert_status(sc);

      /* Update device list. If connection doesn't succeed (due to timeout) the
       * device will be removed from the list in connection closed event
       * handler*/
      device_list[numOfActiveConn].address =
        evt->data.evt_scanner_scan_report.address;
      device_list[numOfActiveConn].address_type =
        evt->data.evt_scanner_scan_report.address_type;
      device_list[numOfActiveConn].conn_handle = connecting_handle;
      device_list[numOfActiveConn].conn_role = CR_PERIPHERAL; // connection role
                                                              // of the remote
                                                              // device
      device_list[numOfActiveConn].conn_state = CS_CONNECTING;

      /* Set connection timeout timer */
      sc = sl_sleeptimer_start_timer(&connection_timeout_timer,
                                     CONNECT_TIMEOUT_SEC * 32768,
                                     sleep_timer_callback,
                                     NULL,
                                     0,
                                     0);
      app_assert_status(sc);

      /* Increment numOfActiveConn */
      numOfActiveConn++;
      break;

    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          & CONNECTION_TIMEOUT) {
        /* Connection fail safe timer triggered, cancel connection procedure and
         * restart scanning/discovery */
        app_log("Connection timeout!\r\n");
        app_log("Cancel connection with device :");

        uint8_t dev_index;
        dev_index = get_dev_index(connecting_handle);
        print_bd_addr(device_list[dev_index].address);
        app_log("\r\n");
        app_log("Handle .......: #%d\r\n", connecting_handle);

        // CANCEL CONNECTION
        sc = sl_bt_connection_close(connecting_handle);
        app_assert_status(sc);
        connecting = false;
      }
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connecting ...\r\n");

      /* If connection role is CENTRAL ...*/
      if (evt->data.evt_connection_opened.master == CR_CENTRAL) {
        /* Cancel fail safe connection timer */
        sc = sl_sleeptimer_stop_timer(&connection_timeout_timer);
        app_assert_status(sc);
        app_log("Connection timeout is cleared.\r\n");

        /* Start discovering the remote GATT database */
        sc = sl_bt_gatt_discover_primary_services_by_uuid(
          evt->data.evt_connection_opened.connection,
          sizeof(serviceUUID),
          serviceUUID);
        app_assert_status(sc);

        discovering_service = true;

        /* connection process completed. */
        connecting = false;
      }

      /* else if connection role is PERIPHERAL ...*/
      else if (evt->data.evt_connection_opened.master == CR_PERIPHERAL) {
        /* update device list */
        device_list[numOfActiveConn].address =
          evt->data.evt_connection_opened.address;
        device_list[numOfActiveConn].address_type =
          evt->data.evt_connection_opened.address_type;
        device_list[numOfActiveConn].conn_handle =
          evt->data.evt_connection_opened.connection;
        device_list[numOfActiveConn].conn_role = CR_CENTRAL;      // connection
                                                                  //   role of
                                                                  //   the
                                                                  //   remote
                                                                  //   device

        /* Increment numOfActiveConn. */
        numOfActiveConn++;
      }

      // Update device connection state. common for both master and slave roles
      device_list[numOfActiveConn - 1].conn_state = CS_CONNECTING;

      /* Advertising stops when connection is opened. Re-start advertising */
      if (numOfActiveConn == MAX_CONNECTIONS) {
        app_log("Maximum number of allowed connections reached.\r\n");
        app_log(
          "Stop scanning but continue advertising in non-connectable mode.\r\n");
        sc = sl_bt_scanner_stop();
        app_assert_status(sc);

        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_legacy_advertiser_non_connectable);
        app_assert_status(sc);
      } else {
        // Max connection not reached. Re-start advertising in connectable mode
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_legacy_advertiser_connectable);
        app_assert_status(sc);
      }
      break;

    // This event ensures connection has been established.
    case sl_bt_evt_connection_parameters_id:
      dev_index = get_dev_index(evt->data.evt_connection_parameters.connection);
      device_list[dev_index].conn_state = CS_CONNECTED;

      /* If new connection is not reported ... */
      if (memcmp(&new_device_id, &device_list[dev_index].address,
                 sizeof(bd_addr)) != 0) {
        memcpy(&new_device_id, &device_list[dev_index].address,
               sizeof(bd_addr));
        app_log("\r\nNEW CONNECTION ESTABLISHED \r\n");
        app_log("Device ID .................: ");
        print_bd_addr(device_list[numOfActiveConn - 1].address);
        app_log("\r\n");
        app_log("Role ......................: %s\r\n",
                (device_list[dev_index].conn_role
                 == CR_PERIPHERAL) ? string_peripheral : string_central);
        app_log("Handle ....................: %d\r\n",
                device_list[dev_index].conn_handle);
        app_log("Number of connected devices: %d\r\n", numOfActiveConn);
        app_log("Available connections .....: %d\r\n",
                MAX_CONNECTIONS - numOfActiveConn);

        /* Print connection summary*/
        sl_app_log_stats();
      }
      break;

    case sl_bt_evt_gatt_service_id:
      /* save the service handle for the Health Thermometer service */
      serviceHandle = evt->data.evt_gatt_service.service;
      break;

    case sl_bt_evt_gatt_characteristic_id:
      /* save the characteristic handle for the Temperature Measurement
       *   characteristic */
      characteristicHandle = evt->data.evt_gatt_characteristic.characteristic;
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      /* if service discovery completed */
      if (discovering_service) {
        discovering_service = false;

        /* discover Temperature Measurement characteristic */
        app_log("success\r\n");
        sc = sl_bt_gatt_discover_characteristics_by_uuid(
          evt->data.evt_gatt_procedure_completed.connection,
          serviceHandle,
          sizeof(characteristicUUID),
          characteristicUUID);
        app_assert_status(sc);
        discovering_characteristic = true;
      }

      /* if characteristic discovery completed */
      else if (discovering_characteristic) {
        discovering_characteristic = false;

        /* enable indications on the Temperature Measurement characteristic */
        sc = sl_bt_gatt_set_characteristic_notification(
          evt->data.evt_gatt_procedure_completed.connection,
          characteristicHandle,
          sl_bt_gatt_indication);
        app_assert_status(sc);
        enabling_indications = true;
      } else if (enabling_indications) {
        enabling_indications = 0;
      }
      break;

    case sl_bt_evt_gatt_characteristic_value_id:
      /* if a temperature value was received from a slave... */
      if (evt->data.evt_gatt_characteristic_value.att_opcode
          == sl_bt_gatt_handle_value_indication) {
        uint16_t temp_measurement_char;

        switch ((evt->data.evt_gatt_characteristic_value.connection - 1) % 8) {
          case 0:
            temp_measurement_char = gattdb_temperature_measurement_0;
            break;
          case 1:
            temp_measurement_char = gattdb_temperature_measurement_1;
            break;
          case 2:
            temp_measurement_char = gattdb_temperature_measurement_2;
            break;
          case 3:
            temp_measurement_char = gattdb_temperature_measurement_3;
            break;
          case 4:
            temp_measurement_char = gattdb_temperature_measurement_4;
            break;
          case 5:
            temp_measurement_char = gattdb_temperature_measurement_5;
            break;
          case 6:
            temp_measurement_char = gattdb_temperature_measurement_6;
            break;
          case 7:
            temp_measurement_char = gattdb_temperature_measurement_7;
            break;
          default:
            temp_measurement_char = gattdb_temperature_measurement_0;
            break;
        }

        /* Acknowledge indication */
        sc = sl_bt_gatt_send_characteristic_confirmation(
          evt->data.evt_gatt_characteristic_value.connection);
        app_assert_status(sc);

        /* Send notifications or indications to all connected remote GATT
         * clients */
        sc = sl_bt_gatt_server_notify_all(temp_measurement_char,
                                          evt->data.evt_gatt_characteristic_value.value.len,
                                          evt->data.evt_gatt_characteristic_value.value.data);
        app_assert_status(sc);
      }
      break;
    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("\r\nCONNECTION CLOSED \r\n");
      // handle of the closed connection
      uint8_t closed_handle = evt->data.evt_connection_closed.connection;
      dev_index = get_dev_index(closed_handle);
      app_log("Device ");
      print_bd_addr(device_list[dev_index].address);
      app_log(" left the connection::0x%04X\r\n",
              evt->data.evt_connection_closed.reason);

      uint8_t i;
      for (i = dev_index; i < numOfActiveConn - 1; i++) {
        device_list[i] = device_list[i + 1];
      }

      if (numOfActiveConn > 0) {
        numOfActiveConn--;
      }

      // print list of remaining connections
      sl_app_log_stats();

      app_log("Number of active connections ...: %d\r\n", numOfActiveConn);
      app_log("Available connections ..........: %d\r\n",
              MAX_CONNECTIONS - numOfActiveConn);

      /* If we have one less available connection than the maximum allowed...*/
      if (numOfActiveConn == MAX_CONNECTIONS - 1) {
        // start scanning,
        sc = sl_bt_scanner_start(sl_bt_gap_1m_phy,
                                 sl_bt_scanner_discover_generic);
        app_assert_status_f(sc, "Failed to start discovery #1\n");
        app_log("Scanning restarted.\r\n");

        // and also start advertising as CONNECTABLE
        sc = sl_bt_legacy_advertiser_start(
          advertising_set_handle,
          sl_bt_legacy_advertiser_connectable);
        app_assert_status(sc);
        app_log("Advertising restarted in CONNECTABLE mode.\r\n");
      }

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
