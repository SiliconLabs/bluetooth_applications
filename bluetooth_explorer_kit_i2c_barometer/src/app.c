/***************************************************************************//**
 * @file app.c
 * @brief Explorer Kit Bluetooth barometer example using I2C bus DPS310 pressure sensor
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include "em_common.h"
#include "sl_app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "barometer.h"      // Added for the barometer driver
#include "em_cmu.h"         // Added for GPIO LED control

#define BAROMETER_SOFT_TIMER_HANDLE 0
#define BAROMETER_SOFT_TIMER_PERIOD 32768   // 32768 = 1 second

#define ENABLE_DEBUG_LED            1       // 1=LED blink on, 0=LED not used

// Refresh the local database with the acquired sensor value and send it via BLE indications
static void sendPressure(float pressure);
// User callback function for the nonblocking sensor function
void barometer_callback(float pressure);

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// If the notification is enabled or not
static uint8_t notification_enabled = 0;
static int16_t connection = -1;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  sl_status_t sc;
  barometer_init_t init = BAROMETER_INIT_DEFAULT;

#if ENABLE_DEBUG_LED
  /* DEBUG */
  CMU_ClockEnable(cmuClock_GPIO, true); /* Enable GPIO module clock */
  GPIO_PinModeSet(gpioPortA, 4, gpioModePushPull, 0); /* Set pin PA04 direction as push-pull output for LED control*/
  GPIO_PinOutSet(gpioPortA, 4); /* Set PA04 to turn LED On to indicate barometer init start */
#endif

  sc = barometer_init(&init);

  sl_app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to initialize the barometer\n",
                (int)sc);

#if ENABLE_DEBUG_LED
  GPIO_PinOutClear(gpioPortA, 4); /* Clear PA04 to turn LED Off to indicate successfull barometer init */
#endif

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
      sl_app_assert(sc == SL_STATUS_OK,
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
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to write attribute\n",
                    (int)sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to create advertising set\n",
                    (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set advertising timing\n",
                    (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:

      notification_enabled = 0;

      connection = evt->data.evt_connection_opened.connection;

      sc = sl_bt_system_set_soft_timer(BAROMETER_SOFT_TIMER_PERIOD, BAROMETER_SOFT_TIMER_HANDLE, 0);

      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start soft timer\n",
                    (int)sc);

      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:

      notification_enabled = 0;

      connection = -1;

      sc = sl_bt_system_set_soft_timer(0, BAROMETER_SOFT_TIMER_HANDLE, 0);

      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to stop soft timer\n",
                    (int)sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_system_soft_timer_id:

      if (evt->data.evt_system_soft_timer.handle == BAROMETER_SOFT_TIMER_HANDLE) {

#if ENABLE_DEBUG_LED
        GPIO_PinOutSet(gpioPortA, 4); /* Set PA04 to turn LED On to indicate barometer reading start */
#endif

        // Start the measurement and subscribe the callback function
        sc = barometer_get_pressure_non_blocking(barometer_callback);

        sl_app_assert(sc == SL_STATUS_OK,
                      "[E: 0x%04x] Failed to start pressure measurement\n",
                      (int)sc);

      }

      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:

      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_envsens_pressure) {
        // client characteristic configuration changed by remote GATT client
        if (evt->data.evt_gatt_server_characteristic_status.status_flags == gatt_server_client_config) {
          if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == gatt_notification) {
            notification_enabled = 1;
          }
          else {
            notification_enabled = 0;
          }
        }
      }

      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void barometer_callback(float pressure)
{

#if ENABLE_DEBUG_LED
  GPIO_PinOutClear(gpioPortA, 4); /* Clear PA04 to turn LED Off to indicate barometer reading end */
#endif

  sendPressure(pressure);
}

static void sendPressure(float pressure)
{
  uint8_t pressure_buffer[4];
  sl_status_t sc;

  *(uint32_t *)pressure_buffer = (uint32_t)pressure;

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_envsens_pressure, 0, 4, pressure_buffer);

  sl_app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to update GATT pressure value\n",
                (int)sc);

  if ((notification_enabled == 1) && (connection != -1))
  {
    sc = sl_bt_gatt_server_send_notification(connection, gattdb_envsens_pressure, 4, pressure_buffer);

    sl_app_assert(sc == SL_STATUS_OK,
                     "[E: 0x%04x] Failed to send a pressure value notification\n",
                     (int)sc);
  }
}
