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

// Max and min and keys for data objects
#define PEOPLE_ENTERED_SO_FAR_KEY            (NVM3_KEY_MIN)
#define LOWER_THRESHOLD_KEY                  (NVM3_KEY_MIN + 1)
#define UPPER_THRESHOLD_KEY                  (NVM3_KEY_MIN + 2)
#define HYSTERESIS_KEY                       (NVM3_KEY_MIN + 3)
#define ROOM_CAPACITY_KEY                    (NVM3_KEY_MIN + 4)
#define NOTIFICATION_STATUS_KEY              (NVM3_KEY_MIN + 5)
#define IR_THRESHOLD_KEY                     (NVM3_KEY_MIN + 6)

#define PEOPLE_ENTERED_SO_FAR_VALUE_DEFAULT  (0)

#define LOWER_THRESHOLD_MIN                  (-32760)
#define LOWER_THRESHOLD_MAX                  (32760)
#define LOWER_THRESHOLD_DEFAULT              (-200)

#define UPPER_THRESHOLD_MIN                  (-32760)
#define UPPER_THRESHOLD_MAX                  (32760)
#define UPPER_THRESHOLD_DEFAULT              (100)

#define HYSTERESIS_MIN                       (1)
#define HYSTERESIS_MAX                       (65534)
#define HYSTERESIS_DEFAULT                   (50)

#define NOTIFICATION_STATUS_DEFAULT          (1)

#define ROOM_CAPACITY_VALUE_MIN              (1)
#define ROOM_CAPACITY_VALUE_MAX              (65534)
#define ROOM_CAPACITY_VALUE_DEFAULT          (100)

#define IR_THRESHOLD_MIN                     (-32760)
#define IR_THRESHOLD_MAX                     (32760)
#define IR_THRESHOLD_DEFAULT                 (800)
// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE                  nvm3_defaultHandle

// Local function declaration
static Ecode_t conf_data_u8_read(nvm3_ObjectKey_t key, uint8_t *u8_value);
static Ecode_t conf_data_u16_read(nvm3_ObjectKey_t key, uint16_t *u16_value);
static Ecode_t conf_data_i16_read(nvm3_ObjectKey_t key, int16_t *i16_value);
static Ecode_t conf_data_counter_read(nvm3_ObjectKey_t key,
                                      uint32_t *u32_value);
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t min_value,
                              uint8_t max_value,
                              uint8_t default_value);
static void conf_data_u16_init(nvm3_ObjectKey_t key,
                               uint16_t min_value,
                               uint16_t max_value,
                               uint16_t default_value);
static void conf_data_i16_init(nvm3_ObjectKey_t key,
                               int16_t min_value,
                               int16_t max_value,
                               int16_t default_value);
static void conf_counter_init(nvm3_ObjectKey_t key,
                              uint32_t default_value);

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

  // Initialize the people count config.
  conf_counter_init(PEOPLE_ENTERED_SO_FAR_KEY,
                    PEOPLE_ENTERED_SO_FAR_VALUE_DEFAULT);

  // Initialize the notification enable config.
  conf_data_u8_init(NOTIFICATION_STATUS_KEY, 0, 1, 1);

  // Initialize the hysteresis capacity config.
  conf_data_u16_init(HYSTERESIS_KEY,
                     HYSTERESIS_MIN,
                     HYSTERESIS_MAX,
                     HYSTERESIS_DEFAULT);

  // Initialize the room capacity config.
  conf_data_u16_init(ROOM_CAPACITY_KEY,
                     ROOM_CAPACITY_VALUE_MIN,
                     ROOM_CAPACITY_VALUE_MAX,
                     ROOM_CAPACITY_VALUE_DEFAULT);

  // Initialize the lower threshold config.
  conf_data_i16_init(LOWER_THRESHOLD_KEY,
                     LOWER_THRESHOLD_MIN,
                     LOWER_THRESHOLD_MAX,
                     LOWER_THRESHOLD_DEFAULT);

  // Initialize the ir threshold config.
  conf_data_i16_init(IR_THRESHOLD_KEY,
                     IR_THRESHOLD_MIN,
                     IR_THRESHOLD_MAX,
                     IR_THRESHOLD_DEFAULT);

  // Initialize the upper threshold config.
  conf_data_i16_init(UPPER_THRESHOLD_KEY,
                     UPPER_THRESHOLD_MIN,
                     UPPER_THRESHOLD_MAX,
                     UPPER_THRESHOLD_DEFAULT);

  // Initialize the hysteresis config.
  conf_data_u16_init(HYSTERESIS_KEY,
                     HYSTERESIS_MIN,
                     HYSTERESIS_MAX,
                     HYSTERESIS_DEFAULT);
}

