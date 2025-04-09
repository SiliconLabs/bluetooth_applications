/***************************************************************************//**
 * @file client_nvm.h
 * @brief accel5 client nvm header file.
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

#ifndef CLIENT_NVM_H_
#define CLIENT_NVM_H_

#include "sl_status.h"
#include "app_assert.h"
#include "app_log.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"

#define NVM_OPERATION_TIME_THRESHOLD_KEY                 (NVM3_KEY_MIN)

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE                              nvm3_defaultHandle

/***************************************************************************//**
 * @brief
 *    Initialize the nvm3 module.
 *
 ******************************************************************************/
void client_config_nvm3_init(void);

/***************************************************************************//**
 * @brief
 *    Set operation time threshold to the nvm3 entry.
 *
 * @param[in] operation_time_threshold
 *    operation time threshold value.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t client_nvm3_set_operation_time_threshold(
  uint32_t operation_time_threshold);

/***************************************************************************//**
 * @brief
 *    Get operation time threshold of nvm3 entry.
 *
 * @param[out] operation_time_threshold
 *    operation time threshold value.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t client_nvm3_get_operation_time_threshold(
  uint32_t *operation_time_threshold);

#endif
