/***************************************************************************//**
 * @file container_nvm.h
 * @brief container nvm header file.
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
#ifndef CONTAINER_NVM_H_
#define CONTAINER_NVM_H_

#include "sl_status.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"

//max container supported
#define CONTAINER_MAX_SLOT         4
#define CONTAINER_NVM_KEY_0        (NVM3_KEY_MIN)
#define CONTAINER_NVM_KEY_2        (NVM3_KEY_MIN + 1)
#define CONTAINER_NVM_KEY_3        (NVM3_KEY_MIN + 2)
#define CONTAINER_NVM_KEY_4        (NVM3_KEY_MIN + 3)


// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE        nvm3_defaultHandle

// container config structure
typedef struct container_nvm_config_t
{
  uint16_t id_configuration;
  uint16_t lowest_level;
  uint16_t highest_level;
}container_nvm_config_t;

/***************************************************************************//**
 * @brief
 *    Initialize the nvm3 module.
 *
 ******************************************************************************/
void container_config_nvm3_init(void);

/***************************************************************************//**
 * @brief NVM3 write u8 data
 *
 * @param key  NVM key
 * @param cfg  value to store
 * @return sl_status_t  status of function
 *
 ******************************************************************************/
sl_status_t container_nvm3_write(nvm3_ObjectKey_t key,
                                 container_nvm_config_t cfg);

/***************************************************************************//**
 * @brief NVM3 read u8 data
 *
 * @param key  NVM3 key
 * @param cfg  hold the data after read
 * @return sl_status_t  status of function
 *
 ******************************************************************************/
sl_status_t container_nvm3_read(nvm3_ObjectKey_t key, container_nvm_config_t *cfg);

/***************************************************************************//**
 * @brief NVM get config data
 *
 * @param container_config array to store configuration data
 *
 ******************************************************************************/
void container_nvm3_get_config(container_nvm_config_t *container_config);

#endif /* CONTAINER_NVM_H_ */
