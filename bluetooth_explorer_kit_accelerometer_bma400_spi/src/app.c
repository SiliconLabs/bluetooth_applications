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
#include "app_log.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "bma400.h"

#define ACCEL_TIMER_HANDLE 0
#define ACCEL_TIMER_PERIOD 3276  // 100 ms

/* Earth's gravity in m/s^2 */
#define GRAVITY_EARTH     (9.80665f)

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint8_t connection_handle = 0xff;
static uint8_t indication_enabled = 0;

static sl_status_t bma400_conf_accelerometer(void);
static sl_status_t accelerometer_update_data(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  sl_status_t sc;

  sc = bma400_conf_accelerometer();
  if (sc != SL_STATUS_OK) {
    app_log_warning("Warning! Fail to initialize accel 5 click, reason: 0x%04x\r\n",
                    sc);
  }
  else {
    app_log_info("Initialize accel 5 click successful.\r\n");
  }
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
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
      // Print boot message.
      app_log_info("Bluetooth stack booted: v%d.%d.%d-b%d\n",
                   evt->data.evt_system_boot.major,
                   evt->data.evt_system_boot.minor,
                   evt->data.evt_system_boot.patch,
                   evt->data.evt_system_boot.build);

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

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

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("Connection opened\n");
      indication_enabled = 0;
      // Save connection handle for future reference
      connection_handle = evt->data.evt_connection_opened.connection;

      sc = sl_bt_system_set_soft_timer(ACCEL_TIMER_PERIOD,
                                       ACCEL_TIMER_HANDLE, 0);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("connection closed, reason: 0x%2.2x\r\n",
                    evt->data.evt_connection_closed.reason);
      indication_enabled = 0;
      connection_handle = 0xff;
      // Stop timer
      sc = sl_bt_system_set_soft_timer(0, ACCEL_TIMER_HANDLE, 0);
      app_assert_status(sc);
      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      break;

    case sl_bt_evt_system_soft_timer_id:
      if (evt->data.evt_system_soft_timer.handle == ACCEL_TIMER_HANDLE) {
        accelerometer_update_data();
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          != gattdb_acceleration) {
        break;
      }
      // client characteristic configuration changed by remote GATT client
      if (evt->data.evt_gatt_server_characteristic_status.status_flags
          != gatt_server_client_config) {
        break;
      }

      if (evt->data.evt_gatt_server_characteristic_status.client_config_flags
          == gatt_indication) {
        indication_enabled = 1;
      }
      else {
        indication_enabled = 0;
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
 * Initialize the bma400.
 *****************************************************************************/
static sl_status_t bma400_conf_accelerometer(void)
{
  bma400_sensor_conf_t conf;
  sl_status_t ret;

  ret = bma400_init();
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Get the accelerometer configurations which are set in the sensor */
  ret = bma400_get_sensor_conf(&conf, 1);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Select the type of configuration to be modified */
  conf.type = BMA400_ACCEL;

  /* Modify the desired configurations as per macros
   * available in bma400.h file */
  conf.param.accel.odr = BMA400_ODR_100HZ;
  conf.param.accel.range = BMA400_RANGE_2G;
  conf.param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;

  /* Set the desired configurations to the sensor */
  ret = bma400_set_sensor_conf(&conf, 1);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  ret = bma400_set_power_mode(BMA400_MODE_NORMAL);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Update the current accelerations from accel 5 click board.
 *****************************************************************************/
static sl_status_t accelerometer_update_data(void)
{
  sl_status_t ret;
  bma400_sensor_data_t accel_data; /* acceleration data */
  uint8_t accel_buffer[3];

  // Read the current accelerations from accel 5 click board
  ret = bma400_get_accel_data(BMA400_DATA_ONLY, &accel_data);
  if (ret != SL_STATUS_OK) {
    app_log_warning("Warning! Invalid Accelerometer reading\n");
    return ret;
  }

  // The axis values are absolute accelerations where 1 g is about value of 98
  if (accel_data.x > 0)
    accel_buffer[0] = accel_data.x / 10;
  else
    accel_buffer[0] = (-accel_data.x) / 10;

  if (accel_data.y > 0)
    accel_buffer[1] = accel_data.y / 10;
  else
    accel_buffer[1] = (-accel_data.y) / 10;

  if (accel_data.z > 0)
    accel_buffer[2] = accel_data.z / 10;
  else
    accel_buffer[2] = (-accel_data.z) / 10;

  // Write the value of an attribute in the local GATT database
  ret = sl_bt_gatt_server_write_attribute_value(gattdb_acceleration,
                                               0,
                                               3,
                                               accel_buffer);
  app_assert_status(ret);

  if (indication_enabled) {
    ret = sl_bt_gatt_server_send_indication(connection_handle,
                                           gattdb_acceleration,
                                           3,
                                           accel_buffer);
    app_assert_status(ret);
  }

  return SL_STATUS_OK;
}
