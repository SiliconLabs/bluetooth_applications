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
#include "app.h"
#include "app_log.h"
#include "sl_i2cspm_instances.h"
#include "bthome_v2.h"
#include "mikroe_pl_n823_01.h"

#define MOTION_OBJECT_ID          0x21
#define PROCESS_DATA_EXT_SIGNAL   0x01
#define APP_TIMER_TIMEOUT         1000
#define PIR_VOLTAGE_OUT_THRESHOLD 10

#define PIR_MAP_OUT_MIN_MV        0
#define PIR_MAP_OUT_MAX_MV        3300

static uint8_t device_name[] = "PIRAlarm";
static uint8_t device_key[] = "11112222333344445555666677778888";

static sl_sleeptimer_timer_handle_t app_timer;

static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                               void *data);
static void process_data_signal_handler(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  app_log("====== BTHome v2 - PIR Alarm Example ========\n");

  sc = bthome_v2_init(device_name, true, device_key, false);
  app_assert_status(sc);
  app_log("==        BTHome v2 initialization         ==\n");

  sc = mikroe_pl_n823_01_init(sl_i2cspm_mikroe);
  app_assert_status(sc);
  app_log("==     MikroE PIR Click initialization     ==\n");
  app_log("=============================================\n");
  sc = sl_sleeptimer_start_periodic_timer_ms(&app_timer,
                                             APP_TIMER_TIMEOUT,
                                             app_timer_callback,
                                             NULL,
                                             0,
                                             0);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  bthome_v2_bt_on_event(evt);
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      bthome_v2_add_measurement(MOTION_OBJECT_ID, 0);
      bthome_v2_send_packet();
      break;

    case sl_bt_evt_system_external_signal_id:
      process_data_signal_handler();
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * Process sensor data signal.
 *****************************************************************************/
static void process_data_signal_handler(void)
{
  uint16_t adc_val;
  float map_out;
  bool motion;
  static bool prev_motion;

  adc_val = mikroe_pl_n823_01_get_adc();
  map_out = mikroe_pl_n823_01_scale_results(adc_val,
                                            PIR_MAP_OUT_MIN_MV,
                                            PIR_MAP_OUT_MAX_MV);

  motion = map_out < PIR_VOLTAGE_OUT_THRESHOLD ? true : false;
  if (motion) {
    app_log("[INFO]: Motion DETECTED.\n");
  }
  if (motion != prev_motion) {
    bthome_v2_reset_measurement();
    bthome_v2_add_measurement(MOTION_OBJECT_ID, motion);
    bthome_v2_send_packet();
  }
  prev_motion = motion;
}

/**************************************************************************//**
 * App timer callback.
 *****************************************************************************/
static void app_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                               void *data)
{
  (void) timer;
  (void) data;

  sl_bt_external_signal(PROCESS_DATA_EXT_SIGNAL);
}
