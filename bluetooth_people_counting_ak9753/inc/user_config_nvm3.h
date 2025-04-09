/***************************************************************************//**
 * @file user_config_nvm3.h
 * @brief NVM3 application code
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

#ifndef USER_CONFIG_NVM3_H_
#define USER_CONFIG_NVM3_H_

#include <stdbool.h>

/***************************************************************************//**
 * @brief
 *    Initialize the nvm3 module for the people counting configuration.
 *
 ******************************************************************************/
void user_config_nvm3_init(void);

/***************************************************************************//**
 * @brief
 *    Set room capacity to the nvm3 entry.
 *
 * @param[in] room_capacity
 *    Room capacity value.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_room_capacity(uint16_t room_capacity);

/***************************************************************************//**
 * @brief
 *    Get room capacity value from the nvm3 entry.
 *
 * @return
 *    The value of room capacity.
 ******************************************************************************/
uint16_t user_config_nvm3_get_room_capacity(void);

/***************************************************************************//**
 * @brief
 *    Set people entered the room so far value to the nvm3 entry.
 *
 * @param[in] people_count
 *    People entered the room so far value to set.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_people_entered_so_far(uint32_t people_count);

/***************************************************************************//**
 * @brief
 *    Get people entered the room so far value from the nvm3 entry.
 *
 * @return
 *    The value of people entered the room so far.
 ******************************************************************************/
uint32_t user_config_nvm3_get_people_entered_so_far(void);

/***************************************************************************//**
 * @brief
 *    Set lower threshold to the nvm3 entry.
 *
 * @return
 *    SL_STATUS_OK if success.
 *    SL_STATUS_FAIL if failed.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_lower_threshold(int16_t threshold);

/***************************************************************************//**
 * @brief
 *    Get lower threshold from the nvm3 entry.
 *
 * @return
 *    Threshold value.
 ******************************************************************************/
int16_t user_config_nvm3_get_lower_threshold(void);

/***************************************************************************//**
 * @brief
 *    Set upper threshold to the nvm3 entry.
 *
 * @return
 *    SL_STATUS_OK if success.
 *    SL_STATUS_FAIL if failed.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_upper_threshold(int16_t threshold);

/***************************************************************************//**
 * @brief
 *    Get upper threshold from the nvm3 entry.
 *
 * @return
 *    Threshold value.
 ******************************************************************************/
int16_t user_config_nvm3_get_upper_threshold(void);

/***************************************************************************//**
 * @brief
 *    Set hysteresis to the nvm3 entry.
 *
 * @return
 *    SL_STATUS_OK if success.
 *    SL_STATUS_FAIL if failed.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_hysteresis(uint16_t hysteresis);

/***************************************************************************//**
 * @brief
 *    Get hysteresis from the nvm3 entry.
 *
 * @return
 *    Hysteresis value.
 ******************************************************************************/
uint16_t user_config_nvm3_get_hysteresis(void);

/***************************************************************************//**
 * @brief
 *    Set ir threshold to the nvm3 entry.
 *
 * @return
 *    SL_STATUS_OK if success.
 *    SL_STATUS_FAIL if failed.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_ir_threshold(int16_t threshold);

/***************************************************************************//**
 * @brief
 *    Get ir threshold from the nvm3 entry.
 *
 * @return
 *    Threshold value.
 ******************************************************************************/
int16_t user_config_nvm3_get_ir_threshold(void);

/***************************************************************************//**
 * @brief
 *    Set notification status to the nvm3 entry.
 *
 * @param[in] enable
 *    Enable or disable.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_notification_status(bool enable);

/***************************************************************************//**
 * @brief
 *    Get notification status value from the nvm3 entry.
 *
 * @return
 *    The value of notification status.
 ******************************************************************************/
bool user_config_nvm3_get_notification_status(void);

#endif /* USER_CONFIG_NVM3_H_ */
