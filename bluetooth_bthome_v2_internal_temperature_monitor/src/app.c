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
#include "app.h"

#include "bthome_v2.h"
#include "tempdrv.h"
#include "app_log.h"
#include "sl_sleeptimer.h"

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
#define APP_TIMER_TIMEOUT      10000
#define APP_UPDATE_TEMPERATURE 0x01

/**************************************************************************//**
 * static variable.
 *****************************************************************************/
static int8_t temperature = 0;
static uint8_t dev_name[] = "Temp";
static uint8_t dev_key[] = "12345678912345678912345678912345";
static sl_sleeptimer_timer_handle_t app_timer_handle;

/**************************************************************************//**
 * static function.
 *****************************************************************************/
static void app_periodic_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                                  void *data);
static void app_external_signal_handle();

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  Ecode_t ret;
  sl_status_t sc;

  app_log("BTHome v2 - Internal Temperature Monitor - Application Init.\n");
  ret = TEMPDRV_Init();
  if (ret == ECODE_EMDRV_TEMPDRV_OK) {
    app_log("Temperature driver initialztion done.\n");
  }
  ret = TEMPDRV_Enable(true);
  app_assert_status(ret);

  // read temperature.
  temperature = TEMPDRV_GetTemp();

  sc = sl_sleeptimer_start_periodic_timer_ms(&app_timer_handle,
                                             APP_TIMER_TIMEOUT,
                                             app_periodic_timer_cb,
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
      sc = bthome_v2_init(dev_name, true, dev_key, false);
      app_assert_status(sc);

      bthome_v2_add_measurement_float(ID_TEMPERATURE, (float)temperature);
      sc = bthome_v2_send_packet();
      app_assert_status(sc);
      break;

    // external signal event handler
    case sl_bt_evt_system_external_signal_id:
      app_external_signal_handle();
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static void app_periodic_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                                  void *data)
{
  (void) handle;
  (void) data;

  sl_bt_external_signal(APP_UPDATE_TEMPERATURE);
}

static void app_external_signal_handle()
{
  temperature = TEMPDRV_GetTemp();

  app_log("temperature: %d\n", temperature);
  bthome_v2_reset_measurement();
  bthome_v2_add_measurement_float(ID_TEMPERATURE, (float)temperature);
  // change the advertising data
  bthome_v2_build_packet();
}
