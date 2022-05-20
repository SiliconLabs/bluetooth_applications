/***************************************************************************//**
 * @file app_logic.h
 * @brief Application Logic
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
#ifndef APP_LOGIC_H_
#define APP_LOGIC_H_

#include <stdbool.h>
#include <stdint.h>
#include "sl_sleeptimer.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief
 *    Typedef for application runtime parameters.
 ******************************************************************************/
typedef struct {
  // movement counter increases if the device is still moving.
  uint16_t movement_counter;
  // sample counter increases each time if the device wakes up by timer.
  uint16_t sample_counter;
  // flag determines whether the device is moving or not.
  bool movement_flag;
  // Timer handler for request time.
  sl_sleeptimer_timer_handle_t last_req_conf_timer_handle;
  // Timer handler for wakeup time.
  sl_sleeptimer_timer_handle_t wake_up_period_timer_handle;
  // Timer handler for notification.
  sl_sleeptimer_timer_handle_t notify_timer_handle;
  // Timer handler for blink interval.
  sl_sleeptimer_timer_handle_t blink_led_timer_handle;
  // Timer handler for notification break time.
  sl_sleeptimer_timer_handle_t notify_break_timer_handle;
} md_runtime_data_t;

/***************************************************************************//**
 * @brief
 *    Typedef for device modes.
 ******************************************************************************/
typedef enum movement_detection_mode {
  MD_NORMAL_MODE = 0x00,     ///< Normal mode
  MD_CONFIGURED_MODE = 0x01  ///< Configuration mode
} movement_detection_mode_t;

/***************************************************************************//**
 * @brief
 *    This function initializes the external drivers, configures the
 *    accelerometer sensor and set the application parameters.
 ******************************************************************************/
void app_logic_init(void);

/***************************************************************************//**
 * @brief
 *    This function handle system boot event, check button 0 is pressed or
 *    not and determines device enter configure mode or normal mode.
 *
 * @return
 *  MD_CONFIGURED_MODE when button 0 is pressed, otherwise MD_NORMAL_MODE.
 ******************************************************************************/
movement_detection_mode_t app_logic_handle_system_boot_evt(void);

/***************************************************************************//**
 * @brief
 *    This function resets configuration time so that the configuration timeout
 *    is not exceeded.
 ******************************************************************************/
void app_logic_reset_last_req_conf_timer(void);

/***************************************************************************//**
 * @brief
 *    This function handles the wake-up event of accelerometer sensor.
 ******************************************************************************/
void app_logic_handle_acc_wakeup_evt(void);

/***************************************************************************//**
 * @brief
 *    This function handles the wake-up time period event. It loads data from
 *    accelerometer sensor's fifo and check whether the device is continuously
 *    moving or not.
 ******************************************************************************/
void app_logic_handle_wakeup_time_period_evt(void);

/***************************************************************************//**
 * @brief
 *    This function handles notification timer expired event. It stops blinking
 *    notification and start sensor's breaking time.
 ******************************************************************************/
void app_logic_handle_notify_timer(void);

/***************************************************************************//**
 * @brief
 *    This function handles the expired event of blink led timer. It toggles
 *    the led0 state.
 ******************************************************************************/
void app_logic_handle_blink_led_periodic_timer(void);

/***************************************************************************//**
 * @brief
 *    This function handles the expired event of notification breaking timer.
 *    It enables accelerometer's auto wake-up interrupt again.
 ******************************************************************************/
void app_logic_handle_notify_break_timer(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LOGIC_H_ */
