/**************************************************************************//**
* @file app.c
* @brief Application interface provided to main().
* @version 1.0.0
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "ble_att_handler.h"
#include "app_log.h"
#include "app_assert.h"
#include "app.h"
#include "hrm_app.h"
#include "heart_rate.h"
#include "pulse_oximeter.h"
#include "device_information.h"

static void services_init(void);

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t connection_handle = 0xff;

static void app_bt_evt_system_external_signal_handler(uint32_t ext_signal);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  app_log("====== Application Initialization =======\n");
  hrm_init_app();
  services_init();
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
  hrm_loop();
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

  switch (SL_BT_MSG_ID(evt->header))
  {
    /* ------------------------------- */
    /* This event indicates the device has started and the radio is ready. */
    /* Do not call any stack command before receiving this boot event! */
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
        160,      // min. adv. interval (milliseconds * 1.6)
        160,      // max. adv. interval (milliseconds * 1.6)
        0,        // adv. duration
        0);       // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_log("[BLE]: Start advertising ...\n");
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising\n",
                 (int)sc);
      break;

    /* ------------------------------- */
    /* This event indicates that a new connection was opened. */
    case sl_bt_evt_connection_opened_id:
      connection_handle = evt->data.evt_connection_opened.connection;
      app_log("[BLE]: Connection opened.\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("[BLE]: Connection closed.\n");

      /* Service disconnect handlers */
      heart_rate_disconnect_event(evt);
      pulse_oximeter_disconnect_event(evt);
      device_information_disconnect_event(evt);
      connection_handle = 0xff;

      /* Restart advertising after client has disconnected. */
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

    case sl_bt_evt_gatt_server_user_write_request_id:

      /* Service write handlers */
      /* Heart Rate characteristics written */
      if (evt->data.evt_gatt_server_user_write_request.characteristic
          == gattdb_heart_rate_control_point) {
        heart_rate_write_callback(evt);
      }

      /* Pulse Oximeter characteristics written */
      else if (evt->data.evt_gatt_server_user_write_request.characteristic
               == gattdb_record_access_control_point) {
        pulse_oximeter_write_callback(evt);
      }

      break;

    case sl_bt_evt_gatt_server_user_read_request_id:

      /* Handle previous read operation for long characteristics */
      if (ble_att_send_data_handler(evt->data.evt_gatt_server_user_read_request.
                                    characteristic,
                                    evt->data.evt_gatt_server_user_read_request.
                                    offset)) {
        /* Event handled */
        break;
      }

      /* Service read handlers */
      /* Heart Rate characteristics read */
      if (evt->data.evt_gatt_server_user_read_request.characteristic
          == gattdb_body_sensor_location) {
        heart_rate_read_callback(evt);
      }

      /* Device Information characteristics read */
      else if ((evt->data.evt_gatt_server_user_read_request.characteristic
                >= gattdb_manufacturer_name_string)
               && (evt->data.evt_gatt_server_user_read_request.characteristic
                   <= gattdb_firmware_revision_string)) {
        device_information_read_callback(evt);
      }

      /* Pulse Oximeter characteristics read */
      else if ((evt->data.evt_gatt_server_user_read_request.characteristic
                >= gattdb_plx_spot_check_measurement)
               && (evt->data.evt_gatt_server_user_read_request.characteristic
                   <= gattdb_record_access_control_point)) {
        pulse_oximeter_read_callback(evt);
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      /* Heart Rate characteristics read */
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_heart_rate_measurement) {
        heart_rate_characteristic_status(evt);
      } else if (evt->data.evt_gatt_server_characteristic_status.characteristic
                 == gattdb_plx_continuous_measurement) {
        pulse_oximeter_characteristic_status(evt);
      }
      break;

    /*Handle exteral signal*/
    case sl_bt_evt_system_external_signal_id:
      app_bt_evt_system_external_signal_handler(
        evt->data.evt_system_external_signal.extsignals);
      break;

    /* ------------------------------- */
    /* Default event handler. */
    default:
      break;
  }
}

/*******************************************************************************
 * @brief
 *   BLE service initialization
 * @param
 *   None
 * @return
 *   None
 ******************************************************************************/
static void services_init(void)
{
  heart_rate_init();
  device_information_init();
  pulse_oximeter_init();
}

/*******************************************************************************
 * @brief
 *   Handle external signal events.
 * @param[in] ext_signal
 *   External signal.
 ******************************************************************************/
static void app_bt_evt_system_external_signal_handler(uint32_t ext_signal)
{
  switch (ext_signal)
  {
    case MAXM86161_IRQ_EVENT:
    case BTN0_IRQ_EVENT:
      hrm_process_event(ext_signal);
      break;
    case HEART_RATE_TIMER_EXT:
      heart_rate_send_new_data(connection_handle);
      break;
    case PLUSE_OXIMETER_TIMER_EXT:
      pulse_oximeter_send_new_data(connection_handle);
      break;
    case OLED_TIMER_EXT:
      hrm_update_display();
      break;
    default:
      break;
  }
}
