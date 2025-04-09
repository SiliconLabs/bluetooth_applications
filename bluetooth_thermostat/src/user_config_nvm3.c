/***************************************************************************//**
 * @file user_config_nvm3.c
 * @brief NVM3 application code
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
#include <stdio.h>
#include <string.h>
#include "sl_status.h"
#include "app_assert.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"
#include "user_config_nvm3.h"
#include "app_log.h"

/***************************************************************************//**
 * @addtogroup user_config_nvm3
 * @brief  NVM3 User configuration.
 * @details
 * @{
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Defines

// Max and min keys for data objects
#define MODE_KEY                       (NVM3_KEY_MIN)
#define SETPOINT_KEY                   (NVM3_KEY_MIN + 1)
#define HYSTERESIS_KEY                 (NVM3_KEY_MIN + 2)
#define LOWER_THRESHOLD_KEY            (NVM3_KEY_MIN + 3)
#define UPPER_THRESHOLD_KEY            (NVM3_KEY_MIN + 4)
#define THRESHOLD_ALARM_STATUS_KEY     (NVM3_KEY_MIN + 5)

#define MODE_DEFAULT                   (0)

#define THRESHOLD_ALARM_DEFAULT        (1)
#define SETPOINT_VALUE_MIN             (-3500)
#define SETPOINT_VALUE_MAX             (12000)
#define SETPOINT_VALUE_DEFAULT         (2500)

#define HYSTERESIS_VALUE_MIN           (0)
#define HYSTERESIS_VALUE_MAX           (15500)
#define HYSTERESIS_VALUE_DEFAULT       (100)

#define LOWER_THRESHOLD_VALUE_MIN      (-3500)
#define LOWER_THRESHOLD_VALUE_MAX      (12000)
#define LOWER_THRESHOLD_VALUE_DEFAULT  (0)

#define UPPER_THRESHOLD_VALUE_MIN      (-3500)
#define UPPER_THRESHOLD_VALUE_MAX      (12000)
#define UPPER_THRESHOLD_VALUE_DEFAULT  (5000)

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE            nvm3_defaultHandle

// -----------------------------------------------------------------------------
// Private function declarations
static Ecode_t conf_data_u8_read(nvm3_ObjectKey_t key, uint8_t *u8_value);
static Ecode_t conf_data_i16_read(nvm3_ObjectKey_t key, int16_t *i16_value);

static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t min_value,
                              uint8_t max_value,
                              uint8_t default_value);
static void conf_data_i16_init(nvm3_ObjectKey_t key,
                               uint16_t min_value,
                               uint16_t max_value,
                               uint16_t default_value);

// -----------------------------------------------------------------------------
// Public function definitions

/***************************************************************************//**
 * Initialize NVM3 config.
 ******************************************************************************/
void user_config_nvm3_init(void)
{
  Ecode_t err;

  // This will call nvm3_open() with default parameters for
  // memory base address and size, cache size, etc.
  err = nvm3_initDefault();
  EFM_ASSERT(err == ECODE_NVM3_OK);

  // Initialize the mode config.
  conf_data_u8_init(MODE_KEY, 0, 1,
                    MODE_DEFAULT);

  // Initialize the setpoint config.
  conf_data_i16_init(SETPOINT_KEY,
                     SETPOINT_VALUE_MIN,
                     SETPOINT_VALUE_MAX,
                     SETPOINT_VALUE_DEFAULT);

  // Initialize the hysteresis config.
  conf_data_i16_init(HYSTERESIS_KEY,
                     HYSTERESIS_VALUE_MIN,
                     HYSTERESIS_VALUE_MAX,
                     HYSTERESIS_VALUE_DEFAULT);

  // Initialize the lower threshold config.
  conf_data_i16_init(LOWER_THRESHOLD_KEY,
                     LOWER_THRESHOLD_VALUE_MIN,
                     LOWER_THRESHOLD_VALUE_MAX,
                     LOWER_THRESHOLD_VALUE_DEFAULT);

  // Initialize the upper threshold config.
  conf_data_i16_init(UPPER_THRESHOLD_KEY,
                     UPPER_THRESHOLD_VALUE_MIN,
                     UPPER_THRESHOLD_VALUE_MAX,
                     UPPER_THRESHOLD_VALUE_DEFAULT);

  // Initialize the notification enable config.
  conf_data_u8_init(THRESHOLD_ALARM_STATUS_KEY, 0, 2, THRESHOLD_ALARM_DEFAULT);
}

/***************************************************************************//**
 * Set NVM3 Mode
 ******************************************************************************/
