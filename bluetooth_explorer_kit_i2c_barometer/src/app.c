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
#include "sl_i2cspm_instances.h"
#include "mikroe_dps310_i2c.h"

#define READING_INTERVAL_MSEC 1000
#define APP_TIMER_EXT_SIGNAL  0x01

static bool temperature_notify_enable = false;
static bool pressure_notify_enable = false;
static uint8_t connection = 0;

static uint8_t advertising_set_handle = 0xff;

static sl_sleeptimer_timer_handle_t app_timer;

static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                               void *data);

static void app_read_temp_press(void);

static void app_gatt_update(float temperature, float pressure);

static void app_send_notification(uint8_t *value, uint16_t characteristic);

static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                               void *data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;
  app_log("============== Application initialization ==============\n");
  sc = mikroe_pressure3_init(sl_i2cspm_mikroe);
  if (sc != SL_STATUS_OK) {
    app_log("[DPS310]: Mikroe Pressure 3 Click initialized failed!\n");
  } else {
    app_log("[DPS310]: Mikroe Pressure 3 Click initialized success.\n");
  }
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

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);

      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to generate advertising data.\n",
                 (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set advertising timing.\n",
                 (int)sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising.\n",
                 (int)sc);
      app_log("[BLE]: Start advertising ...\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:

      app_log("[BLE]: Connection opened.\n");

      temperature_notify_enable = false;
      pressure_notify_enable = false;
      connection = evt->data.evt_connection_opened.connection;

      sl_sleeptimer_start_periodic_timer_ms(&app_timer,
                                            READING_INTERVAL_MSEC,
                                            app_timer_callback,
                                            NULL,
                                            0,
                                            0);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:

      app_log("[BLE]: Connection closed.\n");
      connection = 0;
      sl_sleeptimer_stop_timer(&app_timer);
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to generate advertising data.\n",
                 (int)sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising.\n",
                 (int)sc);
      app_log("[BLE]: Start advertising ...\n");
      break;
    case sl_bt_evt_system_external_signal_id:
      app_read_temp_press();
      break;
    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_temperature) {
        // client characteristic configuration changed by remote GATT client
        if (evt->data.evt_gatt_server_characteristic_status.status_flags
            == gatt_server_client_config) {
          if (evt->data.evt_gatt_server_characteristic_status.
              client_config_flags == gatt_notification) {
            temperature_notify_enable = true;
            app_log("[BLE]: Enable temperature notification.\n");
          } else {
            temperature_notify_enable = false;
            app_log("[BLE]: Disable temperature notification.\n");
          }
        }
      } else if (evt->data.evt_gatt_server_characteristic_status.characteristic
                 == gattdb_pressure) {
        if (evt->data.evt_gatt_server_characteristic_status.status_flags
            == gatt_server_client_config) {
          if (evt->data.evt_gatt_server_characteristic_status.
              client_config_flags == gatt_notification) {
            pressure_notify_enable = true;
            app_log("[BLE]: Enable pressure notification.\n");
          } else {
            pressure_notify_enable = false;
            app_log("[BLE]: Disable pressure notification.\n");
          }
        }

        break;
          ///////////////////////////////////////////////////////////////////////////
          // Add additional event handlers here as your application requires!
          //        //
          ///////////////////////////////////////////////////////////////////////////

          // -------------------------------
          // Default event handler.
          default:
            break;
      }
  }
}

/**************************************************************************//**
 * Sleep timer callback function.
 *****************************************************************************/
static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                               void *data)
{
  (void)timer;
  (void)data;

  sl_bt_external_signal(APP_TIMER_EXT_SIGNAL);
}

/**************************************************************************//**
 * Read temperature and pressure function.
 *****************************************************************************/
static void app_read_temp_press(void)
{
  sl_status_t sc;
  float temperature, pressure;

  sc = mikroe_pressure3_get_t_p_data(&temperature, &pressure);
  if (sc != SL_STATUS_OK) {
    app_log("[DPS310]: Reading temperature and pressure failed.\n");
  } else {
    app_log("[DPS310]: Pressure: %.2f mbar\r\n", pressure);
    app_log("[DPS310]: Temperature: %.2f C\r\n", temperature);
    app_log(" ----------------------------\r\n");
  }

  app_gatt_update(temperature, pressure);
}

/**************************************************************************//**
 * Update GATT function.
 *****************************************************************************/
static void app_gatt_update(float temperature, float pressure)
{
  sl_status_t sc;
  uint8_t temperature_tmp[2];
  uint8_t pressure_tmp[4];

  *(uint16_t *)temperature_tmp = (uint16_t)(temperature * 100);

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_temperature,
                                               0,
                                               sizeof(temperature_tmp),
                                               (const uint8_t *)(temperature_tmp));

  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to update GATT temperature value.\n",
             (int)sc);

  *(uint32_t *)pressure_tmp = (uint32_t)pressure;
  sc = sl_bt_gatt_server_write_attribute_value(gattdb_pressure,
                                               0,
                                               sizeof(pressure_tmp),
                                               (const uint8_t *)(pressure_tmp));

  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to update GATT temperature value\n",
             (int)sc);

  if (connection) {
    if (temperature_notify_enable) {
      app_send_notification(temperature_tmp, gattdb_temperature);
    } else {
    }
  }
  if (pressure_notify_enable) {
    app_send_notification(pressure_tmp, gattdb_pressure);
  } else {
  }
}

/**************************************************************************//**
 * Send notification function.
 *****************************************************************************/
static void app_send_notification(uint8_t *value, uint16_t characteristic)
{
  sl_status_t sc;

  sc = sl_bt_gatt_server_send_notification(connection,
                                           characteristic,
                                           sizeof(value),
                                           (const uint8_t *)value);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to send a notification.\n",
             (int)sc);
}
