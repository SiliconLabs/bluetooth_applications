/***************************************************************************//**
 * @file fingerprint_nvm.c
 * @brief fingerprint nvm source file.
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
#include "fingerprint_nvm.h"

// Local function declaration.
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t min_value,
                              uint8_t max_value,
                              uint8_t default_value);

/***************************************************************************//**
 * NVM3 u8 data initialize.
 ******************************************************************************/
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t min_value,
                              uint8_t max_value,
                              uint8_t default_value)
{
  Ecode_t err;
  uint8_t read_value;

  // check if the designated keys contain data, and initialize if needed.
  err = fingerprint_nvm3_u8_read(key, &read_value);
  if ((err == ECODE_NVM3_OK)
      && (read_value >= min_value)
      && (read_value <= max_value)) {
    return;
  } else {
    nvm3_deleteObject(NVM3_DEFAULT_HANDLE, key);
  }
  // Write default value
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       key,
                       (unsigned char *)&default_value,
                       sizeof(default_value));
}

/***************************************************************************//**
 * NVM3 read u8 data.
 ******************************************************************************/
Ecode_t fingerprint_nvm3_u8_read(nvm3_ObjectKey_t key, uint8_t *u8_value)
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
    return err;
  }
  if ((type == NVM3_OBJECTTYPE_DATA)
      && (len == sizeof(uint8_t))) {
    return nvm3_readData(NVM3_DEFAULT_HANDLE,
                         key,
                         u8_value,
                         sizeof(uint8_t));
  }
  return ECODE_NVM3_ERR_OBJECT_IS_NOT_DATA;
}

/***************************************************************************//**
 * Set write u8 data
 ******************************************************************************/
sl_status_t fingerprint_nvm3_u8_write(nvm3_ObjectKey_t key, uint8_t value)
{
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       key,
                       (unsigned char *)&value,
                       sizeof(value));
  if (ECODE_NVM3_OK == err) {
    return SL_STATUS_OK;
  } else {
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * NVM config init
 ******************************************************************************/
void fingerprint_config_nvm3_init(void)
{
  Ecode_t err;

  err = nvm3_initDefault();
  EFM_ASSERT(err == ECODE_NVM3_OK);

  for (int i = 0; i < FINGERPRINT_MAX_SLOT; i++) {
    conf_data_u8_init(NVM3_FINGERPRINT_KEY + i, 0, 9, 0xFF);
  }
}

/***************************************************************************//**
 * NVM get config
 ******************************************************************************/
void fingerprint_nvm3_get_config(uint8_t *fp_config)
{
  for (int i = 0; i < FINGERPRINT_MAX_SLOT; i++) {
    fingerprint_nvm3_u8_read(NVM3_FINGERPRINT_KEY + i, &fp_config[i]);
  }
}
