/***************************************************************************//**
 * @file app_callbacks.c
 * @brief Application Callbacks Source File
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
 *
 * # EXPERIMENTAL QUALITY
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/
#include "app_log.h"
#include "sl_bt_api.h"

#include "sl_simple_led_instances.h"

#include "app_config.h"
#include "app_callbacks.h"
#include "app_ble_events.h"

/**************************************************************************//**
 * Callback when the last request timeout timer expire
 *****************************************************************************/
void app_sleep_timer_last_req_conf_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  // Send the external event to the BLE stack
  sl_bt_external_signal(MD_LAST_REQ_TIMEOUT_TIMER_EVENT);
}

/**************************************************************************//**
 * Callback when the wakeup time period timer expire
 *****************************************************************************/
void app_sleep_timer_wakeup_period_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  // Send the external event to the BLE stack
  sl_bt_external_signal(MD_WAKEUP_PERIOD_TIMER_EVENT);
}

/**************************************************************************//**
 * Callback when the sensor's auto wakeup interrupt fires.
 *****************************************************************************/
void app_external_int_auto_wakeup_callback(
  uint8_t intNo)
{
  if (intNo == BMA400_INT_PIN) {
    sl_bt_external_signal(MD_ACC_WAKEUP_EVENT);
  }
}

/**************************************************************************//**
 * Callback when the notify timer expire.
 *****************************************************************************/
void app_sleep_timer_notify_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  // Send the external event to the BLE stack
  sl_bt_external_signal(MD_NOTIFY_TIMER_EVENT);
}

/**************************************************************************//**
 * Callback when the blink led timer expire.
 *****************************************************************************/
void app_sleep_timer_blink_led_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  sl_simple_led_toggle(sl_led_led0.context);
}

/**************************************************************************//**
 * Callback when the notify break timer expire.
 *****************************************************************************/
void app_sleep_timer_notify_break_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  // Send the external event to the BLE stack
  sl_bt_external_signal(MD_NOTIFY_BREAK_TIMER_EVENT);
}
