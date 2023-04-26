/***************************************************************************//**
 * @file fingerprint_nvm.h
 * @brief fingerprint nvm header file.
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
#ifndef FINGERPRINT_NVM_H_
#define FINGERPRINT_NVM_H_

#include "sl_status.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"

#define NVM3_FINGERPRINT_KEY       NVM3_KEY_MIN
#define FINGERPRINT_MAX_SLOT       10

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE        nvm3_defaultHandle

/***************************************************************************//**
 * @brief
 *    Initialize the nvm3 module.
 *
 ******************************************************************************/
void fingerprint_config_nvm3_init(void);

/***************************************************************************//**
 * @brief NVM3 write u8 data
 *
 * @param key  NVM key
 * @param value value to store
 * @return sl_status_t  status of function
 *
 ******************************************************************************/
sl_status_t fingerprint_nvm3_u8_write(nvm3_ObjectKey_t key, uint8_t value);

/***************************************************************************//**
 * @brief NVM3 read u8 data
 *
 * @param key  NVM3 key
 * @param u8_value  value to store data that stored by NVM3
 * @return Ecode_t  status of function
 *
 ******************************************************************************/
Ecode_t fingerprint_nvm3_u8_read(nvm3_ObjectKey_t key, uint8_t *u8_value);

/***************************************************************************//**
 * @brief NVM get config data
 *
 * @param fp_config array to store configuration data
 *
 ******************************************************************************/
void fingerprint_nvm3_get_config(uint8_t *fp_config);

#endif /* FINGERPRINT_NVM_H_ */
