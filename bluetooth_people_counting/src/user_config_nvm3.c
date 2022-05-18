/***************************************************************************//**
 * @file user_config_nvm3.c
 * @brief NVM3 application code
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
#include <stdio.h>
#include <string.h>
#include "sl_status.h"
#include "app_assert.h"

#include "nvm3_default.h"
#include "nvm3_default_config.h"
#include "user_config_nvm3.h"

// -----------------------------------------------------------------------------
// Logging
#define TAG "nvm3"
// use applog for the log printing
#if defined(SL_CATALOG_APP_LOG_PRESENT) && APP_LOG_ENABLE
#include "app_log.h"
#define log_info(fmt, ...)  app_log_info("[" TAG "] " fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) app_log_error("[" TAG "] " fmt, ##__VA_ARGS__)
// use stdio printf for the log printing
#elif defined(SL_CATALOG_RETARGET_STDIO_PRESENT)
#define log_info(fmt, ...)   printf("[" TAG "] " fmt, ##__VA_ARGS__)
#define log_error(fmt, ...)  printf("[" TAG "] " fmt, ##__VA_ARGS__)
#else  // the logging is disabled
#define log_info(...)
#define log_error(...)
#endif // #if defined(SL_CATALOG_APP_LOG_PRESENT)

/***************************************************************************//**
 * @addtogroup user_config_nvm3
 * @brief  NVM3 User configuration.
 * @details
 * @{
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Defines

// Max and min keys for data objects
#define PEOPLE_ENTERED_SO_FAR_KEY   (NVM3_KEY_MIN)
#define MIN_DISTANCE_KEY            (NVM3_KEY_MIN+1)
#define MAX_DISTANCE_KEY            (NVM3_KEY_MIN+2)
#define DISTANCE_THRESHOLD_KEY      (NVM3_KEY_MIN+3)
#define TIMING_BUDGET_KEY           (NVM3_KEY_MIN+4)
#define NOTIFICATION_STATUS_KEY     (NVM3_KEY_MIN+5)
#define ROOM_CAPACITY_KEY           (NVM3_KEY_MIN+6)

#define PEOPLE_ENTERED_SO_FAR_VALUE_DEFAULT  (0)

#define MIN_DISTANCE_VALUE_MIN      (1)
#define MIN_DISTANCE_VALUE_MAX      (3600)
#define MIN_DISTANCE_VALUE_DEFAULT  (0)

#define MAX_DISTANCE_VALUE_MIN      (1)
#define MAX_DISTANCE_VALUE_MAX      (3600)
#define MAX_DISTANCE_VALUE_DEFAULT  (2700)

#define DISTANCE_THRESHOLD_VALUE_MIN      (1)
#define DISTANCE_THRESHOLD_VALUE_MAX      (3600)
#define DISTANCE_THRESHOLD_VALUE_DEFAULT  (1600)

#define TIMING_BUDGET_VALUE_MIN     (20)
#define TIMING_BUDGET_VALUE_MAX     (1000)
#define TIMING_BUDGET_VALUE_DEFAULT (33)

#define NOTIFICATION_STATUS_DEFAULT  (1)

#define ROOM_CAPACITY_VALUE_MIN     (1)
#define ROOM_CAPACITY_VALUE_MAX     (65534)
#define ROOM_CAPACITY_VALUE_DEFAULT (100)

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE nvm3_defaultHandle

// -----------------------------------------------------------------------------
// Private function declarations
static Ecode_t conf_data_u8_read(nvm3_ObjectKey_t key, uint8_t *u8_value);
static Ecode_t conf_data_u16_read(nvm3_ObjectKey_t key, uint16_t *u16_value);
static Ecode_t conf_data_counter_read(nvm3_ObjectKey_t key, uint32_t *u32_value);
static Ecode_t conf_data_counter_read(nvm3_ObjectKey_t key, uint32_t *u32_value);
static void conf_data_u8_init(nvm3_ObjectKey_t key,
                              uint8_t min_value,
                              uint8_t max_value,
                              uint8_t default_value);
static void conf_data_u16_init(nvm3_ObjectKey_t key,
                               uint16_t min_value,
                               uint16_t max_value,
                               uint16_t default_value);
static void conf_counter_init(nvm3_ObjectKey_t key,
                             uint32_t default_value);


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

  // Initialize the people count config.
  conf_counter_init(PEOPLE_ENTERED_SO_FAR_KEY,
                    PEOPLE_ENTERED_SO_FAR_VALUE_DEFAULT);

  // Initialize the min distance config.
  conf_data_u16_init(MIN_DISTANCE_KEY,
                     MIN_DISTANCE_VALUE_MIN,
                     MIN_DISTANCE_VALUE_MAX,
                     MIN_DISTANCE_VALUE_DEFAULT);

  // Initialize the max distance config.
  conf_data_u16_init(MAX_DISTANCE_KEY,
                     MAX_DISTANCE_VALUE_MIN,
                     MAX_DISTANCE_VALUE_MAX,
                     MAX_DISTANCE_VALUE_DEFAULT);

  // Initialize the distance threshold config.
  conf_data_u16_init(DISTANCE_THRESHOLD_KEY,
                     DISTANCE_THRESHOLD_VALUE_MIN,
                     DISTANCE_THRESHOLD_VALUE_MAX,
                     DISTANCE_THRESHOLD_VALUE_DEFAULT);

  // Initialize the timing budget config.
  conf_data_u16_init(TIMING_BUDGET_KEY,
                     TIMING_BUDGET_VALUE_MIN,
                     TIMING_BUDGET_VALUE_MAX,
                     TIMING_BUDGET_VALUE_DEFAULT);

  // Initialize the notification enable config.
  conf_data_u8_init(NOTIFICATION_STATUS_KEY, 0, 1, 1);

  // Initialize the room capacity config.
  conf_data_u16_init(ROOM_CAPACITY_KEY,
                     ROOM_CAPACITY_VALUE_MIN,
                     ROOM_CAPACITY_VALUE_MAX,
                     ROOM_CAPACITY_VALUE_DEFAULT);
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
    log_info("Stored people counter value: %lu\r\n", people_count);
    return SL_STATUS_OK;
  } else {
    log_info("Error storing people counter value\r\n");
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
 * Set NVM3 Min distance.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_min_distance(uint16_t distance)
{
  Ecode_t err;

  if ((distance > MAX_DISTANCE_VALUE_MAX)
      || (distance < MIN_DISTANCE_VALUE_MIN)){
      log_info("Invalid min distance config: %d\r\n", distance);
      return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       MIN_DISTANCE_KEY,
                       (unsigned char *)&distance,
                       sizeof(distance));
  if (ECODE_NVM3_OK == err) {
    log_info("Stored min distance config: %d\r\n", distance);
    return SL_STATUS_OK;
  } else {
    log_info("Error storing min distance config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Min distance.
 ******************************************************************************/
