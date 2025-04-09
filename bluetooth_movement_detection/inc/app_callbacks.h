/***************************************************************************//**
 * @file app_callbacks.h
 * @brief Application Callbacks
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef APP_CALLBACKS_H_
#define APP_CALLBACKS_H_

#include "sl_sleeptimer.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief
 *    This function handles the last request timeout timer  expiration event.
 ******************************************************************************/
void app_sleep_timer_last_req_conf_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);

/**************************************************************************//**
 * Callback when the wake-up time period timer expire
 *****************************************************************************/
void app_sleep_timer_wakeup_period_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);

/**************************************************************************//**
 * Callback when the sensor's auto wake-up interrupt fires.
 *****************************************************************************/
void app_external_int_auto_wakeup_callback(
  uint8_t intNo);

/**************************************************************************//**
 * Callback when the blink led timer expire.
 *****************************************************************************/
void app_sleep_timer_blink_led_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);

/**************************************************************************//**
 * Callback when the notify timer expire.
 *****************************************************************************/
void app_sleep_timer_notify_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);

/**************************************************************************//**
 * Callback when the notify break timer expire.
 *****************************************************************************/
void app_sleep_timer_notify_break_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);

#ifdef __cplusplus
}
#endif

#endif /* APP_CALLBACKS_H_ */
