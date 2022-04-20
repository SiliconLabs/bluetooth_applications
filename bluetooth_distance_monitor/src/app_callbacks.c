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
#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"
#include "sl_sleeptimer.h"

#include "app_callbacks.h"
#include "app_ble_events.h"

/**************************************************************************//**
 * Handler for the button
 *****************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if ((handle == &sl_button_btn0)
      && (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED)) {
    // Send the external event to the BLE stack
    sl_bt_external_signal(DISTANCE_MONITOR_BUTTON_EVENT);
  }
}

/**************************************************************************//**
 * Callback when the display sleep timer expire
 *****************************************************************************/
void app_sleep_timer_main_callback(sl_sleeptimer_timer_handle_t *handle,
                                   void *data)
{
  (void) handle;
  (void) data;
  // Send the external event to the BLE stack
  sl_bt_external_signal(DISTANCE_MONITOR_TIMER_EVENT);
}

/**************************************************************************//**
 * Callback when the main sleep timer expire
 *****************************************************************************/
void app_sleep_timer_display_callback(sl_sleeptimer_timer_handle_t *handle,
                                      void *data)
{
  (void) handle;
  (void) data;
  // Send the external event to the BLE stack
  sl_bt_external_signal(DISTANCE_MONITOR_DISPLAY_EVENT);
}