uint16_t user_config_nvm3_get_min_distance(void)
{
  uint16_t distance;
  Ecode_t err;

  err = conf_data_u16_read(MIN_DISTANCE_KEY, &distance);
  app_assert_s(err == ECODE_NVM3_OK);
  return distance;
}

/***************************************************************************//**
 * Set NVM3 Max distance.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_max_distance(uint16_t distance)
{
  Ecode_t err;

  if ((distance > MAX_DISTANCE_VALUE_MAX)
      || (distance < MAX_DISTANCE_VALUE_MIN)){
      log_info("Invalid max distance config: %d\r\n", distance);
      return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       MAX_DISTANCE_KEY,
                       (unsigned char *)&distance,
                       sizeof(distance));
  if (ECODE_NVM3_OK == err) {
    log_info("Stored max distance config: %d\r\n", distance);
    return SL_STATUS_OK;
  } else {
    log_info("Error storing max distance config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Max distance.
 ******************************************************************************/
uint16_t user_config_nvm3_get_max_distance(void)
{
  uint16_t distance;
  Ecode_t err;

  err = conf_data_u16_read(MAX_DISTANCE_KEY, &distance);
  app_assert_s(err == ECODE_NVM3_OK);
  return distance;
}

/***************************************************************************//**
 * Set NVM3 Distance Threshold.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_distance_threshold(uint16_t distance)
{
  Ecode_t err;

  if ((distance > DISTANCE_THRESHOLD_VALUE_MAX)
      || (distance < DISTANCE_THRESHOLD_VALUE_MIN)){
      log_info("Invalid distance threshold config: %d\r\n", distance);
      return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       DISTANCE_THRESHOLD_KEY,
                       (unsigned char *)&distance,
                       sizeof(distance));
  if (ECODE_NVM3_OK == err) {
    log_info("Stored distance threshold config: %d\r\n", distance);
    return SL_STATUS_OK;
  } else {
    log_info("Error storing distance threshold config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Distance Threshold.
 ******************************************************************************/
