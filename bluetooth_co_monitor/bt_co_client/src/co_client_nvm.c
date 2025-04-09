/***************************************************************************//**
 * @file co_client_nvm.c
 * @brief co client nvm source file.
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
#include "co_client_nvm.h"

// Local function declaration.
static Ecode_t conf_data_u8_read(nvm3_ObjectKey_t key, uint8_t *u8_value);
static Ecode_t conf_data_u32_read(nvm3_ObjectKey_t key, uint32_t *value);
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t min_value,
                              uint8_t max_value,
                              uint8_t default_value);
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
  conf_data_u8_init(NVM_NOTIFICATION_STATUS_KEY, 0, 1, 1);

  // Initialize the buzzer volume config
  conf_data_u8_init(NVM_BUZZER_VOLUME_KEY, 0, 10, 5);

  // Initialize the threshold co config
  conf_data_u32_init(NVM_THRESHOLD_CO_KEY, 0, 500, 10);
}

/***************************************************************************//**
 * Set NVM3 Notification Status.
 ******************************************************************************/
sl_status_t client_nvm3_set_notification_status(bool enable)
{
  uint8_t data = enable ? 1:0;
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       NVM_NOTIFICATION_STATUS_KEY,
                       (unsigned char *)&data,
                       sizeof(data));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored notification config: %d\r\n", data);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing notification config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Set NVM3 buzzer volume
 ******************************************************************************/
sl_status_t client_nvm3_set_buzzer_volume(uint8_t buzzer_volume)
{
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       NVM_BUZZER_VOLUME_KEY,
                       (unsigned char *)&buzzer_volume,
                       sizeof(buzzer_volume));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored buzzer volume config: %d\r\n", buzzer_volume);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing buzzer volume config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Set NVM3 Alarm Threshold.
 ******************************************************************************/
sl_status_t client_nvm3_set_alarm_threshold(uint32_t threshold)
{
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       NVM_THRESHOLD_CO_KEY,
                       &threshold,
                       sizeof(threshold));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored alarm threshold config: %ld\r\n", threshold);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing alarm threshold config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Notification Status.
 ******************************************************************************/
bool client_nvm3_get_notification_status(void)
{
  Ecode_t err;
  uint8_t notification_status;

  err = conf_data_u8_read(NVM_NOTIFICATION_STATUS_KEY, &notification_status);
  app_assert_s(err == ECODE_NVM3_OK);
  return notification_status == 0 ? false:true;
}

/***************************************************************************//**
 * Get NVM3 Click Noise Status.
 ******************************************************************************/
uint8_t client_nvm3_get_buzzer_volume(void)
{
  Ecode_t err;
  uint8_t buzzer_volume;

  err = conf_data_u8_read(NVM_BUZZER_VOLUME_KEY, &buzzer_volume);
  app_assert_s(err == ECODE_NVM3_OK);
  return buzzer_volume;
}

/***************************************************************************//**
 * Get NVM3 Alarm Threshold.
 ******************************************************************************/
uint32_t client_nvm3_get_alarm_threshold(void)
{
  Ecode_t err;
  uint32_t threshold;

  err = conf_data_u32_read(NVM_THRESHOLD_CO_KEY, &threshold);
  app_assert_s(err == ECODE_NVM3_OK);
  return threshold;
}

/***************************************************************************//**
 * Get Client Configuration.
 ******************************************************************************/
void client_nvm3_get_config(client_nvm_config_t *cfg)
{
  cfg->threshold_co_ppm = client_nvm3_get_alarm_threshold();
  cfg->buzzer_volume = client_nvm3_get_buzzer_volume();
  cfg->is_notification_active = client_nvm3_get_notification_status();
}

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
  err = conf_data_u8_read(key, &read_value);
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
 * NVM3 read u8 data.
 ******************************************************************************/
static Ecode_t conf_data_u8_read(nvm3_ObjectKey_t key, uint8_t *u8_value)
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
