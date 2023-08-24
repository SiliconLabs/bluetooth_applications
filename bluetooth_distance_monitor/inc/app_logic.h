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
 * @addtogroup Application
 * @{
 *
 * @brief
 *  Implementation of a distance monitor application.
 *
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    Typedef for holding display parameters.
 ******************************************************************************/
typedef struct {
  bool is_animation_active; ///< Threshold parameter scrolling animation status
  uint8_t param_text[32]; ///< Threshold parameter text
  uint8_t *first_letter; ///<  Actual first letter of the param_text
  uint8_t max_char_per_line; ///< Maximum length of characters to be displayed
  uint8_t text_length; ///< Length of the param_text
  ///< Timer handler for screen update
  sl_sleeptimer_timer_handle_t timer_handle;
} distance_monitor_display_data_t;

/***************************************************************************//**
 * @brief
 *    Typedef for application runtime parameters.
 ******************************************************************************/
typedef struct {
  distance_monitor_display_data_t display; ///< Display runtime data
  uint16_t calculated_average_distance; ///< Calculated average distance in mm
  bool is_notification_active; ///< Notification status
  ///< Flag to indicate average value is out of range
  bool is_distance_out_of_range;
  ///< Flag: Buffer is full, average distance can be displayed and used for
  /// calculations.
  bool is_sensor_ready;
  ///< Timer handler for Application logic
  sl_sleeptimer_timer_handle_t main_timer_handle;
} distance_monitor_runtime_data_t;

/***************************************************************************//**
 * @brief
 *    This function initializes the external drivers, configures the distance
 *    sensor and set the application parameters.
 ******************************************************************************/
void app_logic_init(void);

/***************************************************************************//**
 * @brief
 *    Application logic main function. This function is invoked periodically to
 *    gather measurement result, process it, and check the calculated average
 *    distance against the configured thresholds.
 ******************************************************************************/
void app_logic_main_function(void);

/***************************************************************************//**
 * @brief
 *    This function initializes and configures the magnetic buzzer.
 ******************************************************************************/
void app_logic_init_buzzer(void);

/***************************************************************************//**
 * @brief
 *    This function initializes and configures the OLED display, draw
 *    Silabs logo on the screen, draw main screen and setups a periodic timer
 *    for screen updates.
 ******************************************************************************/
void app_logic_init_display(void);

/***************************************************************************//**
 * @brief
 *    Initialization function for the distance sensor. This function waits until
 *    the sensor boots up, and initialize, configure it once the sensor is
 *    ready to be configured.
 ******************************************************************************/
void app_logic_init_distance_sensor(void);

/***************************************************************************//**
 * @brief
 *   This function configures the range mode of the distance sensor.
 ******************************************************************************/
void app_logic_configure_distance_sensor_range_mode(void);

/***************************************************************************//**
 * @brief
 *   This function configures the volume of the magnetic buzzer.
 ******************************************************************************/
void app_logic_configure_buzzer_volume(void);

/***************************************************************************//**
 * @brief
 *   This function configures the operation status of the notification logic.
 ******************************************************************************/
void app_logic_configure_notification_status(bool toggle_status);

/***************************************************************************//**
 * @brief
 *   This function draws threshold parameters on the screen.
 ******************************************************************************/
void app_logic_draw_parameter_line(void);

/***************************************************************************//**
 * @brief
 *   This function called periodically to update screen on the OLED display.
 *   Updates the measured values, scroll the threshold line, and display
 *   messages in case of need.
 ******************************************************************************/
void app_logic_on_update_display(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* APP_LOGIC_H_ */