uint16_t user_config_nvm3_get_distance_threshold(void)
{
  uint16_t distance;
  Ecode_t err;

  err = conf_data_u16_read(DISTANCE_THRESHOLD_KEY, &distance);
  app_assert_s(err == ECODE_NVM3_OK);
  return distance;
}

/***************************************************************************//**
 * Set NVM3 Timing Budget.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_timing_budget(uint16_t timing_budget)
{
  Ecode_t err;

  if ((timing_budget > TIMING_BUDGET_VALUE_MAX)
      || (timing_budget < TIMING_BUDGET_VALUE_MIN)){
      log_info("Invalid timing budget config: %d\r\n", timing_budget);
      return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       TIMING_BUDGET_KEY,
                       (unsigned char *)&timing_budget,
                       sizeof(timing_budget));
  if (ECODE_NVM3_OK == err) {
    log_info("Stored timing budget config: %d\r\n", timing_budget);
    return SL_STATUS_OK;
  } else {
    log_info("Error storing timing budget config\r\n");
    return SL_STATUS_FAIL;
  }
}

/***************************************************************************//**
 * Get NVM3 Timing Budget.
 ******************************************************************************/
uint16_t user_config_nvm3_get_timing_budget(void)
{
  uint16_t timing_budget;
  Ecode_t err;

  err = conf_data_u16_read(TIMING_BUDGET_KEY, &timing_budget);
  app_assert_s(err == ECODE_NVM3_OK);
  return timing_budget;
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
    log_info("Stored notification active config: %d\r\n", data);
    return SL_STATUS_OK;
  } else {
    log_info("Error storing active config\r\n");
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

/***************************************************************************//**
 * Set NVM3 Room Capacity.
 ******************************************************************************/
sl_status_t user_config_nvm3_set_room_capacity(uint16_t room_capacity)
{
  Ecode_t err;

  if ((room_capacity > ROOM_CAPACITY_VALUE_MAX)
      || (room_capacity < ROOM_CAPACITY_VALUE_MIN)){
      log_info("Invalid room capacity config: %d\r\n", room_capacity);
      return SL_STATUS_INVALID_RANGE;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       ROOM_CAPACITY_KEY,
                       (unsigned char *)&room_capacity,
                       sizeof(room_capacity));
  if (ECODE_NVM3_OK == err) {
    log_info("Stored room capacity config: %d\r\n", room_capacity);
    return SL_STATUS_OK;
  } else {
    log_info("Error storing room capacity config\r\n");
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
  if (type == NVM3_OBJECTTYPE_DATA &&
      len == sizeof(uint8_t)) {
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
  if (type == NVM3_OBJECTTYPE_DATA &&
      len == sizeof(uint16_t)) {
      return nvm3_readData(NVM3_DEFAULT_HANDLE,
                          key,
                          u16_value,
                          sizeof(uint16_t));
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
  err = nvm3_writeData( NVM3_DEFAULT_HANDLE,
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
  err = nvm3_writeData( NVM3_DEFAULT_HANDLE,
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



/** @} (end group user_config_nvm3) */
