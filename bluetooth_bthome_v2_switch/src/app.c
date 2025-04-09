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
#include "em_emu.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "bthome_v2.h"
#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"

#define BUTTON0                      0

#define BUTTON_CHANGE_EXT_SIG        0x01
#define PROCESS_BUTTON_PRESS_EXT_SIG 0x02
#define BUTTON_NONE_EXT_SIG          0x03
#define ENTERING_EM2_EXT_SIG         0x04

#define SAMPLE_BUTTON_TIMEOUT        2000
#define BUTTON_NONE_TIMEOUT          2000
#define ENTERING_EM2_TIMEOUT         5000

static uint8_t button_type = EVENT_BUTTON_NONE;
static uint8_t device_name[] = "BTSwitch";
static uint8_t device_key[] = "11112222333344445555666677778888";

static sl_sleeptimer_timer_handle_t sample_button_timer;
static sl_sleeptimer_timer_handle_t button_none_timer;
static sl_sleeptimer_timer_handle_t entering_em2_timer;

static void sample_button_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                         void *data);
static void button_none_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                       void *data);

static void entering_em2_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                        void *data);

static void button_change_external_signal_handler(void);
static void process_button_press_external_signal_handler(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  app_log("===== BTHome v2 - Switch Application =====\n");
  app_log("======= BTHome v2 initialization =========\n");

  sc = bthome_v2_init(device_name, true, device_key, true);
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2);
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
      bthome_v2_add_measurement_state(EVENT_BUTTON, button_type, 0);
      bthome_v2_send_packet();
      app_log("==== Entering EM2 after 5 seconds ... ====\n");
      sc = sl_sleeptimer_start_timer_ms(&entering_em2_timer,
                                        ENTERING_EM2_TIMEOUT,
                                        entering_em2_timer_callback,
                                        NULL,
                                        0,
                                        0);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          == BUTTON_CHANGE_EXT_SIG) {
        button_change_external_signal_handler();
      } else if (evt->data.evt_system_external_signal.extsignals
                 == PROCESS_BUTTON_PRESS_EXT_SIG) {
        process_button_press_external_signal_handler();

        bthome_v2_reset_measurement();
        bthome_v2_add_measurement_state(EVENT_BUTTON, button_type, 0);
        bthome_v2_send_packet();

        app_log("[BTHome]: Send %s event.\n",
                (button_type == EVENT_BUTTON_PRESS) ? "BUTTON_PRESS"
                :((button_type
                   == EVENT_BUTTON_DOUBLE_PRESS) ? "BUTTON_DOUBLE_PRESS"
                  :((button_type
                     == EVENT_BUTTON_TRIPLE_PRESS) ? "BUTTON_TRIPLE_PRESS"
                    :((button_type
                       == EVENT_BUTTON_LONG_PRESS) ? "BUTTON_LONG_PRESS"
                      :"INVALID"))));

        button_type = EVENT_BUTTON_NONE;

        sc = sl_sleeptimer_start_timer_ms(&button_none_timer,
                                          BUTTON_NONE_TIMEOUT,
                                          button_none_timer_callback,
                                          NULL,
                                          0,
                                          0);
        app_assert_status(sc);
      } else if (evt->data.evt_system_external_signal.extsignals
                 == BUTTON_NONE_EXT_SIG) {
        if (button_type == EVENT_BUTTON_NONE) {
          app_log("[BTHome]: Send BUTTON_NONE event.\n");

          bthome_v2_reset_measurement();
          bthome_v2_add_measurement_state(EVENT_BUTTON, button_type, 0);
          bthome_v2_build_packet();

          sc = sl_sleeptimer_start_timer_ms(&entering_em2_timer,
                                            ENTERING_EM2_TIMEOUT,
                                            entering_em2_timer_callback,
                                            NULL,
                                            0,
                                            0);
          app_assert_status(sc);
        }
      } else if (evt->data.evt_system_external_signal.extsignals
                 == ENTERING_EM2_EXT_SIG) {
        app_log("[BTHome]: Stop advertising and entering EM2.\n");
        bthome_v2_stop();
      }
      break;
    default:
      break;
  }
}

/**************************************************************************//**
 * Button change callback function.
 *****************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  (void)handle;

  if (sl_button_get_state(SL_SIMPLE_BUTTON_INSTANCE(BUTTON0))
      == SL_SIMPLE_BUTTON_PRESSED) {
    sl_bt_external_signal(BUTTON_CHANGE_EXT_SIG);
  }
}

/**************************************************************************//**
 * Sample button sleeptimer callback function.
 *****************************************************************************/
static void sample_button_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                         void *data)
{
  (void)timer;
  (void)data;

  sl_bt_external_signal(PROCESS_BUTTON_PRESS_EXT_SIG);
}

/**************************************************************************//**
 * Button change external signal handler.
 *****************************************************************************/
static void button_change_external_signal_handler(void)
{
  sl_status_t sc;
  bool is_timer_running;

  button_type = ((++button_type) >= EVENT_BUTTON_TRIPLE_PRESS)
                ? EVENT_BUTTON_TRIPLE_PRESS : button_type;

  sc = sl_sleeptimer_is_timer_running(&button_none_timer, &is_timer_running);
  app_assert_status(sc);
  if (is_timer_running) {
    sc = sl_sleeptimer_stop_timer(&button_none_timer);
    app_assert_status(sc);
  }

  sc = sl_sleeptimer_is_timer_running(&entering_em2_timer, &is_timer_running);
  app_assert_status(sc);
  if (is_timer_running) {
    sc = sl_sleeptimer_stop_timer(&entering_em2_timer);
    app_assert_status(sc);
  }

  sc = sl_sleeptimer_is_timer_running(&sample_button_timer, &is_timer_running);
  app_assert_status(sc);
  if (!is_timer_running) {
    sc = sl_sleeptimer_start_timer_ms(&sample_button_timer,
                                      SAMPLE_BUTTON_TIMEOUT,
                                      sample_button_timer_callback,
                                      NULL,
                                      0,
                                      0);
    app_assert_status(sc);
  }
}

/**************************************************************************//**
 * Process button press external signal handler.
 *****************************************************************************/
static void process_button_press_external_signal_handler(void)
{
  sl_sleeptimer_stop_timer(&sample_button_timer);

  if (button_type == EVENT_BUTTON_PRESS) {
    button_type =
      (sl_button_get_state(SL_SIMPLE_BUTTON_INSTANCE(BUTTON0))
       == SL_SIMPLE_BUTTON_PRESSED) ? EVENT_BUTTON_LONG_PRESS
      : EVENT_BUTTON_PRESS;
  }
}

/**************************************************************************//**
 * Button none sleeptimer callback function.
 *****************************************************************************/
static void button_none_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                       void *data)
{
  (void) timer;
  (void) data;

  sl_bt_external_signal(BUTTON_NONE_EXT_SIG);
}

/**************************************************************************//**
 * Entering EM2 sleeptimer callback function.
 *****************************************************************************/
static void entering_em2_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                        void *data)
{
  (void)timer;
  (void)data;

  sl_bt_external_signal(ENTERING_EM2_EXT_SIG);
}
