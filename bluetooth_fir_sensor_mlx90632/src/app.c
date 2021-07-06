/***************************************************************************//**
 * @file
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "sl_simple_timer.h"
#include "sl_i2cspm.h"
#include "app_log.h"
#include <mlx90632.h>
#include <mlx90632_i2c.h>
#include "sl_health_thermometer.h"
#include "stdio.h"

// Connection handle.
static uint8_t app_connection = 0;

// Periodic timer handle.
static sl_simple_timer_t app_periodic_timer;

// Periodic timer callback.
static void app_periodic_timer_callback(sl_simple_timer_t *timer, void *data);

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

//Measuring
static sl_status_t sensor_get(uint32_t *rh, int32_t *t);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  //Initialized the driver.
  mlx90632_init();
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
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to get Bluetooth address\n",
                    (int)sc);

      // Pad and reverse unique ID to get System ID.
      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                   0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to write attribute\n",
                    (int)sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to create advertising set\n",
                    (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set advertising timing\n",
                    (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);

      // Stop timer.
      sc = sl_simple_timer_stop(&app_periodic_timer);
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to stop periodic timer\n",
                    (int)sc);
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

void sl_bt_ht_temperature_measurement_indication_changed_cb(uint8_t connection,
                                                            gatt_client_config_flag_t client_config)
{
  sl_status_t sc;
  app_connection = connection;

  // Indication or notification enabled.
  if (gatt_disable != client_config) {
    // Start timer used for periodic indications.
    sc = sl_simple_timer_start(&app_periodic_timer,
                               1000,
                               app_periodic_timer_callback,
                               NULL,
                               true);
    app_assert(sc == SL_STATUS_OK,
                  "[E: 0x%04x] Failed to start periodic timer\n",
                  (int)sc);
    // Send first indication.
    app_periodic_timer_callback(&app_periodic_timer, NULL);
  }
  // Indications disabled.
  else {
    // Stop timer used for periodic indications.
    (void)sl_simple_timer_stop(&app_periodic_timer);
  }
}

static void app_periodic_timer_callback(sl_simple_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
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

  if (sc) {
    app_log("Warning! Failed to send temperature measurement indication\n");
  }
}

static sl_status_t sensor_get(uint32_t *rh, int32_t *t)
{
  sl_status_t sc = SL_STATUS_OK;
  double  ambient, object;
  float float_object, float_ambient;
  int amb, obj;

  // Perform the measurement
  sc = mlx90632_measurment_cb(&ambient, &object);

  amb = ambient * 100;
  obj = object  * 100;

  float_ambient = (float)ambient*1000;
  (*rh) = (int32_t)float_ambient;
  printf("Ambient: %d.%d C\n", (int)(amb/100), (int)(amb%100));

  float_object = (float)object*1000;
  (*t) = (int32_t)float_object;
  printf("Object: %d.%d C\n", (int)(obj/100), (int)(obj%100));

  return sc;
}
