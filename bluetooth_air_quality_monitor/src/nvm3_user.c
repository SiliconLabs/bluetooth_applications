/***************************************************************************//**
 * @file nvm3_user.c
 * @brief NVM3 functions
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
#include <string.h>

#include "nvm3_default_config.h"
#include <nvm3_user.h>
#include "air_quality_app.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// Max and min keys for data objects
#define IS_NOTIFICATION_ACTIVE_KEY          (NVM3_KEY_MIN)
#define UPDATE_PERIOD_IN_SECOND_KEY         (NVM3_KEY_MIN + 1)
#define BUZZER_VOLUME_KEY                   (NVM3_KEY_MIN + 2)
#define THRESHOLD_CO2_PPM_KEY               (NVM3_KEY_MIN + 3)
#define THRESHOLD_TVOC_PPB_KEY              (NVM3_KEY_MIN + 4)

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE                 nvm3_defaultHandle

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

static Ecode_t nvm3_user_read_byte(nvm3_ObjectKey_t key, uint8_t *u8_value);
static Ecode_t nvm3_user_read_word(nvm3_ObjectKey_t key, uint16_t *u16_value);

static Ecode_t nvm3_user_read_byte(nvm3_ObjectKey_t key, uint8_t *u8_value)
{
  uint32_t type;
  size_t len;
  Ecode_t err;

  // Check if the designated keys contain data, and initialise if needed.
  err = nvm3_getObjectInfo(NVM3_DEFAULT_HANDLE,
                           key,
                           &type,
                           &len);

  if (err != ECODE_NVM3_OK) {
    return err;
  }

  if (type == NVM3_OBJECTTYPE_DATA) {
    return nvm3_readData(NVM3_DEFAULT_HANDLE,
                         key,
                         u8_value,
                         sizeof(uint8_t));
  }

  return ECODE_NVM3_ERR_OBJECT_IS_NOT_DATA;
}

static Ecode_t nvm3_user_read_word(nvm3_ObjectKey_t key, uint16_t *u16_value)
{
  uint32_t type;
  size_t len;
  Ecode_t err;

  // Check if the designated keys contain data, and initialise if needed.
  err = nvm3_getObjectInfo(NVM3_DEFAULT_HANDLE,
                           key,
                           &type,
                           &len);

  if (err != ECODE_NVM3_OK) {
    return err;
  }

  if (type == NVM3_OBJECTTYPE_DATA) {
    return nvm3_readData(NVM3_DEFAULT_HANDLE,
                         key,
                         u16_value,
                         sizeof(uint16_t));
  }

  return ECODE_NVM3_ERR_OBJECT_IS_NOT_DATA;
}

static void nvm3_user_init_byte(nvm3_ObjectKey_t key,
                                uint8_t min_value,
                                uint8_t max_value,
                                uint8_t default_value)
{
  Ecode_t err;
  uint8_t read_value;

  // Check if the designated keys contain data, and initialise if needed.
  err = nvm3_user_read_byte(key, &read_value);
  if ((err == ECODE_NVM3_OK)
      && (read_value >= min_value)
      && (read_value <= max_value)) {
    return;
  } else {
    nvm3_deleteObject(NVM3_DEFAULT_HANDLE, key);
  }

  nvm3_writeData(NVM3_DEFAULT_HANDLE,
                 key,
                 (unsigned char *)&default_value,
                 sizeof(default_value));
}

static void nvm3_user_init_word(nvm3_ObjectKey_t key,
                                uint16_t min_value,
                                uint16_t max_value,
                                uint16_t default_value)
{
  Ecode_t err;
  uint16_t read_value;

  // Check if the designated keys contain data, and initialise if needed.
  err = nvm3_user_read_word(key, &read_value);
  if ((err == ECODE_NVM3_OK)
      && (read_value >= min_value)
      && (read_value <= max_value)) {
    return;
  } else {
    nvm3_deleteObject(NVM3_DEFAULT_HANDLE, key);
  }

  nvm3_writeData(NVM3_DEFAULT_HANDLE,
                 key,
                 (unsigned char *)&default_value,
                 sizeof(default_value));
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize NVM3 example.
 ******************************************************************************/
void nvm3_user_init(void)
{
  Ecode_t err;

  // This will call nvm3_open() with default parameters for
  // memory base address and size, cache size, etc.
  err = nvm3_initDefault();
  EFM_ASSERT(err == ECODE_NVM3_OK);

  // Initialise the notification enable config.
  nvm3_user_init_byte(IS_NOTIFICATION_ACTIVE_KEY,
                      IS_NOTIFICATION_ACTIVE_MIN,
                      IS_NOTIFICATION_ACTIVE_MAX,
                      IS_NOTIFICATION_ACTIVE_DEFAULT);

  nvm3_user_init_byte(UPDATE_PERIOD_IN_SECOND_KEY,
                      UPDATE_PERIOD_IN_SECOND_MIN,
                      UPDATE_PERIOD_IN_SECOND_MAX,
                      UPDATE_PERIOD_IN_SECOND_DEFAULT);

  // Initialise the buzzer volume config.
  nvm3_user_init_byte(BUZZER_VOLUME_KEY,
                      BUZZER_VOLUME_MIN,
                      BUZZER_VOLUME_MAX,
                      BUZZER_VOLUME_DEFAULT);

  // Initialise the co2 threshold config.
  nvm3_user_init_word(THRESHOLD_CO2_PPM_KEY,
                      THRESHOLD_CO2_PPM_MIN,
                      THRESHOLD_CO2_PPM_MAX,
                      THRESHOLD_CO2_PPM_DEFAULT);

  // Initialise the tvoc threshold config.
  nvm3_user_init_word(THRESHOLD_TVOC_PPB_KEY,
                      THRESHOLD_TVOC_PPB_MIN,
                      THRESHOLD_TVOC_PPB_MAX,
                      THRESHOLD_TVOC_PPB_DEFAULT);
}

