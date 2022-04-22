/***************************************************************************//**
 * @file user_config_nvm3.h
 * @brief NVM3 application code
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

#ifndef NVM3_APP_H
#define NVM3_APP_H

#include <stdbool.h>

/***************************************************************************//**
 * Initialize NVM3 example
 ******************************************************************************/
void user_config_nvm3_init(void);

sl_status_t user_config_nvm3_set_people_entered_so_far(uint32_t people_count);
sl_status_t user_config_nvm3_set_min_distance(uint16_t distance);
sl_status_t user_config_nvm3_set_max_distance(uint16_t distance);
sl_status_t user_config_nvm3_set_distance_threshold(uint16_t distance);
sl_status_t user_config_nvm3_set_timing_budget(uint16_t timing_budget);
sl_status_t user_config_nvm3_set_notification_status(bool enable);
sl_status_t user_config_nvm3_toggle_notification_status(void);
sl_status_t user_config_nvm3_set_room_capacity(uint16_t room_capacity);


uint32_t user_config_nvm3_get_people_entered_so_far(void);
uint16_t user_config_nvm3_get_min_distance(void);
uint16_t user_config_nvm3_get_max_distance(void);
uint16_t user_config_nvm3_get_distance_threshold(void);
uint16_t user_config_nvm3_get_timing_budget(void);
bool user_config_nvm3_get_notification_status(void);
uint16_t user_config_nvm3_get_room_capacity(void);

#endif  // NVM3_APP_H
