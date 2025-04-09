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
#include <stdbool.h>
#include "sl_common.h"
#include "sl_status.h"
#include "sl_sleeptimer.h"
#include "app_log.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "sl_sensor_rht.h"
#include "sl_health_thermometer.h"
#include "app.h"

#define TEMP_INDICATE_SIGNAL  1 << 0
#define MITM_PROTECTION       (0x01) // 0=JustWorks,
                                     // 1=PasskeyEntry or NumericComparison
static bool timer_running = false;
// Connection handle.
static uint8_t app_connection = 0;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Periodic timer handle.
static sl_sleeptimer_timer_handle_t app_periodic_timer;

// Periodic timer callback.
static void app_periodic_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                                  void *data);
static void app_indicate_temperature(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;
  app_log("health thermometer initialised\n");
  // Init temperature sensor.
  sc = sl_sensor_rht_init();
  if (sc != SL_STATUS_OK) {
    app_log_warning("Thermometer sensor initialization failed.");
    app_log_nl();
  }
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

  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      sl_bt_sm_delete_bondings();
      sl_bt_sm_configure(MITM_PROTECTION, sl_bt_sm_io_capability_displayonly);
      sl_bt_sm_set_bondable_mode(1);
      app_log("bondable mode is set");
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle, // advertising set handle
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);

      app_log_info("Started advertising\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("Connection opened\n");
      sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("Connection closed\n");

      sc = sl_sleeptimer_is_timer_running(&app_periodic_timer, &timer_running);
      app_assert_status(sc);

      if (timer_running) {
        // stop timer
        sc = sl_sleeptimer_stop_timer(&app_periodic_timer);
        app_assert_status(sc);
      }

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_sm_passkey_display_id:
      app_log("Passkey: %d\n", (int)evt->data.evt_sm_passkey_display.passkey);
      break;

    case sl_bt_evt_sm_confirm_bonding_id:
      sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection, 1);
      app_log("confirm bonding");
      break;

    case sl_bt_evt_sm_confirm_passkey_id:
      app_log("Confirm: %d\n", (int)evt->data.evt_sm_confirm_passkey.passkey);
      sl_bt_sm_passkey_confirm(evt->data.evt_sm_confirm_passkey.connection, 1);
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      app_log("rip\r\n");
      app_log("Device no secure. Closed\r\n");
      sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          & TEMP_INDICATE_SIGNAL) {
        app_indicate_temperature();
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Health Thermometer - Temperature Measurement
 * Indication changed callback
 *
 * Called when indication of temperature measurement is enabled/disabled by
 * the client.
 *****************************************************************************/
void sl_bt_ht_temperature_measurement_indication_changed_cb(uint8_t connection,
                                                            sl_bt_gatt_client_config_flag_t client_config)
{
  sl_status_t sc;
  app_connection = connection;
  // Indication or notification enabled.
  if (sl_bt_gatt_disable != client_config) {
    // Start timer used for periodic indications.
    sc = sl_sleeptimer_start_periodic_timer_ms(&app_periodic_timer,
                                               SL_BT_HT_MEASUREMENT_INTERVAL_SEC * 1000,
                                               app_periodic_timer_cb,
                                               NULL,
                                               0,
                                               0);
    app_assert_status(sc);
    // Send first indication.
    app_periodic_timer_cb(&app_periodic_timer, NULL);
  }
  // Indications disabled.
  else {
    // Stop timer used for periodic indications.
    (void)sl_sleeptimer_stop_timer(&app_periodic_timer);
  }
}

/**************************************************************************//**
 * Timer callback
 * Called periodically to time periodic temperature measurements and
 *   indications.
 *****************************************************************************/
static void app_periodic_timer_cb(sl_sleeptimer_timer_handle_t *timer,
                                  void *data)
{
  (void)data;
  (void)timer;

  sl_bt_external_signal(TEMP_INDICATE_SIGNAL);
}

static void app_indicate_temperature(void)
{
  sl_status_t sc;
  int32_t temperature = 0;
  uint32_t humidity = 0;
  float tmp_c = 0.0;
  // float tmp_f = 0.0;

  // Measure temperature; units are % and milli-Celsius.
  sc = sl_sensor_rht_get(&humidity, &temperature);
  if (SL_STATUS_NOT_INITIALIZED == sc) {
    app_log_info("Relative Humidity and Temperature sensor is not initialized.");
    app_log_nl();
  } else if (sc != SL_STATUS_OK) {
    app_log_warning("Invalid RHT reading: %lu %ld\n", humidity, temperature);
  }

  tmp_c = (float)temperature / 1000;
  app_log_info("Temperature: %5.2f C\n", tmp_c);
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