/***************************************************************************//**
 * Set NVM3 Room Capacity.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_room_capacity(uint16_t room_capacity)
{
  Ecode_t err;

  if ((room_capacity > ROOM_CAPACITY_VALUE_MAX)
      || (room_capacity < ROOM_CAPACITY_VALUE_MIN)) {
    app_log("\r\nInvalid room capacity config: %d", room_capacity);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       ROOM_CAPACITY_KEY,
                       (unsigned char *)&room_capacity,
                       sizeof(room_capacity));
  if (ECODE_NVM3_OK == err) {
    app_log("\r\nStored room capacity config: %d", room_capacity);
    return SL_STATUS_OK;
  } else {
    app_log("\r\nError storing room capacity config");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Room Capacity.
 ******************************************************************************/
uint16_t user_config_nvm3_get_room_capacity(void)
{
  uint16_t room_capacity;
  Ecode_t err;

  err = conf_data_u16_read(ROOM_CAPACITY_KEY, &room_capacity);
  app_assert_s(err == ECODE_NVM3_OK);
  return room_capacity;
}

/***************************************************************************//**
 * Set NVM3 People Entered So Far Counter.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_people_entered_so_far(uint32_t people_count)
{
  Ecode_t err;

  err = nvm3_writeCounter(NVM3_DEFAULT_HANDLE,
                          PEOPLE_ENTERED_SO_FAR_KEY,
                          people_count);
  if (ECODE_NVM3_OK == err) {
    app_log("\r\nStored people counter value: %lu", people_count);
    return SL_STATUS_OK;
  } else {
    app_log("\r\nError storing people counter value");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 People Entered So Far Counter.
 ******************************************************************************/
uint32_t user_config_nvm3_get_people_entered_so_far(void)
{
  Ecode_t err;
  uint32_t people_count;

  err = conf_data_counter_read(PEOPLE_ENTERED_SO_FAR_KEY, &people_count);
  app_assert_s(err == ECODE_NVM3_OK);

  return people_count;
}

/***************************************************************************//**
 * Set NVM3 Lower threshold.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_lower_threshold(int16_t threshold)
{
  Ecode_t err;

  if ((threshold > LOWER_THRESHOLD_MAX)
      || (threshold < LOWER_THRESHOLD_MIN)) {
    app_log("\r\nInvalid lower threshold config: %d", threshold);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       LOWER_THRESHOLD_KEY,
                       (unsigned char *)&threshold,
                       sizeof(threshold));
  if (ECODE_NVM3_OK == err) {
    app_log("\r\nStored lower threshold config: %d", threshold);
    return SL_STATUS_OK;
  } else {
    app_log("\r\nError storing lower threshold config");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Lower threshold.
 ******************************************************************************/
int16_t user_config_nvm3_get_lower_threshold(void)
{
  int16_t threshold;
  Ecode_t err;

  err = conf_data_i16_read(LOWER_THRESHOLD_KEY, &threshold);
  app_assert_s(err == ECODE_NVM3_OK);

  return threshold;
}

/***************************************************************************//**
 * Set NVM3 Upper threshold.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_upper_threshold(int16_t threshold)
{
  Ecode_t err;

  if ((threshold > UPPER_THRESHOLD_MAX)
      || (threshold < UPPER_THRESHOLD_MIN)) {
    app_log("\r\nInvalid upper threshold config: %d", threshold);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       UPPER_THRESHOLD_KEY,
                       (unsigned char *)&threshold,
                       sizeof(threshold));
  if (ECODE_NVM3_OK == err) {
    app_log("\r\nStored upper threshold config: %d", threshold);
    return SL_STATUS_OK;
  } else {
    app_log("\r\nError storing upper threshold config");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Upper threshold.
 ******************************************************************************/
int16_t user_config_nvm3_get_upper_threshold(void)
{
  int16_t threshold;
  Ecode_t err;

  err = conf_data_i16_read(UPPER_THRESHOLD_KEY, &threshold);
  app_assert_s(err == ECODE_NVM3_OK);

  return threshold;
}

