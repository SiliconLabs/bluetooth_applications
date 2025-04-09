/***************************************************************************//**
 * @file co_client_oled_app.h
 * @brief co client oled application header file.
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
 * 1. The origin of this software must not be misrepresented{} you must not
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

#ifndef CO_CLIENT_OLED_APP_H_
#define CO_CLIENT_OLED_APP_H_

#include <stdint.h>

/***************************************************************************//**
 * @brief
 *    Client Oled Application Initialize.
 *
 ******************************************************************************/
void client_oled_app_init(void);

/***************************************************************************//**
 * @brief
 *    Client Oled Application Display Value Of Co Level
 *
 *  @param[in] ppm_value.
 *    CO value receive from the sensor.
 *
 *  @param[in] threshold_value.
 *    Threshold value
 *
 * @details
 *    This function shows the received PPM value from the sensor and threshold
 *    value for alarm
 *
 ******************************************************************************/
void client_oled_app_update_co_level_screen(uint32_t ppm_value,
                                            uint32_t threshold_value);

/***************************************************************************//**
 * @brief
 *    Client Oled Application Display Threshold
 *
 *  @param[in] threshold.
 *    Threshold receive from the user.
 *
 *  @param[in] alarm_status.
 *    Alarm status for buzzer.
 *
 * @details
 *    This function shows the received threshold value and alarm status.
 *
 ******************************************************************************/
void client_oled_app_update_threshold_screen(uint32_t threshold,
                                             uint8_t alarm_status);

#endif /* CO_CLIENT_OLED_APP_H_ */