/***************************************************************************//**
 *  Set the notification status to NVM.
 ******************************************************************************/
Ecode_t nvm3_user_set_notification_active(uint8_t enable)
{
  uint8_t data = enable ? 1 : 0;
  Ecode_t err;

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       IS_NOTIFICATION_ACTIVE_KEY,
                       (unsigned char *)&data,
                       sizeof(data));
  return err;
}

/***************************************************************************//**
 *  Get the notification status from NVM.
 ******************************************************************************/
Ecode_t nvm3_user_get_notification_active(uint8_t *enable)
{
  Ecode_t err;

  err = nvm3_user_read_byte(IS_NOTIFICATION_ACTIVE_KEY, enable);
  return err;
}

/***************************************************************************//**
 *  Set the Air quality monitor sleep timer period in seconds to NVM.
 ******************************************************************************/
Ecode_t nvm3_user_set_update_period(uint8_t period_sec)
{
  Ecode_t err;

  if ((period_sec > UPDATE_PERIOD_IN_SECOND_MAX)
      || (period_sec < UPDATE_PERIOD_IN_SECOND_MIN)) {
    return ECODE_NVM3_ERR_PARAMETER;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       UPDATE_PERIOD_IN_SECOND_KEY,
                       (unsigned char *)&period_sec,
                       sizeof(period_sec));
  return err;
}

/***************************************************************************//**
 *  Get the Air quality monitor sleep timer period in seconds from NVM.
 ******************************************************************************/
Ecode_t nvm3_user_get_update_period(uint8_t *update_period)
{
  Ecode_t err;

  err = nvm3_user_read_byte(UPDATE_PERIOD_IN_SECOND_KEY, update_period);
  return err;
}

/***************************************************************************//**
 *  Set the buzzer volume to NVM.
 ******************************************************************************/
Ecode_t nvm3_user_set_buzzer_volume(uint8_t volume)
{
  Ecode_t err;

  if ((volume > BUZZER_VOLUME_MAX) || (volume < BUZZER_VOLUME_MIN)) {
    return ECODE_NVM3_ERR_PARAMETER;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       BUZZER_VOLUME_KEY,
                       (unsigned char *)&volume,
                       sizeof(volume));
  return err;
}

/***************************************************************************//**
 *  Get the buzzer volume from NVM.
 ******************************************************************************/
Ecode_t nvm3_user_get_buzzer_volume(uint8_t *volume)
{
  Ecode_t err;

  err = nvm3_user_read_byte(BUZZER_VOLUME_KEY, volume);
  return err;
}

/***************************************************************************//**
 *  Set the notification threshold for CO2 level in ppm to NVM.
 ******************************************************************************/
Ecode_t nvm3_user_set_threshold_co2(uint16_t threshold_co2)
{
  Ecode_t err;

  if ((threshold_co2 > THRESHOLD_CO2_PPM_MAX)
      || (threshold_co2 < THRESHOLD_CO2_PPM_MIN)) {
    return ECODE_NVM3_ERR_PARAMETER;
  }

  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       THRESHOLD_CO2_PPM_KEY,
                       (unsigned char *)&threshold_co2,
                       sizeof(threshold_co2));
  return err;
}

/***************************************************************************//**
 *  Get the notification threshold for CO2 level in ppm from NVM.
 ******************************************************************************/
Ecode_t nvm3_user_get_threshold_co2(uint16_t *threshold_co2)
{
  Ecode_t err;

  err = nvm3_user_read_word(THRESHOLD_CO2_PPM_KEY, threshold_co2);
  return err;
}

/***************************************************************************//**
 *  Set the notification threshold for tVOC level in ppb to NVM.
 ******************************************************************************/
Ecode_t nvm3_user_set_threshold_tvoc(uint16_t threshold_tvoc)
{
  Ecode_t err;

  if ((threshold_tvoc > THRESHOLD_TVOC_PPB_MAX)
      || (threshold_tvoc < THRESHOLD_TVOC_PPB_MIN)) {
    return ECODE_NVM3_ERR_PARAMETER;
  }
  err = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                       THRESHOLD_TVOC_PPB_KEY,
                       (unsigned char *)&threshold_tvoc,
                       sizeof(threshold_tvoc));
  return err;
}

/***************************************************************************//**
 *  Get the notification threshold for tVOC level in ppb from NVM.
 ******************************************************************************/
Ecode_t nvm3_user_get_threshold_tvoc(uint16_t *threshold_tvoc)
{
  Ecode_t err;

  err = nvm3_user_read_word(THRESHOLD_TVOC_PPB_KEY, threshold_tvoc);
  return err;
}

/***************************************************************************//**
 * NVM3 ticking function.
 ******************************************************************************/
void nvm3_user_process_action(void)
{
  // Check if NVM3 controller can release any out-of-date objects
  // to free up memory.
  // This may take more than one call to nvm3_repack()
  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE)) {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }
}
