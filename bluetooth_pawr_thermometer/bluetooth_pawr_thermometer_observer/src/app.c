/***************************************************************************//**
 * @file app.c
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "string.h"
#include "sl_bt_api.h"
#include "sl_sleeptimer.h"
#include "sl_simple_led_instances.h"

#include "gatt_db.h"
#include "app.h"
#include "app_assert.h"

#include "tempdrv.h"

// Ping opcode
#define PAWR_PING_OPCODE               0x00
// Read data sensor opcode
#define PAWR_READ_DATA_SENSOR_OPCODE   0x01
// Control LED opcode
#define PAWR_CONTROL_LED_OPCODE        0x02

typedef struct device_address_s {
  uint8_t group_id; // ranging from 0-127
  uint8_t device_id; // valid in the range of 1-255.
                     // 0 is the default (reset) value meaning no address
                     // assigned to the device
} device_address_t;

static uint8_t advertising_set_handle = 0xff;
static uint8_t conn_handle = 0xff;

static device_address_t device_addr;

static void process_opcode(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  sc = TEMPDRV_Init();
  app_assert_status(sc);
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

      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);
      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);
      sc = sl_bt_sm_delete_bondings();
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // set parameter to receive sync
      sc = sl_bt_past_receiver_set_default_sync_receive_parameters(
        sl_bt_past_receiver_mode_synchronize,
        0,
        1000,
        sl_bt_sync_report_all);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      conn_handle = evt->data.evt_connection_opened.connection;
      break;

    case sl_bt_evt_sm_confirm_bonding_id:
      sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection, 1);
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      if (evt->data.evt_gatt_server_attribute_value.attribute
          == gattdb_device_address) {
        device_addr.group_id =
          evt->data.evt_gatt_server_attribute_value.value.data[0];
        device_addr.device_id =
          evt->data.evt_gatt_server_attribute_value.value.data[1];
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // 0 is the default (reset) value
      // It means no address assigned to the device
      if (device_addr.device_id == 0) {
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
      }
      break;

    case sl_bt_evt_pawr_sync_transfer_received_id:
      sc = sl_bt_connection_close(conn_handle);
      app_assert_status(sc);
      break;

    case sl_bt_evt_pawr_sync_subevent_report_id:
      process_opcode(evt);
      break;

    case sl_bt_evt_sync_closed_id:
      // Reset value meaning no address assigned to the device
      device_addr.device_id = 0;
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);
      // Restart advertising after sync is lost
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static void process_opcode(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t opcode = evt->data.evt_pawr_sync_subevent_report.data.data[2];

  switch (opcode) {
    case PAWR_PING_OPCODE:
      break;
    case PAWR_READ_DATA_SENSOR_OPCODE:
    {
      uint8_t response_data[2] = { TEMPDRV_GetTemp(),
                                   sl_led_get_state(&sl_led_led0) };
      sc = sl_bt_pawr_sync_set_response_data(
        evt->data.evt_pawr_sync_subevent_report.sync,
        evt->data.evt_pawr_sync_subevent_report.event_counter,
        evt->data.evt_pawr_sync_subevent_report.subevent,
        evt->data.evt_pawr_sync_subevent_report.subevent,
        device_addr.device_id,
        sizeof(response_data),
        response_data);
      app_assert_status(sc);
    }
    break;
    case PAWR_CONTROL_LED_OPCODE:
      sl_led_toggle(&sl_led_led0);
      break;
  }
}
