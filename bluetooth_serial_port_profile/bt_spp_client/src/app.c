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
#include "sl_iostream_handles.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "app_log.h"
#include "app_assert.h"

/*******************************************************************************
 *    Local Macros and Definitions
 ******************************************************************************/

/* Set here the operation mode of the SPP device: */
#define SPP_SERVER_MODE    0
#define SPP_CLIENT_MODE    1

#define SPP_OPERATION_MODE SPP_CLIENT_MODE

/*Main states */
#define DISCONNECTED       0
#define SCANNING           1
#define FIND_SERVICE       2
#define FIND_CHAR          3
#define ENABLE_NOTIF       4
#define DATA_MODE          5
#define DISCONNECTING      6

#define STATE_ADVERTISING  1
#define STATE_CONNECTED    2
#define STATE_SPP_MODE     3

// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907
const uint8_t serviceUUID[16] = { 0x07,
                                  0xb9,
                                  0xf9,
                                  0xd7,
                                  0x50,
                                  0xa4,
                                  0x20,
                                  0x89,
                                  0x77,
                                  0x40,
                                  0xcb,
                                  0xfd,
                                  0x2c,
                                  0xc1,
                                  0x80,
                                  0x48 };

// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
const uint8_t charUUID[16] = { 0xd6,
                               0x58,
                               0xd6,
                               0x21,
                               0xbc,
                               0x55,
                               0x81,
                               0x9f,
                               0x42,
                               0x44,
                               0x71,
                               0x6d,
                               0xc4,
                               0x6e,
                               0xc2,
                               0xfe };

/* maximum number of iterations when polling UART RX data before sending data
 *   over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease
 *   throughput */
#define UART_POLL_TIMEOUT  5000

/*Bookkeeping struct for storing amount of received/sent data  */
typedef struct
{
  uint32_t num_pack_sent;
  uint32_t num_bytes_sent;
  uint32_t num_pack_received;
  uint32_t num_bytes_received;
  uint32_t num_writes; /* Total number of send attempts */
} ts_counters;

/* Function is only used in SPP client mode */
#if (SPP_OPERATION_MODE == SPP_CLIENT_MODE)
static bool process_scan_response(
  sl_bt_evt_scanner_legacy_advertisement_report_t *pResp);

#endif

/* Common local functions*/
static void print_stats(ts_counters *p_counters);
static void reset_variables();
static void send_spp_data();

/*******************************************************************************
 *    Local Variables
 ******************************************************************************/
static uint8_t conn_handle = 0xFF;
static uint8_t main_state;
static uint32_t service_handle;
static uint16_t char_handle;

ts_counters counters;

// Default maximum packet size is 20 bytes. This is adjusted after connection is
// opened based on the connection parameters
static uint8_t max_packet_size = 20;
static uint8_t min_packet_size = 20;  // Target minimum bytes for one packet

static void reset_variables()
{
  conn_handle = 0xFF;
  main_state = STATE_ADVERTISING;
  service_handle = 0;
  char_handle = 0;
  max_packet_size = 20;

  memset(&counters, 0, sizeof(counters));
}

/**************************************************************************
 *    Application Init code
 ***************************************************************************/
void app_init(void)
{
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  if (STATE_SPP_MODE == main_state) {
    send_spp_data();
  }
}

