/***************************************************************************//**
 * @file switch_nvm.c
 * @brief switch nvm source file.
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
#include "switch_nvm.h"

/***************************************************************************//**
 * Local function declaration.
 ******************************************************************************/

static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t default_value);

static sl_status_t switch_nvm3_read(nvm3_ObjectKey_t key,
                                    uint8_t *cfg);

static sl_status_t switch_nvm3_write(nvm3_ObjectKey_t key,
                                     uint8_t cfg);

/***************************************************************************//**
 * Public function definition.
 ******************************************************************************/
void switch_nvm3_init(void)
{
  Ecode_t err;
  err = nvm3_initDefault();
  EFM_ASSERT(err == ECODE_NVM3_OK);

  conf_data_u8_init(SWITCH_STATE_KEY, 0X00);
}

sl_status_t nvm3_get_switch_state(uint8_t *state)
{
  sl_status_t ret;

  ret = switch_nvm3_read(SWITCH_STATE_KEY, state);
  return ret;
}

sl_status_t nvm3_save_switch_state(uint8_t state)
{
  sl_status_t ret;

  ret = switch_nvm3_write(SWITCH_STATE_KEY, state);
  return ret;
}

/***************************************************************************//**
 * Local function definition.
 ******************************************************************************/
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t default_value)
{
  Ecode_t err;
  uint8_t read_value;

  // check if the designated keys contain data, and initialize if needed.
  err = switch_nvm3_read(key, &read_value);
  if ((err == ECODE_NVM3_OK)
      && (read_value <= 1)) {
    return;
  } else {
    nvm3_deleteObject(nvm3_defaultHandle, key);
  }
  // Write default value
  err = nvm3_writeData(nvm3_defaultHandle,
                       key,
                       (uint8_t *)&default_value,
                       sizeof(default_value));
}

/***************************************************************************//**
 * NVM3 read u8 data.
 ******************************************************************************/
static sl_status_t switch_nvm3_read(nvm3_ObjectKey_t key,
                                    uint8_t *cfg)
{
  uint32_t type;
  size_t len;
  Ecode_t err;

  // check if the designated keys contain data, and initialize if needed.
  err = nvm3_getObjectInfo(nvm3_defaultHandle,
                           key,
                           &type,
                           &len);
  if (err != ECODE_NVM3_OK) {
    return SL_STATUS_FAIL;
  }
  if ((type == NVM3_OBJECTTYPE_DATA)
      && (len == sizeof(uint8_t))) {
    return nvm3_readData(nvm3_defaultHandle,
                         key,
                         cfg,
                         sizeof(uint8_t))
           == ECODE_NVM3_OK ? SL_STATUS_OK : SL_STATUS_FAIL;
  } else {
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * NVM3 write u8 data
 ******************************************************************************/
static sl_status_t switch_nvm3_write(nvm3_ObjectKey_t key,
                                     uint8_t cfg)
{
  Ecode_t err;

  err = nvm3_writeData(nvm3_defaultHandle,
                       key,
                       (uint8_t *)&cfg,
                       sizeof(cfg));
  if (ECODE_NVM3_OK == err) {
    return SL_STATUS_OK;
  } else {
    return SL_STATUS_FAIL;
  }
}
