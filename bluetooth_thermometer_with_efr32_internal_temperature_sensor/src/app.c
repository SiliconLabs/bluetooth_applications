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
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_log.h"
#include "app_timer.h"
#include "tempdrv.h"
#include "sl_health_thermometer.h"

// Connection handle.
static uint8_t app_connection = 0;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Periodic timer handle.
static app_timer_t app_periodic_timer;

// Periodic timer callback.
static void app_periodic_timer_cb(app_timer_t *timer, void *data);

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
  bd_addr address;
  uint8_t address_type;

  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Print boot message.
      app_log_info("Bluetooth stack booted: v%d.%d.%d-b%d\n",
                   evt->data.evt_system_boot.major,
                   evt->data.evt_system_boot.minor,
                   evt->data.evt_system_boot.patch,
                   evt->data.evt_system_boot.build);

      // Extract unique ID from BT Address.
      sc = sl_bt_gap_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      app_log_info("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   address_type ? "static random" : "public device",
                   address.addr[5],
                   address.addr[4],
                   address.addr[3],
                   address.addr[2],
                   address.addr[1],
                   address.addr[0]);

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

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);

      app_log_info("Started advertising\n");

      TEMPDRV_Init();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("Connection opened\r\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("Connection closed\n");

      // Stop timer.
      sc = app_timer_stop(&app_periodic_timer);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (gattdb_temperature_measurement
          == evt->data.evt_gatt_server_characteristic_status.characteristic) {
        // client characteristic configuration changed by remote GATT client
        if (sl_bt_gatt_server_client_config
            == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.
            evt_gatt_server_characteristic_status.status_flags) {
          sl_bt_ht_temperature_measurement_indication_changed_cb(
            evt->data.evt_gatt_server_characteristic_status.connection,
            (sl_bt_gatt_client_config_flag_t)evt->data.
            evt_gatt_server_characteristic_status.client_config_flags);
        }
        // confirmation of indication received from remove GATT client
        else if (sl_bt_gatt_server_confirmation
                 == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.
                 evt_gatt_server_characteristic_status.status_flags) {
          sl_bt_ht_temperature_measurement_indication_confirmed_cb(
            evt->data.evt_gatt_server_characteristic_status.connection);
        } else {
          app_assert(false,
                     "[E: 0x%04x] Unexpected status flag in evt_gatt_server_characteristic_status\n",
                     (int)evt->data.evt_gatt_server_characteristic_status.
                     status_flags);
        }
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

/**************************************************************************//**
 * Temperature Measurement characteristic indication confirmed.
 *****************************************************************************/
void sl_bt_ht_temperature_measurement_indication_confirmed_cb(
  uint8_t connection)
{
  (void)connection;
}

/**************************************************************************//**
 * Health Thermometer - Temperature Measurement
 * Indication changed callback
 *
 * Called when indication of temperature measurement is enabled/disabled by
 * the client.
 *****************************************************************************/
void sl_bt_ht_temperature_measurement_indication_changed_cb(
  uint8_t connection, sl_bt_gatt_client_config_flag_t client_config)
{
  sl_status_t sc;
  app_connection = connection;
  // Indication or notification enabled.
  if (sl_bt_gatt_disable != client_config) {
    // Start timer used for periodic indications.
    sc = app_timer_start(&app_periodic_timer,
                         SL_BT_HT_MEASUREMENT_INTERVAL_SEC * 1000,
                         app_periodic_timer_cb,
                         NULL,
                         true);
    app_assert_status(sc);
    // Send first indication.
    app_periodic_timer_cb(&app_periodic_timer, NULL);
  }
  // Indications disabled.
  else {
    // Stop timer used for periodic indications.
    (void)app_timer_stop(&app_periodic_timer);
  }
}

/**************************************************************************//**
 * Timer callback
 * Called periodically to time periodic temperature measurements and indications.
 *****************************************************************************/
static void app_periodic_timer_cb(app_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  sl_status_t sc;
  int32_t temperature;

  // Measure temperature; units are milli-Celsius.
  temperature = 1000 * (int32_t)TEMPDRV_GetTemp();
  app_log_info("Temperature: %ld mC\n", temperature);
  // Send temperature measurement indication to connected client.
  sc = sl_bt_ht_temperature_measurement_indicate(app_connection,
                                                 temperature,
                                                 false);
  // Conversion to Fahrenheit: F = C * 1.8 + 32
  // tmp_f = (float)(temperature*18+320000)/10000;
  // app_log_info("Temperature: %5.2f F\n", tmp_f);
  // Send temperature measurement indication to connected client.
  // sc = sl_bt_ht_temperature_measurement_indicate(app_connection,
  //                                                (temperature*18+320000)/10,
  //                                                true);
  if (sc) {
    app_log_warning("Failed to send temperature measurement indication\n");
  }
}
