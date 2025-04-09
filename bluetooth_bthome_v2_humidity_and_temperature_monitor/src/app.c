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
#include "sl_sleeptimer.h"
#include "sl_i2cspm_instances.h"
#include "app_log.h"
#include "app.h"
#include <math.h>
#include "mikroe_shtc3.h"
#include "bthome_v2.h"

#define APP_READING_INTERVAL 10000
#define APP_TIMER_EXT_SIGNAL 0x01

static uint8_t device_name[] = "HumTemp";
static uint8_t device_key[] = "11112222333344445555666677778888";

static sl_sleeptimer_timer_handle_t app_timer;
static mikroe_shtc3_measurement_data_t measurement_data;

static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                               void *data);
static void app_timer_external_signal_handle(void);
static void rounding_and_add_measurement(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  app_log("BTHome v2 - Humidity and Temperature Monitor\n");
  app_log("======== Application initialization =========\n");

  if (!(mikroe_shtc3_init(sl_i2cspm_qwiic))) {
    app_log("[SHTC3]: SparkFun Humidity Sensor initializes successfully.\n");

    mikroe_shtc3_send_command(MIKROE_SHTC3_CMD_SLEEP);
    sl_sleeptimer_delay_millisecond(500);

    mikroe_shtc3_send_command(MIKROE_SHTC3_CMD_WAKEUP);
    sl_sleeptimer_delay_millisecond(100);
  } else {
    app_log("[Error]:  SparkFun Humidity Sensor initializes failed.\n");
    app_log("[Error]:  Please, check the cable.\n");
    return;
  }

  app_log("=============== Start measuring ===============\n");

  sc = mikroe_shtc3_get_temperature_and_humidity(SHTC3_DATA_MODE_NORMAL,
                                                 &measurement_data);
  app_assert_status(sc);

  sl_sleeptimer_start_periodic_timer_ms(&app_timer,
                                        APP_READING_INTERVAL,
                                        app_timer_callback,
                                        NULL,
                                        0,
                                        0);
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

  bthome_v2_bt_on_event(evt);

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      sc = bthome_v2_init(device_name, true, device_key, false);
      app_assert_status(sc);

      rounding_and_add_measurement();
      sc = bthome_v2_send_packet();
      app_assert_status(sc);
      break;

    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          == APP_TIMER_EXT_SIGNAL) {
        app_timer_external_signal_handle();
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Rounding and add measured values function.
 *****************************************************************************/
static void rounding_and_add_measurement(void)
{
  app_log("[SHTC3]: Temperature: %.1f oC, Humidity: %.2f %%\n",
          measurement_data.temperature,
          measurement_data.humidity);

  measurement_data.humidity = round(measurement_data.humidity * 100) / 100;
  measurement_data.temperature = round(measurement_data.temperature * 10) / 10;

  bthome_v2_add_measurement_float(ID_HUMIDITY_PRECISE,
                                  measurement_data.humidity);
  bthome_v2_add_measurement_float(ID_TEMPERATURE,
                                  measurement_data.temperature);
}

/**************************************************************************//**
 * Application timer callback function.
 *****************************************************************************/
static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                               void *data)
{
  (void) timer;
  (void) data;

  sl_bt_external_signal(APP_TIMER_EXT_SIGNAL);
}

/**************************************************************************//**
 * Handle external signal function.
 *****************************************************************************/
static void app_timer_external_signal_handle(void)
{
  sl_status_t sc;

  sc = mikroe_shtc3_get_temperature_and_humidity(SHTC3_DATA_MODE_NORMAL,
                                                 &measurement_data);
  app_assert_status(sc);

  bthome_v2_reset_measurement();
  rounding_and_add_measurement();
  bthome_v2_build_packet();
}
