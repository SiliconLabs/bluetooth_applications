/***************************************************************************//**
 * @file
 * @brief BGM GATT service
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
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include "sl_bluetooth.h"
#include "sl_bt_bgm.h"
#include "app_log.h"
#include "app_assert.h"
#include "assert.h"
#include <stdio.h>

uint8_t security_level = 1;
uint8_t connection;
// Glucose measurement current record number
uint16_t records_num = 3;

// Glucose sequence number
uint8_t seq_num = 0;

// GLusose abort operation - Report Stored Records
bool bgm_abort_operation_flag = false;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
// Fast advertiser 30s timer
static sl_sleeptimer_timer_handle_t bgm_fast_adv_timer;
static bool fast_adv = true;
void sl_bt_start_adv(void);

/**************************************************************************//**
 * BGM fast advertiser 30s oneshot timer stop
 *****************************************************************************/
static void sl_bgm_fast_adv_timeout(sl_sleeptimer_timer_handle_t *handle,
                                    void *data)
{
  (void)&handle;
  (void)&data;

  sl_bt_external_signal(SIGNAL_FAST_ADV_TIMEOUT);
}

/**************************************************************************//**
 * start customer advertisement
 *****************************************************************************/
void sl_bt_start_adv(void)
{
  sl_status_t sc = SL_STATUS_FAIL;
  // Start general advertising and enable connections.
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);
  if (fast_adv) {
    app_log("start fast advertiser 20 ms to 30 ms\n");
    sl_sleeptimer_start_periodic_timer(&bgm_fast_adv_timer,
                                       SL_BGM_FAST_ADV_TIMEOUT,
                                       sl_bgm_fast_adv_timeout,
                                       NULL,
                                       0,
                                       0);
  } else {
    app_log("set advertising interval long(1 s to 2.5 s) to reduce power\n");
  }
}

static bd_addr address;
static uint8_t type;

/**************************************************************************//**
 * create customer advertisement according Glucose Profile (GLP) test suite
 * 4.4.3 GLP/SEN/GLF/BV-03-I [Public Address in AD or Scan Response]
 *****************************************************************************/
void sl_bt_create_user_adv(void)
{
  sl_status_t sc;
  uint8_t temp;
  if (type == sl_bt_gap_public_address) {
    temp = AD_PUBLIC_ADDRESS;
  } else {
    temp = AD_RANDOM_ADDRESS;
  }
  const uint8_t value[] =
  { 0x02, 0x01, 0x06, 0x03, 0x03, 0x08, 0x18, 0x0B, 0x08, 0x73, \
    0x69, 0x6C, 0x61, 0x62, 0x73, 0x2D, 0x62, 0x67,
    0x6D, 0x03,                                        \
    0x19, 0x0D, 0x00, 0x07, temp, address.addr[0],     \
    address.addr[1], address.addr[2], address.addr[3], \
    address.addr[4], address.addr[5] };
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        sizeof(value),
                                        value);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Callback function of connection close event.
 *****************************************************************************/
static void sl_bt_connection_closed_cb(uint16_t reason, uint8_t connection)
{
  (void)reason;
  (void)connection;
  sl_status_t sc = SL_STATUS_OK;
  app_log("central disconnect\n");
  sl_bt_create_user_adv();
  sl_bt_start_adv();
  app_assert_status(sc);
}

/**************************************************************************//**
 * list all the current bonding address
 *****************************************************************************/
