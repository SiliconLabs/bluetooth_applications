/***************************************************************************//**
 * @file vl53l1x_app.h
 * @brief Distance sampling and people counting module with VL53L1X Sensor
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef VL53L1X_APP_H
#define VL53L1X_APP_H

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void vl53l1_app_init(void);

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void vl53l1_app_process_action(void);

uint16_t vl53l1_app_get_current_measured_distance(void);

uint16_t vl53l1_app_get_people_count(void);
uint32_t vl53l1_app_get_people_entered_so_far(void);

void vl53l1_app_clear_people_count(void);
void vl53l1_app_clear_people_entered_so_far();

uint32_t vl53l1_app_get_measured_period_ms(void); // TODO: remove this

sl_status_t vl53l1_app_change_timing_budget_in_ms(uint16_t timing_budget);

#endif // VL53L1X_APP_H