sl_status_t user_config_nvm3_set_mode(uint8_t mode)
{
  Ecode_t err;

  if (mode > 1) {
    app_log("Invalid mode config: %d\r\n", mode);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       MODE_KEY,
                       (unsigned char *)&mode,
                       sizeof(mode));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored mode config: %d\r\n", mode);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing min distance config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Mode
 ******************************************************************************/
uint8_t user_config_nvm3_get_mode(void)
{
  Ecode_t err;
  uint8_t mode;

  err = conf_data_u8_read(MODE_KEY, &mode);
  app_assert_s(err == ECODE_NVM3_OK);
  return mode;
}

/***************************************************************************//**
 * Set NVM3 Setpoint.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_setpoint(int16_t setpoint)
{
  Ecode_t err;

  if ((setpoint > SETPOINT_VALUE_MAX)
      || (setpoint < SETPOINT_VALUE_MIN)) {
    app_log("Invalid setpoint config: %d\r\n", setpoint);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       SETPOINT_KEY,
                       (unsigned char *)&setpoint,
                       sizeof(setpoint));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored setpoint config: %d\r\n", setpoint);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing setpoint config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Setpoint.
 ******************************************************************************/
int16_t user_config_nvm3_get_setpoint(void)
{
  int16_t setpoint;
  Ecode_t err;

  err = conf_data_i16_read(SETPOINT_KEY, &setpoint);
  app_assert_s(err == ECODE_NVM3_OK);
  return setpoint;
}

/***************************************************************************//**
 * Set NVM3 hysteresis.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_hysteresis(int16_t hysteresis)
{
  Ecode_t err;

  if ((hysteresis > HYSTERESIS_VALUE_MAX)
      || (hysteresis < HYSTERESIS_VALUE_MIN)) {
    app_log("Invalid hysteresis config: %d\r\n", hysteresis);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       HYSTERESIS_KEY,
                       (unsigned char *)&hysteresis,
                       sizeof(hysteresis));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored hysteresis config: %d\r\n", hysteresis);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing hysteresis config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 hysteresis.
 ******************************************************************************/
int16_t user_config_nvm3_get_hysteresis(void)
{
  int16_t hysteresis;
  Ecode_t err;

  err = conf_data_i16_read(HYSTERESIS_KEY, &hysteresis);
  app_assert_s(err == ECODE_NVM3_OK);
  return hysteresis;
}

/***************************************************************************//**
 * Set NVM3 lower threshold.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_lower_threshold(int16_t lower_threshold)
{
  Ecode_t err;

  if ((lower_threshold > LOWER_THRESHOLD_VALUE_MAX)
      || (lower_threshold < LOWER_THRESHOLD_VALUE_MIN)) {
    app_log("Invalid lower threshold config: %d\r\n", lower_threshold);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       LOWER_THRESHOLD_KEY,
                       (unsigned char *)&lower_threshold,
                       sizeof(lower_threshold));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored lower threshold config: %d\r\n", lower_threshold);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing lower threshold config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get Lower Threshold.
 ******************************************************************************/
int16_t user_config_nvm3_get_lower_threshold(void)
{
  int16_t lower_threshold;
  Ecode_t err;

  err = conf_data_i16_read(LOWER_THRESHOLD_KEY, &lower_threshold);
  app_assert_s(err == ECODE_NVM3_OK);
  return lower_threshold;
}

/***************************************************************************//**
 * Set upper threshold
 ******************************************************************************/
sl_status_t user_config_nvm3_set_upper_threshold(int16_t upper_threshold)
{
  Ecode_t err;

  if ((upper_threshold > UPPER_THRESHOLD_VALUE_MAX)
      || (upper_threshold < UPPER_THRESHOLD_VALUE_MIN)) {
    app_log("Invalid timing budget config: %d\r\n", upper_threshold);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       UPPER_THRESHOLD_KEY,
                       (unsigned char *)&upper_threshold,
                       sizeof(upper_threshold));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored upper threshold config: %d\r\n", upper_threshold);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing upper threshold config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 upper threshold.
 ******************************************************************************/
int16_t user_config_nvm3_get_uppper_threshold(void)
{
  int16_t upper_threshold;
  Ecode_t err;

  err = conf_data_i16_read(UPPER_THRESHOLD_KEY, &upper_threshold);
  app_assert_s(err == ECODE_NVM3_OK);
  return upper_threshold;
}

/***************************************************************************//**
 * Set NVM3 threshold alarm status
 ******************************************************************************/
sl_status_t user_config_nvm3_set_alarm_status(bool enable)
{
  uint8_t data = enable ? 1:0;
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       THRESHOLD_ALARM_STATUS_KEY,
                       (unsigned char *)&data,
                       sizeof(data));
  if (ECODE_NVM3_OK == err) {
    app_log("Stored alarm status config: %d\r\n", data);
    return SL_STATUS_OK;
  } else {
    app_log("Error storing alarm status config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 threshold alarm status
 ******************************************************************************/
bool user_config_nvm3_get_alarm_status(void)
{
  Ecode_t err;
  uint8_t threshold_alarm_status;

  err = conf_data_u8_read(THRESHOLD_ALARM_STATUS_KEY, &threshold_alarm_status);
  app_assert_s(err == ECODE_NVM3_OK);
  return threshold_alarm_status == 0 ? false:true;
}

// -----------------------------------------------------------------------------
// Private function

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

static Ecode_t conf_data_i16_read(nvm3_ObjectKey_t key, int16_t *i16_value)
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
      && (len == sizeof(int16_t))) {
    return nvm3_readData(NVM3_DEFAULT_HANDLE,
                         key,
                         i16_value,
                         sizeof(int16_t));
  }
  return ECODE_NVM3_ERR_OBJECT_IS_NOT_DATA;
}

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

static void conf_data_i16_init(nvm3_ObjectKey_t key,
                               uint16_t min_value,
                               uint16_t max_value,
                               uint16_t default_value)
{
  Ecode_t err;
  int16_t read_value;

  // check if the designated keys contain data, and initialize if needed.
  err = conf_data_i16_read(key, &read_value);
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

/** @} (end group user_config_nvm3) */
