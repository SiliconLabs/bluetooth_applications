/***************************************************************************//**
 * @file nvm3_user.h
 * @brief Define driver structures and APIs for the NVM3
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

#ifndef NVM3_USER_H
#define NVM3_USER_H

#include "nvm3_default.h"

/***************************************************************************//**
 * @addtogroup nvm3_user
 * @brief nvm3_user interface.
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *  Initialize the NVM3.
 *
 ******************************************************************************/
void nvm3_user_init(void);

/***************************************************************************//**
 * @brief
 *  Set the notification status to NVM.
 *
 * @param[in] enable
 *   Status of notification.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_set_notification_active(uint8_t enable);

/***************************************************************************//**
 * @brief
 *  Set the update period in seconds value to NVM.
 *
 * @param[in] period_sec
 *   Data to write.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_set_update_period(uint8_t period_sec);

/***************************************************************************//**
 * @brief
 *  Set the buzzer volume value to NVM.
 *
 * @param[in] volume
 *   Data to write.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_set_buzzer_volume(uint8_t volume);

/***************************************************************************//**
 * @brief
 *  Set the notification threshold for CO2 level in ppm to NVM.
 *
 * @param[in] threshold_co2
 *   Data to write.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_set_threshold_co2(uint16_t threshold_co2);

/***************************************************************************//**
 * @brief
 *  Set the notification threshold for TVOC level in ppb to NVM.
 *
 * @param[in] threshold_tvoc
 *   Data to write.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_set_threshold_tvoc(uint16_t threshold_tvoc);

/***************************************************************************//**
 * @brief
 *  Get the notification status from NVM.
 *
 * @param[out] enable
 *   Status of notification.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_get_notification_active(uint8_t *enable);

/***************************************************************************//**
 * @brief
 *  Get the Air quality monitor sleep timer period in seconds from NVM.
 *
 * @param[out] update_period
 *   The Air quality monitor sleep timer period in seconds from NVM.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_get_update_period(uint8_t *update_period);

/***************************************************************************//**
 * @brief
 *  Get the buzzer volume from NVM.
 *
 * @param[out] volume
 *   The Volume value. It should be in [0:10] range.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_get_buzzer_volume(uint8_t *volume);

/***************************************************************************//**
 * @brief
 *  Get the notification threshold for CO2 level in ppm from NVM.
 *
 * @param[out] threshold_co2
 *   The notification threshold for CO2 level in ppm.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_get_threshold_co2(uint16_t *threshold_co2);

/***************************************************************************//**
 * @brief
 *  Get the notification threshold for TVOC level in ppb from NVM.
 *
 * @param[out] threshold_tvoc
 *   The notification threshold for CO2 level in ppm.
 *
 * @return
 *   @ref ECODE_NVM3_OK on success or a NVM3 @ref Ecode_t on failure.
 ******************************************************************************/
Ecode_t nvm3_user_get_threshold_tvoc(uint16_t *threshold_tvoc);

/***************************************************************************//**
 * NVM3 ticking function.
 ******************************************************************************/
void nvm3_user_process_action(void);

/** @} */

#endif // NVM3_USER_H
