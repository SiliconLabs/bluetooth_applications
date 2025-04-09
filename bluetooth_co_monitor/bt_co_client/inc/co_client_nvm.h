/***************************************************************************//**
 * @file co_client_nvm.h
 * @brief co client nvm header file.
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

#ifndef CO_CLIENT_NVM_H_
#define CO_CLIENT_NVM_H_

#include "sl_status.h"
#include "app_assert.h"
#include "app_log.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"

#define NVM_THRESHOLD_CO_KEY                 (NVM3_KEY_MIN)
#define NVM_NOTIFICATION_STATUS_KEY          (NVM3_KEY_MIN + 2)
#define NVM_BUZZER_VOLUME_KEY                (NVM3_KEY_MIN + 3)

#define NOTIFICATION_STATUS_DEFAULT          (1)
#define CLICK_NOISE_STATUS_DEFAULT           (1)

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE                  nvm3_defaultHandle

typedef struct {
  uint32_t threshold_co_ppm;
  bool is_notification_active;
  uint8_t buzzer_volume;
} client_nvm_config_t;

/***************************************************************************//**
 * @brief
 *    Initialize the nvm3 module.
 *
 ******************************************************************************/
void client_config_nvm3_init(void);

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
sl_status_t client_nvm3_set_notification_status(bool enable);

/***************************************************************************//**
 * @brief
 *    Set buzzer volume to the nvm3 entry.
 *
 * @param[in] enable
 *    Enable or disable.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t client_nvm3_set_buzzer_volume(uint8_t buzzer_volume);

/***************************************************************************//**
 * @brief
 *    Set alarm threshold to the nvm3 entry.
 *
 * @param[in] threshold
 *    Threshold value.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t client_nvm3_set_alarm_threshold(uint32_t threshold);

/***************************************************************************//**
 * @brief
 *    Get notification status from the nvm3 entry.
 *
 * @return
 *    Notification status value.
 ******************************************************************************/
bool client_nvm3_get_notification_status(void);

/***************************************************************************//**
 * @brief
 *    Get buzzer volume from the nvm3 entry.
 *
 * @return
 *    Click noise status value.
 ******************************************************************************/
uint8_t client_nvm3_get_buzzer_volume(void);

/***************************************************************************//**
 * @brief
 *    Get alarm threshold from the nvm3 entry.
 *
 * @return
 *    Threshold value.
 ******************************************************************************/
uint32_t client_nvm3_get_alarm_threshold(void);

/***************************************************************************//**
 * @brief
 *    Get client configuration from the nvm3 entry.
 *
 * @param[in] cfg
 *    Struct that holds the configuration of the client.
 ******************************************************************************/
void client_nvm3_get_config(client_nvm_config_t *cfg);

#endif /* CO_CLIENT_NVM_H_ */
