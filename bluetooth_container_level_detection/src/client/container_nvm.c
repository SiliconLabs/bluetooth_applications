/***************************************************************************//**
 * @file container_nvm.c
 * @brief container nvm source file.
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
#include "container_nvm.h"

// Local function declaration.
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint16_t min_value,
                              uint16_t max_value,
                              container_nvm_config_t default_value);

/***************************************************************************//**
 * NVM3 u8 data initialize.
 ******************************************************************************/
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint16_t min_value,
                              uint16_t max_value,
                              container_nvm_config_t default_config)
{
  Ecode_t err;
  container_nvm_config_t read_value;

  // check if the designated keys contain data, and initialize if needed.
  err = container_nvm3_read(key, &read_value);
  if ((err == ECODE_NVM3_OK)
      && (read_value.lowest_level >= min_value)
      && (read_value.highest_level <= max_value)) {
    return;
  } else {
    nvm3_deleteObject(NVM3_DEFAULT_HANDLE, key);
  }
  // Write default value
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       key,
                       (container_nvm_config_t *)&default_config,
                       sizeof(default_config));
}

/***************************************************************************//**
 * NVM3 read u8 data.
 ******************************************************************************/
sl_status_t container_nvm3_read(nvm3_ObjectKey_t key,
                                container_nvm_config_t *cfg)
{
  uint32_t type;
  size_t len;
  Ecode_t err;

  // check if the designated keys contain data, and initialize if needed.
  err = nvm3_getObjectInfo(NVM3_DEFAULT_HANDLE,
                           key,
                           &type,
                           &len);
  if (err != ECODE_NVM3_OK) {
    return SL_STATUS_FAIL;
  }
  if ((type == NVM3_OBJECTTYPE_DATA)
      && (len == sizeof(container_nvm_config_t))) {
    return nvm3_readData(NVM3_DEFAULT_HANDLE,
                         key,
                         cfg,
                         sizeof(container_nvm_config_t))
           == ECODE_NVM3_OK ? SL_STATUS_OK : SL_STATUS_FAIL;
  } else {
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Set write u8 data
 ******************************************************************************/
sl_status_t container_nvm3_write(nvm3_ObjectKey_t key,
                                 container_nvm_config_t cfg)
{
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       key,
                       (container_nvm_config_t *)&cfg,
                       sizeof(cfg));
  if (ECODE_NVM3_OK == err) {
    return SL_STATUS_OK;
  } else {
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * NVM config init
 ******************************************************************************/
void container_config_nvm3_init(void)
{
  Ecode_t err;
  container_nvm_config_t default_config;

  default_config.id_configuration = 0x00;
  default_config.highest_level = 2161;
  default_config.lowest_level = 40;

  err = nvm3_initDefault();
  EFM_ASSERT(err == ECODE_NVM3_OK);

  for (int i = 0; i < CONTAINER_MAX_SLOT; i++) {
    conf_data_u8_init(NVM3_KEY_MIN + i, 40, 4000, default_config);
  }
}

/***************************************************************************//**
 * NVM get config
 ******************************************************************************/
void container_nvm3_get_config(container_nvm_config_t *container_config)
{
  for (int i = 0; i < CONTAINER_MAX_SLOT; i++) {
    container_nvm3_read(NVM3_KEY_MIN + i, &container_config[i]);
  }
}