/**************************************************************************//**
 * Bluetooth stack event handler for SPP Client mode
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  uint16_t max_mtu_out;
  sl_status_t status;

  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      app_log("SPP Role: SPP client\r\n");
      reset_variables();
      sl_bt_gatt_server_set_max_mtu(247, &max_mtu_out);
      sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
      main_state = SCANNING;
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      if (process_scan_response(
            &evt->data.evt_scanner_legacy_advertisement_report)) {
        status = sl_bt_connection_open(
          evt->data.evt_scanner_legacy_advertisement_report.address,
          evt->data.evt_scanner_legacy_advertisement_report.address_type,
          sl_bt_gap_1m_phy,
          &conn_handle);
        if (SL_STATUS_OK == status) {
          sl_bt_scanner_stop();
        }
      }
      break;

    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened!\r\n");
      main_state = FIND_SERVICE;
      sl_bt_gatt_discover_primary_services_by_uuid(conn_handle,
                                                   16,
                                                   serviceUUID);
      break;

    case sl_bt_evt_connection_closed_id:
      if (STATE_SPP_MODE == main_state) {
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      }
      print_stats(&counters);
      reset_variables();
      sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
      main_state = SCANNING;
      break;

    case sl_bt_evt_connection_parameters_id:
      app_log("Conn.parameters: interval %u units\r\n",
              evt->data.evt_connection_parameters.interval);
      break;

    case sl_bt_evt_gatt_mtu_exchanged_id:
      // Calculate maximum data per one notification / write-without-response,
      // this depends on the MTU. up to ATT_MTU-3 bytes can be sent at once
      max_packet_size = evt->data.evt_gatt_mtu_exchanged.mtu - 3;

      /* Try to send maximum length packets whenever possible */
      min_packet_size = max_packet_size;
      app_log("MTU exchanged: %d\r\n", evt->data.evt_gatt_mtu_exchanged.mtu);
      break;

    case sl_bt_evt_gatt_service_id:
      if (evt->data.evt_gatt_service.uuid.len == 16) {
        if (memcmp(serviceUUID, evt->data.evt_gatt_service.uuid.data, 16)
            == 0) {
          app_log("Service discovered\r\n");
          service_handle = evt->data.evt_gatt_service.service;
        }
      }
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      switch (main_state) {
        case FIND_SERVICE:
          if (service_handle > 0) {
            // Service found, next step: search for characteristics
            sl_bt_gatt_discover_characteristics(conn_handle, service_handle);
            main_state = FIND_CHAR;
          } else {
            // Service is not found: disconnect
            app_log("SPP service not found!\r\n");
            sl_bt_connection_close(conn_handle);
          }
          break;

        case FIND_CHAR:
          if (char_handle > 0) {
            // Characteristic found, turn on indications
            sl_bt_gatt_set_characteristic_notification(conn_handle,
                                                       char_handle,
                                                       sl_bt_gatt_notification);
            main_state = ENABLE_NOTIF;
          } else {
            // Characteristic is not found: disconnect
            app_log("SPP char not found?\r\n");
            sl_bt_connection_close(conn_handle);
          }
          break;

        case ENABLE_NOTIF:
          main_state = STATE_SPP_MODE;
          app_log("SPP Mode ON\r\n");
          // disable deep sleep (for using USART)
          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
          break;

        default:
          break;
      }
      break;

    case sl_bt_evt_gatt_characteristic_id:
      if (evt->data.evt_gatt_characteristic.uuid.len == 16) {
        if (memcmp(charUUID, evt->data.evt_gatt_characteristic.uuid.data, 16)
            == 0) {
          app_log("Char discovered\r\n");
          char_handle = evt->data.evt_gatt_characteristic.characteristic;
        }
      }
      break;

    case sl_bt_evt_gatt_characteristic_value_id:
      if (evt->data.evt_gatt_characteristic_value.characteristic
          == char_handle) {
        for (uint8_t i = 0;
             i < evt->data.evt_gatt_server_attribute_value.value.len; i++) {
          sl_iostream_putchar(
            sl_iostream_vcom_handle,
            evt->data.evt_gatt_server_attribute_value.value.data[i]);
        }
        counters.num_pack_received++;
        counters.num_bytes_received +=
          evt->data.evt_gatt_server_attribute_value.value.len;
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static bool process_scan_response(
  sl_bt_evt_scanner_legacy_advertisement_report_t *pResp)
{
  // Decoding advertising packets is done here. The list of AD types can be
  // found at:
  // https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile

  uint8_t i = 0, ad_len, ad_type;
  bool ad_match_found = false;

  char name[32];

  while (i < (pResp->data.len - 1)) {
    ad_len = pResp->data.data[i];
    ad_type = pResp->data.data[i + 1];

    if ((ad_type == 0x08) || (ad_type == 0x09)) {
      // Type 0x08 = Shortened Local Name
      // Type 0x09 = Complete Local Name
      memcpy(name, &(pResp->data.data[i + 2]), ad_len - 1);
      name[ad_len - 1] = 0;
      app_log("%s\r\n", name);
    }

    // 4880c12c-fdcb-4077-8920-a450d7f9b907
    if ((ad_type == 0x06) || (ad_type == 0x07)) {
      // Type 0x06 = Incomplete List of 128-bit Service Class UUIDs
      // Type 0x07 = Complete List of 128-bit Service Class UUIDs
      if (memcmp(serviceUUID, &(pResp->data.data[i + 2]), 16) == 0) {
        app_log("Found SPP device\r\n");
        ad_match_found = true;
      }
    }
    // Jump to next AD record
    i = i + ad_len + 1;
  }

  return ad_match_found;
}

static void print_stats(ts_counters *p_counters)
{
  app_log("Outgoing data:\r\n");
  app_log(" bytes/packets sent: %lu / %lu ",
          p_counters->num_bytes_sent,
          p_counters->num_pack_sent);
  app_log(", num writes: %lu\r\n", p_counters->num_writes);
  app_log("(RX buffer overflow is not tracked)\r\n");
  app_log("Incoming data:\r\n");
  app_log(" bytes/packets received: %lu / %lu\r\n",
          p_counters->num_bytes_received,
          p_counters->num_pack_received);

  return;
}

static void send_spp_data()
{
  uint8_t len = 0;
  uint8_t data[256];
  sl_status_t result, read_result;
  uint16_t sent_len = 0;
  uint32_t timeout = 0;
  char c;

  // Read up to max_packet_size characters from local buffer
  while (len < max_packet_size) {
    read_result = sl_iostream_getchar(sl_iostream_vcom_handle, &c);
    if (SL_STATUS_OK == read_result) {
      data[len++] = (uint8_t)c;
    } else if (len == 0) {
      /* If the first ReadChar() fails then return immediately */
      return;
    } else {
      // Speed optimization: if there are some bytes to be sent but the length
      // is still below the preferred minimum packet size, then wait for
      // additional bytes until timeout. Target is to put as many bytes as
      // possible into each air packet.

      // Conditions for exiting the while loop and proceed to send data:
      if (timeout++ > UART_POLL_TIMEOUT) {
        break;
      } else if (len >= min_packet_size) {
        break;
      }
    }
  }

  if (len > 0) {
    // Stack may return "out-of-memory" (SL_STATUS_NO_MORE_RESOURCE) error if
    //   the local buffer is full -> in that case, just keep trying until the
    //   command succeeds
    do {
      result = sl_bt_gatt_write_characteristic_value_without_response(
        conn_handle,
        char_handle,
        len,
        data,
        &sent_len);
      counters.num_writes++;
    } while (result == SL_STATUS_NO_MORE_RESOURCE);

    if (result != 0) {
      app_log("Unexpected error: %lu\r\n", result);
    } else {
      counters.num_pack_sent++;
      counters.num_bytes_sent += len;
    }
  }
  return;
}