void list_bondings(void)
{
  bd_addr address;
  uint32_t num_bondings;
  size_t bondings_len;
  uint8_t bondings[10];
  sl_status_t sc;
  uint8_t address_type;
  uint8_t security_mode;
  uint8_t key_size;
  sl_bt_sm_get_bonding_handles(0,
                               &num_bondings,
                               10,
                               &bondings_len,
                               bondings);
  if (num_bondings > 0) {
    for (uint8_t i = 0; i < num_bondings; i++) {
      sc = sl_bt_sm_get_bonding_details(i,
                                        &address,
                                        &address_type,
                                        &security_mode,
                                        &key_size);
      app_assert_status(sc);
      app_log("address type %d, bonding handler %d: ", address_type, i);
      for (uint8_t i = 0; i < 6; i++) {
        app_log("0x%02X ", address.addr[i]);
      }
      app_log("\n");
    }
  }
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 *****************************************************************************/
void sl_bt_bgm_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc = SL_STATUS_FAIL;
  uint8_t bonding;
  uint16_t handler;
  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id: {
      bd_addr   address;
      uint8_t   address_type;
      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      if (sc) {
        app_log("Get address error = 0x%04lX\r\n", sc);
        app_assert_status(sc);
      }
      app_log("address:\n");
      for (uint8_t i = 0; i < sizeof(address.addr); i++) {
        app_log("%02X ", address.addr[i]);
      }
      app_log("\n");
      // security parameters
      uint8_t flags = 0x0E;
      uint8_t io_capabilities = sl_bt_sm_io_capability_noinputnooutput;
      app_log("system boot in BGM\n");
      sc = sl_bt_sm_set_bondable_mode(1);
      sc = sl_bt_sm_configure(flags, io_capabilities);
      app_assert_status(sc);
      app_log("set security\n");
      // for(uint8_t i = 0; i < BGM_DEFUALT_RECORDS_NUM; i++){
      // sl_bt_bgm_add_measurement_record();
      // }
      list_bondings();
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);
      // First 30 seconds (fast connection) Advertising Interval 20 ms to 30 ms
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        32, // min. adv. interval Value in units of 0.625 ms
        48, // max. adv. interval Value in units of 0.625 ms
        3000,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      sl_bt_create_user_adv();
      sl_bt_start_adv();
    }
    break;
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection = evt->data.evt_connection_opened.connection;
      type = evt->data.evt_connection_opened.address_type;
      app_log("client address type is %d\n", type);
      address = evt->data.evt_connection_opened.address;

      for (uint8_t i = 0; i < 6; i++) {
        app_log("0x%02X ", address.addr[i]);
      }
      app_log("\n");
      bonding = evt->data.evt_connection_opened.bonding;
      app_log_info("current connection handler is %d， bonding 0x%02X\n", \
                   connection, bonding);
      break;
    case sl_bt_evt_connection_closed_id:
      sl_bt_connection_closed_cb(evt->data.evt_connection_closed.reason,
                                 evt->data.evt_connection_closed.connection);
      break;
    case sl_bt_evt_gatt_server_characteristic_status_id:
      handler = evt->data.evt_gatt_server_characteristic_status.characteristic;
      switch (handler) {
        case gattdb_glucose_measurement:
          sl_bt_bgm_measurement_notification_handler(evt);
          break;
        case gattdb_glucose_measurement_context:
          sl_bt_bgm_measurement_context_handler(evt);
          break;
        case gattdb_record_access_control_point:
          sl_bt_bgm_racp_indication_handler(evt);
          break;
        default:
          app_log_info("GATT handler is %d\n", handler);
          break;
      }
      break;
    case sl_bt_evt_gatt_server_user_write_request_id:
      handler = evt->data.evt_gatt_server_user_write_request.characteristic;
      switch (handler) {
        case gattdb_record_access_control_point:
          app_log("client write RACP: \n");
          sl_bt_bgm_racp_handler(evt);
          break;
        default:
          app_log("wrong write handler\n");
          break;
      }
      break;
    case sl_bt_evt_gatt_server_user_read_request_id:
      handler = evt->data.evt_gatt_server_user_write_request.characteristic;
      switch (handler) {
        case gattdb_record_access_control_point:
          app_log("client write RACP: \n");
          sl_bt_bgm_racp_handler(evt);
          break;
        case gattdb_glucose_feature:
          sl_bt_bgm_read_feature(evt);
          break;
        default:
          app_log("wrong read handler %d\n", handler);
          break;
      }
      break;
    case sl_bt_evt_connection_parameters_id:
      app_log("security level %d\n",
              evt->data.evt_connection_parameters.security_mode + 1);
      security_level = evt->data.evt_connection_parameters.security_mode + 1;
      break;
    case sl_bt_evt_sm_bonded_id:
      app_log("bonded\n");
      break;
    case sl_bt_evt_sm_bonding_failed_id:
      app_log("bonding failed 0x%04X\n",
              evt->data.evt_sm_bonding_failed.reason);
      break;
    case sl_bt_evt_connection_phy_status_id:
      app_log("PHY is %d\n", evt->data.evt_connection_phy_status.phy);
      break;
    case sl_bt_evt_connection_remote_used_features_id:
      break;
    case sl_bt_evt_sm_confirm_bonding_id:
      connection = evt->data.evt_sm_confirm_bonding.connection;
      app_log_info("sl_bt_evt_sm_confirm_bonding_id 0x%08lX\n", evt->header);
      sc = sl_bt_sm_bonding_confirm(connection, 1);
      if (sc != SL_STATUS_OK) {
        app_log_info("sl_bt_sm_bonding_confirm failed\n");
      }
      break;
    case sl_bt_evt_gatt_mtu_exchanged_id:
      app_log("MTU is %d\n", evt->data.evt_gatt_mtu_exchanged.mtu);
      break;
    case sl_bt_evt_sm_confirm_passkey_id:
      app_log_info("sl_bt_evt_sm_confirm_passkey_id 0x%08lX\n", evt->header);
      break;
    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          == SIGNAL_FAST_ADV_TIMEOUT) {
        // Stop timer.
        sc = sl_sleeptimer_stop_timer(&bgm_fast_adv_timer);
        if (sc != SL_STATUS_OK) {
          app_log_info("stop bgm_fast_adv_timer failed 0x%04lX\n", sc);
        }
        fast_adv = false;
        // After 30 seconds (reduced power) Advertising Interval 1 s to 2.5 s
        sc = sl_bt_advertiser_set_timing(
          advertising_set_handle,
          1600,     // min. adv. interval Value in units of 0.625 ms
          4000,     // max. adv. interval Value in units of 0.625 ms
          0,       // adv. duration
          0);      // max. num. adv. events
        app_assert_status(sc);
        sl_bt_start_adv();
      }
      if (evt->data.evt_system_external_signal.extsignals
          == SIGNAL_REPORT_ALL) {
        sl_bt_signal_report_all_cb();
      }
      break;
    default:
      app_log("unknown evt 0x%08lX\n", evt->header);
      break;
  }
}
