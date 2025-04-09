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
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "sl_sleeptimer.h"
#include "mikroe_mlx90632.h"
#include "sl_i2cspm_instances.h"
#include "app_log.h"
#include "sl_health_thermometer.h"

#define APP_SENSOR_GET_AND_INDICATE_GATTDB 0x01
#define APP_TIMER_INTERVAL                 1000

/**************************************************************************//**
 * static variables.
 *****************************************************************************/
// Connection handle.
static uint8_t app_connection = 0;
// Periodic timer handle.
static sl_sleeptimer_timer_handle_t app_periodic_timer;
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * static function.
 *****************************************************************************/
// Measuring
static sl_status_t sensor_get(uint32_t *ambient_temp, int32_t *object_temp);

// Periodic timer callback.
static void app_periodic_timer_cb(sl_sleeptimer_timer_handle_t *timer,
                                  void *data);

// external signal handler.
static void app_sensor_get_and_indicate_gattdb(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  app_log("Bluetooth - IR thermostate MLX90632 sensor.\n");

  // Initialized the driver.
  if (mikroe_mlx90632_init(sl_i2cspm_mikroe) == SL_STATUS_OK) {
    app_log("IrThermo sensor initializes successfully\n");
  }

  mikroe_mlx90632_default_config();
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
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened.\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);

      // Stop timer.
      sc = sl_sleeptimer_stop_timer(&app_periodic_timer);
      app_assert_status(sc);
      break;
    case sl_bt_evt_system_external_signal_id:
      app_sensor_get_and_indicate_gattdb();
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void sl_bt_ht_temperature_measurement_indication_changed_cb(uint8_t connection,
                                                            sl_bt_gatt_client_config_flag_t client_config)
{
  sl_status_t sc;
  app_connection = connection;

  // Indication or notification enabled.
  if (sl_bt_gatt_disable != client_config) {
    // Start timer used for periodic indications.
    sc = sl_sleeptimer_start_periodic_timer_ms(&app_periodic_timer,
                                               APP_TIMER_INTERVAL,
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
    sl_sleeptimer_stop_timer(&app_periodic_timer);
  }
}

static void app_periodic_timer_cb(sl_sleeptimer_timer_handle_t *timer,
                                  void *data)
{
  (void)data;
  (void)timer;
  sl_bt_external_signal(APP_SENSOR_GET_AND_INDICATE_GATTDB);
}

static void app_sensor_get_and_indicate_gattdb(void)
{
  sl_status_t sc;
  int32_t object = 0;
  uint32_t ambient = 0;

  // Measure temperature; units are % and milli-Celsius.
  sc = sensor_get(&ambient, &object);
  if (sc != SL_STATUS_OK) {
    app_log("Warning! Invalid reading: %lu %ld\n", ambient, object);
  }

  // Send temperature measurement indication to connected client.
  sc = sl_bt_ht_temperature_measurement_indicate(app_connection,
                                                 object,
                                                 false);

  if (sc != SL_STATUS_OK) {
    app_log("Warning! Failed to send temperature measurement indication\n");
  }
}

static sl_status_t sensor_get(uint32_t *ambient_temp, int32_t *object_temp)
{
  float amb, obj;

  // Perform the measurement
  if (SL_STATUS_OK != mikroe_mlx90632_present()) {
    app_log("IrThermo 3 Click is not present on the bus\n");
    return SL_STATUS_FAIL;
  }

  amb = mikroe_mlx90632_get_ambient_temperature();
  obj = mikroe_mlx90632_get_object_temperature();

  *ambient_temp = (uint32_t)(amb * 1000);
  *object_temp = (int32_t)(obj * 1000);

  app_log("ambient: %.2f\n", amb);
  app_log("Object: %.2f\n", obj);

  return SL_STATUS_OK;
}
