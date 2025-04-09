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

#ifndef NVM3_USER_CONFIG_H
#define NVM3_USER_CONFIG_H

/***************************************************************************//**
 * @brief
 *    Initialize the nvm3 module for the people counting configuration.
 *
 ******************************************************************************/
void user_config_nvm3_init(void);

/***************************************************************************//**
 * @brief
 *    Set mode to the nvm3 entry.
 *
 * @param[in] mode
 *    Mode to set.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_mode(uint8_t mode);

/***************************************************************************//**
 * @brief
 *    Get mode value from the nvm3 entry.
 *
 * @return
 *    Mode of application
 ******************************************************************************/
uint8_t user_config_nvm3_get_mode(void);

/***************************************************************************//**
 * @brief
 *    Set setpoint value to the nvm3 entry.
 *
 * @param[in] setpoint
 *    Setpoint value
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_setpoint(int16_t setpoint);

/***************************************************************************//**
 * @brief
 *    Get setpoint value from the nvm3 entry.
 *
 * @return
 *    Setpoint value
 ******************************************************************************/
int16_t user_config_nvm3_get_setpoint(void);

/***************************************************************************//**
 * @brief
 *    Set hysteresis value to the nvm3 entry.
 *
 * @param[in] hysteresis
 *    Hysteresis value
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_hysteresis(int16_t hysteresis);

/***************************************************************************//**
 * @brief
 *    Get hysteresis value from the nvm3 entry.
 *
 * @return
 *    Hysteresis value
 ******************************************************************************/
int16_t user_config_nvm3_get_hysteresis(void);

/***************************************************************************//**
 * @brief
 *    Set lower threshold value to the nvm3 entry.
 *
 * @param[in] lower_threshold
 *    lower_threshold value
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_lower_threshold(int16_t lower_threshold);

/***************************************************************************//**
 * @brief
 *    Get lower threshold value from the nvm3 entry.
 *
 * @return
 *    Lower threshold value
 ******************************************************************************/
int16_t user_config_nvm3_get_lower_threshold(void);

/***************************************************************************//**
 * @brief
 *    Set upper threshold value to the nvm3 entry.
 *
 * @param[in] upper_threshold
 *    upper_threshold value
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_upper_threshold(int16_t upper_threshold);

/***************************************************************************//**
 * @brief
 *    Get upper threshold value from the nvm3 entry.
 *
 * @return
 *    Upper threshold value
 ******************************************************************************/
int16_t user_config_nvm3_get_uppper_threshold(void);

/***************************************************************************//**
 * @brief
 *    Set alarm status to the nvm3 entry.
 *
 * @param[in] enable
 *    True or false
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_alarm_status(bool enable);

/***************************************************************************//**
 * @brief
 *    Get alarm status value from the nvm3 entry.
 *
 * @return
 *    Alarm status (true or false)
 ******************************************************************************/
bool user_config_nvm3_get_alarm_status(void);

#endif // NVM3_USER_CONFIG_H
