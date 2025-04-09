/***************************************************************************//**
 * @file app.c
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
#include "app_assert.h"
#include "app_log.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_timer.h"
#include "lcd.h"
#include "env.h"
#include "var.h"
#include "ui.h"

#include "peps_follower.h"
#include "peps_leader.h"

#include "lfxoctune.h"

#include <em_gpio.h>

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t connection_handle = SL_BT_INVALID_CONNECTION_HANDLE;

static void rssi_capture_timer_start(uint32_t timeout_ms);
static void rssi_timer_cb(app_timer_t *timer, void *data);
static void rssi_timer_start(void);
static void rssi_timer_stop(void);

static app_timer_t rssi_timer;

typedef enum rssi_capture_state {
  RSSI_CAPTURE_IDLE,
  RSSI_CAPTURE_CONNECTED,
  RSSI_CAPTURE_ONGOING,
} rssi_capture_state_t;

static app_timer_t rssi_capture_timer;
static bool rssi_capture_started = false;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t ret;

  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  lcd_init();
  var_init();
  env_init();
  lfxo_ctune_init();

  ui_set_name(env_device_name);

  if (env_device_type == ENV_DEVICE_PEPS_FOLLOWER) {
    ui_set_address(env_base_address);

    ret = peps_follower_init();

    if (ret == SL_STATUS_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "PEPS follower mode has been initialized.\n");
    } else {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to initialize PEPS follower mode!\n");
    }
  } else if (env_device_type == ENV_DEVICE_PEPS_LEADER) {
    ret = peps_leader_init();

    if (ret == SL_STATUS_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "PEPS leader mode has been initialized.\n");
    } else {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to initialize PEPS leader mode!\n");
    }
  } else {
    app_log_level(APP_LOG_LEVEL_INFO, "Unsupported device mode, ignoring.\n");
  }

  lcd_update();

  ret = sl_bt_system_start_bluetooth();
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to start Bluetooth!\n");
  }

  app_log_level(APP_LOG_LEVEL_INFO, "");
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

  if (env_device_type == ENV_DEVICE_PEPS_FOLLOWER) {
    peps_follower_tick();
  }
}

static void rssi_timer_cb(app_timer_t *timer, void *data)
{
  (void)timer;
  (void)data;
  sl_status_t sc;

  if (connection_handle != SL_BT_INVALID_CONNECTION_HANDLE) {
    unsigned int i;
    bool success;

    sc =
      sl_bt_connection_get_median_rssi(connection_handle,
                                       &var_central_rssi[ENV_LOCATION_CENTER]);
    if (sc != SL_STATUS_OK) {
      var_central_rssi[ENV_LOCATION_CENTER] = -128;
    }

    ui_set_rssi(var_central_rssi[ENV_LOCATION_CENTER]);

    success = peps_leader_fetch_rssi_values();

    for (i = 0; i < ENV_LOCATION_CENTER; i++)
    {
      ui_set_location_rssi(i, var_central_rssi[i]);
    }

    if (success) {
      peps_leader_update_location();
    } else {
      app_log_level(APP_LOG_LEVEL_WARNING,
                    "Failed to fetch all RSSI, skipping calculation!\n");

      // stop RSSI collection
      rssi_timer_stop();
      // disconnect the slaves
      peps_leader_broadcast_disconnect();
      // reconnect the slaves
      rssi_capture_timer_start(100);
    }

    lcd_update();
  }
}

static void rssi_timer_start(void)
{
  sl_status_t sc;

  rssi_capture_started = true;
  sc = app_timer_start(&rssi_timer, 1000, rssi_timer_cb, NULL, true);
  app_assert_status(sc);
}

static void rssi_timer_stop(void)
{
  sl_status_t sc;

  sc = app_timer_stop(&rssi_timer);
  app_assert_status(sc);

  rssi_capture_started = false;
}

static void rssi_capture_timer_start_cb(app_timer_t *timer, void *data)
{
  (void)timer;
  (void)data;

  if (peps_leader_forward_connection_details(connection_handle)
      == SL_STATUS_OK) {
    rssi_timer_start();
  }
}

static void rssi_capture_timer_start(uint32_t timeout_ms)
{
  sl_status_t sc;

  sc = app_timer_start(&rssi_capture_timer,
                       timeout_ms,
                       rssi_capture_timer_start_cb,
                       NULL,
                       false);
  app_assert_status(sc);
}

static void __attribute__((unused)) rssi_capture_timer_stop(void)
{
  sl_status_t sc;

  sc = app_timer_stop(&rssi_timer);
  app_assert_status(sc);
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
      if (env_device_type == ENV_DEVICE_PEPS_LEADER) {
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
      }

      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection_handle = evt->data.evt_connection_opened.connection;
      app_log_info("Connection opened.\n");

      if (env_device_type == ENV_DEVICE_PEPS_LEADER) {
#if 0
        sc = sl_bt_connection_set_parameters(connection_handle,
                                             80,
                                             80,
                                             0,
                                             30,
                                             0,
                                             65535);
        if (sc != SL_STATUS_OK) {
          app_log_warning("sl_bt_connection_set_parameters returned %08x\n",
                          sc);
        }

        sc = sl_bt_connection_disable_slave_latency(connection_handle, 1);
        if (sc != SL_STATUS_OK) {
          app_log_warning(
            "sl_bt_connection_disable_slave_latency returned %08x\n",
            sc);
        }
#endif

        rssi_capture_timer_start(2730);
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      connection_handle = SL_BT_INVALID_CONNECTION_HANDLE;
      app_log_info("Connection closed.\n");

      if (env_device_type == ENV_DEVICE_PEPS_LEADER) {
        unsigned int i;

        rssi_timer_stop();

        peps_leader_broadcast_disconnect();

        ui_clear_rssi();
        ui_clear_angle();
        ui_clear_distance();

        for (i = 0; i < ENV_LOCATION_CENTER; i++)
        {
          ui_clear_location_rssi(i);
        }

        lcd_update();

        // Generate data for advertising
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        // Restart advertising after client has disconnected.
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
      }
      break;

    // -------------------------------
    // This event indicates that the value of an attribute in the local GATT
    // database was changed by a remote GATT client.
    case sl_bt_evt_gatt_server_attribute_value_id:
      break;

    // -------------------------------
    // This event occurs when the remote device enabled or disabled the
    // notification.
    case sl_bt_evt_gatt_server_characteristic_status_id:
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_connection_analyzer_report_id:
      app_log_level(APP_LOG_LEVEL_INFO, "central: %d peripheral: %d\n",
                    evt->data.evt_connection_analyzer_report.central_rssi,
                    evt->data.evt_connection_analyzer_report.peripheral_rssi);

      // subsequent toggles on the slave side mean that a report has arrived
      GPIO_PinOutToggle(MONACT_PORT, MONACT_PIN);

      peps_follower_update_rssi(
        evt->data.evt_connection_analyzer_report.central_rssi,
        evt->data.evt_connection_analyzer_report.peripheral_rssi);
      // ui_set_rssi(evt->data.evt_connection_analyzer_report.central_rssi);
      // lcd_update();

      break;

    case sl_bt_evt_connection_analyzer_completed_id:
      GPIO_PinOutToggle(MONACT_PORT, MONACT_PIN);
      GPIO_PinOutToggle(MONACT_PORT, MONACT_PIN);
      peps_follower_sniffer_stopped();

      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

bool app_stop(void)
{
  bool ret = false;

  if (rssi_capture_started) {
    rssi_timer_stop();
    ret = true;
  }

  return ret;
}

bool app_start(void)
{
  bool ret = false;

  if (!rssi_capture_started
      && (connection_handle != SL_BT_INVALID_CONNECTION_HANDLE)) {
    rssi_capture_timer_start(100);
    ret = true;
  }

  return ret;
}
