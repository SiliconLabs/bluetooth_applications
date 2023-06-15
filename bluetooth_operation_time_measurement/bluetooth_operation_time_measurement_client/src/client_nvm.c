/***************************************************************************//**
 * @file client_nvm.c
 * @brief accel5 client nvm source file.
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
#include <client_nvm.h>

// Local function declaration.
static Ecode_t conf_data_u32_read(nvm3_ObjectKey_t key, uint32_t *value);
static void conf_data_u32_init(nvm3_ObjectKey_t key,
                               uint32_t min_value,
                               uint32_t max_value,
                               uint32_t default_value);

/***************************************************************************//**
 * Initialize NVM3 config.
 ******************************************************************************/
void client_config_nvm3_init(void)
{
  Ecode_t err;

  // This will call nvm3_open() with default parameters for
  // memory base address and size, cache size, etc.
  err = nvm3_initDefault();
  EFM_ASSERT(err == ECODE_NVM3_OK);

  // Initialize the notification enable config.
  conf_data_u32_init(NVM_OPERATION_TIME_THRESHOLD_KEY, 0, 0xFFFF, 60 * 60);
}

/***************************************************************************//**
 * Set NVM3 operation time threshold.
 ******************************************************************************/
sl_status_t client_nvm3_set_operation_time_threshold(
  uint32_t operation_time_threshold)
{
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       NVM_OPERATION_TIME_THRESHOLD_KEY,
                       &operation_time_threshold,
                       sizeof(operation_time_threshold));
  if (ECODE_NVM3_OK == err) {
    app_log("stored operation time threshold: %lu\r\n",
            operation_time_threshold);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing operation time threshold\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 operation time threshold.
 ******************************************************************************/
sl_status_t client_nvm3_get_operation_time_threshold(
  uint32_t *operation_time_threshold)
{
  Ecode_t err;

  err = conf_data_u32_read(NVM_OPERATION_TIME_THRESHOLD_KEY,
                           operation_time_threshold);
  if (ECODE_NVM3_OK == err) {
    app_log("read operation time threshold: %lu\r\n",
            *operation_time_threshold);
    return SL_STATUS_OK;
  } else {
    app_log("Error reading operation time threshold\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * NVM3 u32 data initialize.
 ******************************************************************************/
static void conf_data_u32_init(nvm3_ObjectKey_t key,
                               uint32_t min_value,
                               uint32_t max_value,
                               uint32_t default_value)
{
  Ecode_t err;
  uint32_t read_value;

  // check if the designated keys contain data, and initialize if needed.
  err = conf_data_u32_read(key, &read_value);
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
                       &default_value,
                       sizeof(default_value));
}

/***************************************************************************//**
 * NVM3 read u32 data.
 ******************************************************************************/
static Ecode_t conf_data_u32_read(nvm3_ObjectKey_t key, uint32_t *value)
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
      && (len == sizeof(uint32_t))) {
    return nvm3_readData(NVM3_DEFAULT_HANDLE,
                         key,
                         value,
                         sizeof(uint32_t));
  }
  return ECODE_NVM3_ERR_OBJECT_IS_NOT_DATA;
}