/***************************************************************************//**
 * Set NVM3 Hysteresis.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_hysteresis(uint16_t hysteresis)
{
  Ecode_t err;

  if ((hysteresis > HYSTERESIS_MAX)
      || (hysteresis < HYSTERESIS_MIN)) {
    app_log("\r\nInvalid hysteresis config: %d", hysteresis);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       HYSTERESIS_KEY,
                       (unsigned char *)&hysteresis,
                       sizeof(hysteresis));
  if (ECODE_NVM3_OK == err) {
    app_log("\r\nStored hysteresis config: %d", hysteresis);
    return SL_STATUS_OK;
  } else {
    app_log("\r\nError storing hysteresis config");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Lower threshold.
 ******************************************************************************/
uint16_t user_config_nvm3_get_hysteresis(void)
{
  uint16_t hysteresis;
  Ecode_t err;

  err = conf_data_u16_read(HYSTERESIS_KEY, &hysteresis);
  app_assert_s(err == ECODE_NVM3_OK);

  return hysteresis;
}

/***************************************************************************//**
 * Set NVM3 ir threshold.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_ir_threshold(int16_t threshold)
{
  Ecode_t err;

  if ((threshold > LOWER_THRESHOLD_MAX)
      || (threshold < LOWER_THRESHOLD_MIN)) {
    app_log("\r\nInvalid IR threshold config: %d", threshold);
    return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       IR_THRESHOLD_KEY,
                       (unsigned char *)&threshold,
                       sizeof(threshold));
  if (ECODE_NVM3_OK == err) {
    app_log("\r\nStored ir threshold config: %d", threshold);
    return SL_STATUS_OK;
  } else {
    app_log("\r\nError storing ir threshold config");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 ir threshold.
 ******************************************************************************/
int16_t user_config_nvm3_get_ir_threshold(void)
{
  int16_t threshold;
  Ecode_t err;

  err = conf_data_i16_read(IR_THRESHOLD_KEY, &threshold);
  app_assert_s(err == ECODE_NVM3_OK);

  return threshold;
}

/***************************************************************************//**
 * Set NVM3 Notification Status.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_notification_status(bool enable)
{
  uint8_t data = enable ? 1:0;
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       NOTIFICATION_STATUS_KEY,
                       (unsigned char *)&data,
                       sizeof(data));
  if (ECODE_NVM3_OK == err) {
    app_log("\r\nStored notification active config: %d", data);
    return SL_STATUS_OK;
  } else {
    app_log("\r\nError storing active config");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Notification Status.
 ******************************************************************************/
bool user_config_nvm3_get_notification_status(void)
{
  Ecode_t err;
  uint8_t notification_status;

  err = conf_data_u8_read(NOTIFICATION_STATUS_KEY, &notification_status);
  app_assert_s(err == ECODE_NVM3_OK);
  return notification_status == 0 ? false:true;
}

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

static Ecode_t conf_data_u16_read(nvm3_ObjectKey_t key, uint16_t *u16_value)
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
      && (len == sizeof(uint16_t))) {
    return nvm3_readData(NVM3_DEFAULT_HANDLE,
                         key,
                         u16_value,
                         sizeof(uint16_t));
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

static Ecode_t conf_data_counter_read(nvm3_ObjectKey_t key, uint32_t *u32_value)
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
  if (type == NVM3_OBJECTTYPE_COUNTER) {
    return nvm3_readCounter(NVM3_DEFAULT_HANDLE,
                            key,
                            u32_value);
  }
  return ECODE_NVM3_ERR_OBJECT_IS_NOT_A_COUNTER;
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

static void conf_data_u16_init(nvm3_ObjectKey_t key,
                               uint16_t min_value,
                               uint16_t max_value,
                               uint16_t default_value)
{
  Ecode_t err;
  uint16_t read_value;

  // check if the designated keys contain data, and initialize if needed.
  err = conf_data_u16_read(key, &read_value);
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
                               int16_t min_value,
                               int16_t max_value,
                               int16_t default_value)
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

static void conf_counter_init(nvm3_ObjectKey_t key,
                              uint32_t default_value)
{
  Ecode_t err;
  uint32_t read_value;

  // check if the designated keys contain data, and initialize if needed.
  err = conf_data_counter_read(key, &read_value);
  if (err == ECODE_NVM3_OK) {
    return;
  } else {
    nvm3_deleteObject(NVM3_DEFAULT_HANDLE, key);
  }
  // Write default value
  err = nvm3_writeCounter(NVM3_DEFAULT_HANDLE,
                          key,
                          default_value);
}
