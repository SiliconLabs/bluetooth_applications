/***************************************************************************//**
 * @file app_display.h
 * @brief Header file of app_display.c
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include <stdint.h>

/***************************************************************************//**
 * @addtogroup app_display
 * @brief app_display interface.
 * @{
 ******************************************************************************/

/**************************************************************************//**
 * @brief
 *  Initialize the app_display application.
 *
 *****************************************************************************/
void app_display_init(void);

/***************************************************************************//**
 * @brief
 *    Display sensor data.
 * @param rh
 *    Humidity sensor value integer part
 * @param rh_decimal
 *    Humidity sensor value decimal part with array size = 2
 * @param t
 *    Temperature sensor value integer part
 * @param t_decimal
 *    Temperature sensor value decimal part with array size = 2
 ******************************************************************************/
void app_display_show_sensor_rht_data(uint16_t rh,
                                      uint8_t rh_decimal[static 2],
                                      int16_t t,
                                      uint8_t t_decimal[static 2]);

/** @} */

#endif // APP_DISPLAY_H
