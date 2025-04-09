/***************************************************************************//**
 * @file env.c
 * @brief NVM3-based environment variable storage
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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

#include "env.h"

#include "lin/sl_lin_common.h"

#include "app_log.h"

#include "nvm3_default.h"
#include "nvm3_default_config.h"

#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"

#include <string.h>

// Maximum number of data objects saved
#define ENV_MAX_OBJECT_COUNT    100

// Max and min keys for data objects
#define ENV_MIN_KEY             NVM3_KEY_MIN
#define ENV_MAX_KEY             (ENV_MIN_KEY + MAX_OBJECT_COUNT - 1)

// Device name [8 character string]
#define ENV_DEVICE_NAME_KEY     (ENV_MIN_KEY + 0)
env_device_name_t env_device_name;

// Device type [enum, 2 bytes]
#define ENV_DEVICE_TYPE_KEY     (ENV_DEVICE_NAME_KEY + 1)
env_device_type_t env_device_type;

// Base address [1 byte]
#define ENV_BASE_ADDRESS_KEY    (ENV_DEVICE_TYPE_KEY + 1)
unsigned char env_base_address;
static unsigned char env_base_address_def = SL_LIN_INVALID_ENDPOINT;

// Number of followers on the LIN bus [1 byte]
#define ENV_FOLLOWER_COUNT_KEY  (ENV_BASE_ADDRESS_KEY + 1)
unsigned char env_follower_count;
static unsigned char env_follower_count_def = 0;

// Device type [enum, 2 bytes]
// this is the start of a list of ENV_MAX_FOLLOWERS keys
#define ENV_FOLLOWER_TYPE_KEY   (ENV_FOLLOWER_COUNT_KEY + 1)
env_device_type_t env_follower_type[ENV_MAX_FOLLOWERS];

// Device base address [1 byte]
// this is the start of a list of ENV_MAX_FOLLOWERS keys
#define ENV_FOLLOWER_ADDR_KEY   (ENV_FOLLOWER_TYPE_KEY + ENV_MAX_FOLLOWERS)
unsigned char env_follower_address[ENV_MAX_FOLLOWERS];

#define ENV_FOLLOWER_NAME_KEY   (ENV_FOLLOWER_ADDR_KEY + ENV_MAX_FOLLOWERS)
env_device_name_t env_follower_name[ENV_MAX_FOLLOWERS];

#define ENV_LOCATION_KEY        (ENV_FOLLOWER_NAME_KEY + ENV_MAX_FOLLOWERS)
unsigned char env_location[ENV_LOCATION_LAST];

#define ENV_LOCATION_PARAMS_KEY (ENV_LOCATION_KEY + ENV_LOCATION_LAST)
env_location_params_t env_location_params[ENV_LOCATION_LAST];

#define ENV_LFXO_CTUNE_KEY      (ENV_LOCATION_PARAMS_KEY + ENV_LOCATION_LAST)

#define ENV_NEXT_FREE_KEY       (ENV_LFXO_CTUNE_KEY + 1)

// Use the default nvm3 handle from nvm3_default.h
#define NVM3_DEFAULT_HANDLE     nvm3_defaultHandle

typedef struct {
  unsigned int key;
  void *var;
  size_t size;
  void *def;
} env_keyset_t;

static env_keyset_t env_keyset[] = {
  { .key = ENV_DEVICE_NAME_KEY,
    .var = &env_device_name,
    .size = sizeof(env_device_name),
    .def = NULL, },
  { .key = ENV_DEVICE_TYPE_KEY,
    .var = &env_device_type,
    .size = sizeof(env_device_type),
    .def = NULL, },
  { .key = ENV_BASE_ADDRESS_KEY,
    .var = &env_base_address,
    .size = sizeof(env_base_address),
    .def = &env_base_address_def, },
  { .key = ENV_FOLLOWER_COUNT_KEY,
    .var = &env_follower_count,
    .size = sizeof(env_follower_count),
    .def = &env_follower_count_def, },
};

const char * const env_device_type_names[ENV_DEVICE_LAST] = {
  "NONE",
  "PEPS_LEADER",
  "PEPS_FOLLOWER",
};

const char * const env_device_location_names[ENV_LOCATION_LAST] = {
#if !defined(ENV_LOCATION_LITE)
  "FRONT",
#endif
  "RIGHT_FRONT",
#if !defined(ENV_LOCATION_LITE)
  "RIGHT_CENTER",
#endif
  "RIGHT_REAR",
#if !defined(ENV_LOCATION_LITE)
  "REAR",
#endif
  "LEFT_REAR",
#if !defined(ENV_LOCATION_LITE)
  "LEFT_CENTER",
#endif
  "LEFT_FRONT",
  "CENTER",
};

#define ARRAY_SIZE(a)           (sizeof(a) / sizeof(a[0]))

void env_pad_name(env_device_name_t name)
{
  unsigned int i;

  for (i = 0; i < ENV_DEVICE_NAME_LEN; i++)
  {
    if (name[i] == '\0') {
      unsigned int j;

      for (j = (i + 1); j < ENV_DEVICE_NAME_LEN; j++)
      {
        name[i] = '\0';
      }

      break;
    }
  }
}

void env_pad_name_from(env_device_name_t out, const env_device_name_t name)
{
  unsigned int i;

  for (i = 0; i < ENV_DEVICE_NAME_LEN; i++)
  {
    if (name[i] == '\0') {
      unsigned int j;

      for (j = i; j < ENV_DEVICE_NAME_LEN; j++)
      {
        out[i] = '\0';
      }

      break;
    }

    out[i] = name[i];
  }
}

static sl_status_t env_set_follower_count(unsigned int entries)
{
  Ecode_t ecode;

  if (env_follower_count == entries) {
    return SL_STATUS_OK;
  }

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_COUNT_KEY,
                         &entries,
                         sizeof(env_follower_count));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s to nvm!\n",
                  "ENV_FOLLOWER_COUNT_KEY");
    return SL_STATUS_IO;
  }

  env_follower_count = entries;

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  return SL_STATUS_OK;
}

static sl_status_t env_move_follower(unsigned int dest,
                                     unsigned int src,
                                     bool do_repack)
{
  Ecode_t ecode;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_TYPE_KEY + dest,
                         &env_follower_type[src],
                         sizeof(env_device_type));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_TYPE_KEY",
                  dest);
    return SL_STATUS_IO;
  }

  env_follower_type[dest] = env_follower_type[src];
  env_follower_type[src] = ENV_DEVICE_NONE;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_TYPE_KEY + src,
                         &env_follower_type[src],
                         sizeof(env_device_type));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_TYPE_KEY",
                  src);
    return SL_STATUS_IO;
  }

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_NAME_KEY + dest,
                         &env_follower_name[src],
                         sizeof(env_device_name));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_NAME_KEY",
                  dest);
    return SL_STATUS_IO;
  }

  memcpy(env_follower_name[dest],
         env_follower_name[src],
         sizeof(env_device_name));
  memset(env_follower_name[src], 0, sizeof(env_device_name));

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_NAME_KEY + src,
                         &env_follower_name[src],
                         sizeof(env_device_name));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_NAME_KEY",
                  src);
    return SL_STATUS_IO;
  }

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_ADDR_KEY + dest,
                         &env_follower_address[src],
                         sizeof(env_base_address));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_ADDR_KEY",
                  dest);
    return SL_STATUS_IO;
  }

  env_follower_address[dest] = env_follower_address[src];
  env_follower_address[src] = SL_LIN_INVALID_ENDPOINT;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_ADDR_KEY + src,
                         &env_follower_address[src],
                         sizeof(env_base_address));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_ADDR_KEY",
                  src);
    return SL_STATUS_IO;
  }

  while (do_repack && nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  return SL_STATUS_OK;
}

static sl_status_t env_clear_follower_by_index(unsigned int idx)
{
  Ecode_t ecode;
  sl_status_t ret = SL_STATUS_OK;

  env_follower_type[idx] = ENV_DEVICE_NONE;
  env_follower_address[idx] = SL_LIN_INVALID_ENDPOINT;
  memset(env_follower_name[idx], 0, sizeof(env_device_name));

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_NAME_KEY + idx,
                         &env_follower_name[idx],
                         sizeof(env_device_name));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_NAME_KEY",
                  idx);
    ret = SL_STATUS_IO;
  }

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_ADDR_KEY + idx,
                         &env_follower_address[idx],
                         sizeof(env_base_address));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_ADDR_KEY",
                  idx);
    ret = SL_STATUS_IO;
  }

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_TYPE_KEY + idx,
                         &env_follower_type[idx],
                         sizeof(env_device_type));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_TYPE_KEY",
                  idx);
    ret = SL_STATUS_IO;
  }

  return ret;
}

static sl_status_t env_remove_follower_by_index(unsigned int idx)
{
  sl_status_t res, ret = SL_STATUS_OK;

  if (idx >= env_follower_count) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid index %u!\n", idx);
    return SL_STATUS_INVALID_INDEX;
  }

  if (idx < (env_follower_count - 1U)) {
    res = env_move_follower(idx, env_follower_count - 1U, false);
    if (res != SL_STATUS_OK) {
      app_log_level(APP_LOG_LEVEL_ERROR, "Failed to move follower %u!\n", idx);
      ret = res;
    }
  } else {
    res = env_clear_follower_by_index(idx);
    if (res != SL_STATUS_OK) {
      app_log_level(APP_LOG_LEVEL_ERROR, "Failed to clear follower %u!\n", idx);
      ret = res;
    }
  }

  res = env_set_follower_count(env_follower_count - 1U);
  if (res != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to update follower count!\n");
    ret = res;
  }

  return ret;
}

sl_status_t env_set_device_name(const env_device_name_t name)
{
  Ecode_t ecode;
  env_device_name_t padded_name;

  env_pad_name_from(padded_name, name);

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_DEVICE_NAME_KEY,
                         padded_name,
                         sizeof(env_device_name));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s to nvm!\n",
                  "ENV_DEVICE_NAME_KEY");
    return SL_STATUS_IO;
  }

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  memcpy(env_device_name, padded_name, sizeof(env_device_name));

  return SL_STATUS_OK;
}

sl_status_t env_set_device_type(env_device_type_t type)
{
  Ecode_t ecode;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_DEVICE_TYPE_KEY,
                         &type,
                         sizeof(env_device_type));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s to nvm!\n",
                  "ENV_DEVICE_TYPE_KEY");
    return SL_STATUS_IO;
  }

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  env_device_type = type;

  return SL_STATUS_OK;
}

sl_status_t env_set_base_address(unsigned char addr)
{
  Ecode_t ecode;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_BASE_ADDRESS_KEY,
                         &addr,
                         sizeof(env_base_address));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s to nvm!\n",
                  "ENV_BASE_ADDRESS_KEY");
    return SL_STATUS_IO;
  }

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  env_base_address = addr;

  return SL_STATUS_OK;
}

sl_status_t env_add_follower(unsigned char address,
                             env_device_type_t type,
                             const env_device_name_t name)
{
  Ecode_t ecode;
  unsigned int i;
  env_device_name_t padded_name;

  env_pad_name_from(padded_name, name);

  if (env_follower_count == ENV_MAX_FOLLOWERS) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Follower slots are full!\n");
    return SL_STATUS_NO_MORE_RESOURCE;
  }

  for (i = 0; i < ENV_MAX_FOLLOWERS; i++)
  {
    if ((env_follower_type[i] == ENV_DEVICE_NONE)
        || (env_follower_name[i][0] == '\0')) {
      break;
    }

    if ((env_follower_address[i] == address)
        || (strncmp(env_follower_name[i], padded_name,
                    sizeof(env_device_name)) == 0)) {
      app_log_level(APP_LOG_LEVEL_ERROR,
                    "Follower address or name already exists!\n");
      return SL_STATUS_ALREADY_EXISTS;
    }
  }

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_ADDR_KEY + i,
                         &address,
                         sizeof(env_base_address));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_ADDR_KEY",
                  i);
    return SL_STATUS_IO;
  }

  env_follower_address[i] = address;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_TYPE_KEY + i,
                         &type,
                         sizeof(env_device_type));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_TYPE_KEY",
                  i);
    return SL_STATUS_IO;
  }

  env_follower_type[i] = type;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_FOLLOWER_NAME_KEY + i,
                         padded_name,
                         sizeof(env_device_name));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_FOLLOWER_NAME_KEY",
                  i);
    return SL_STATUS_IO;
  }

  memcpy(env_follower_name[i], padded_name, sizeof(env_device_name));

  // repack is done by this:
  env_set_follower_count(env_follower_count + 1);

  return SL_STATUS_OK;
}

sl_status_t env_remove_follower_by_address(unsigned char address)
{
  sl_status_t ret;
  unsigned int i;

  for (i = 0; i < env_follower_count; i++)
  {
    if (env_follower_address[i] == address) {
      break;
    }
  }

  if (i >= env_follower_count) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Unknown follower address 0x%02x!\n",
                  address);
    return SL_STATUS_NOT_FOUND;
  }

  ret = env_remove_follower_by_index(i);
  env_remove_location_by_address(address);
  return ret;
}

sl_status_t env_remove_follower_by_name(const env_device_name_t name)
{
  unsigned int i;
  env_device_name_t padded_name;

  env_pad_name_from(padded_name, name);

  for (i = 0; i < env_follower_count; i++)
  {
    if (strncmp(env_follower_name[i], padded_name,
                sizeof(env_device_name)) == 0) {
      break;
    }
  }

  if (i >= env_follower_count) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Unknown follower name '%-*.s'!\n",
                  sizeof(env_device_name),
                  padded_name);
    return SL_STATUS_NOT_FOUND;
  }

  return env_remove_follower_by_index(i);
}

sl_status_t env_remove_followers(void)
{
  sl_status_t res, ret = SL_STATUS_OK;
  unsigned int i;

  for (i = 0; i < ENV_MAX_FOLLOWERS; i++)
  {
    res = env_clear_follower_by_index(i);
    if (res != SL_STATUS_OK) {
      ret = res;
    }
  }

  res = env_set_follower_count(0);
  if (res != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to update follower count!\n");
    ret = res;
  }

  return ret;
}

sl_status_t env_add_location_by_address(env_device_location_t location,
                                        unsigned char address)
{
  Ecode_t ecode;
  unsigned int i;

  if (env_location[location] <= SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Location %s already in use!\n",
                  env_device_location_names[location]);
    return SL_STATUS_ALREADY_EXISTS;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (env_location[i] == address) {
      app_log_level(APP_LOG_LEVEL_ERROR,
                    "Address 0x%02x already exists in the location table!\n",
                    address);
      return SL_STATUS_ALREADY_EXISTS;
    }
  }

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_LOCATION_KEY + location,
                         &address,
                         sizeof(env_location[0]));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_LOCATION_KEY",
                  location);
    return SL_STATUS_IO;
  }

  env_location[location] = address;

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  return SL_STATUS_OK;
}

sl_status_t env_add_location_by_name(env_device_location_t location,
                                     const env_device_name_t name)
{
  env_device_name_t padded_name;
  unsigned int i;

  env_pad_name_from(padded_name, name);

  for (i = 0; i < env_follower_count; i++)
  {
    if (strncmp(env_follower_name[i], padded_name,
                sizeof(env_device_name)) == 0) {
      return env_add_location_by_address(location, env_follower_address[i]);
    }
  }

  app_log_level(APP_LOG_LEVEL_ERROR, "Follower '%-.*s' not found!\n",
                sizeof(env_device_name), padded_name);
  return SL_STATUS_NOT_FOUND;
}

sl_status_t env_remove_location_by_address(unsigned char address)
{
  unsigned int i;

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (env_location[i] == address) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Location not found for address 0x%02x!\n",
                  address);
    return SL_STATUS_NOT_FOUND;
  }

  return env_remove_location(i);
}

sl_status_t env_remove_location_by_name(const env_device_name_t name)
{
  env_device_name_t padded_name;
  unsigned int i;

  env_pad_name_from(padded_name, name);

  for (i = 0; i < env_follower_count; i++)
  {
    if (strncmp(env_follower_name[i], padded_name,
                sizeof(env_device_name)) == 0) {
      return env_remove_location_by_address(env_follower_address[i]);
    }
  }

  app_log_level(APP_LOG_LEVEL_ERROR, "Follower '%-.*s' not found!\n",
                sizeof(env_device_name), padded_name);
  return SL_STATUS_NOT_FOUND;
}

sl_status_t env_remove_location(env_device_location_t location)
{
  Ecode_t ecode;
  unsigned char address = SL_LIN_INVALID_ENDPOINT;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_LOCATION_KEY + location,
                         &address,
                         sizeof(address));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_LOCATION_KEY",
                  location);
    return SL_STATUS_IO;
  }

  env_location[location] = address;

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  return SL_STATUS_OK;
}

sl_status_t env_remove_locations(void)
{
  sl_status_t res, ret = SL_STATUS_OK;
  unsigned int i;

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    res = env_remove_location(i);
    if (res != SL_STATUS_OK) {
      ret = res;
    }
  }

  return ret;
}

sl_status_t env_location_set_coord(env_device_location_t location,
                                   float x,
                                   float y)
{
  env_location_params_t params = env_location_params[location];
  Ecode_t ecode;

  params.x = x;
  params.y = y;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_LOCATION_PARAMS_KEY + location,
                         &params,
                         sizeof(params));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_LOCATION_PARAMS_KEY",
                  location);
    return SL_STATUS_IO;
  }

  env_location_params[location] = params;

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  return SL_STATUS_OK;
}

sl_status_t env_location_set_calib(env_device_location_t location,
                                   float coeff,
                                   float offset)
{
  env_location_params_t params = env_location_params[location];
  Ecode_t ecode;

  params.coeff = coeff;
  params.offset = offset;

  ecode = nvm3_writeData(NVM3_DEFAULT_HANDLE,
                         ENV_LOCATION_PARAMS_KEY + location,
                         &params,
                         sizeof(params));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s[%u] to nvm!\n",
                  "ENV_LOCATION_PARAMS_KEY",
                  location);
    return SL_STATUS_IO;
  }

  env_location_params[location] = params;

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  return SL_STATUS_OK;
}

sl_status_t env_set_lfxo_ctune(unsigned int val)
{
  Ecode_t ecode;
  unsigned int current;

  ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                        ENV_LFXO_CTUNE_KEY,
                        &current,
                        sizeof(current));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_INFO,
                  "Failed to read key %s, ignoring.\n",
                  "ENV_LFXO_CTUNE_KEY");
  } else if (current == val) {
    return SL_STATUS_OK;
  }

  ecode =
    nvm3_writeData(NVM3_DEFAULT_HANDLE, ENV_LFXO_CTUNE_KEY, &val, sizeof(val));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to write %s to nvm!\n",
                  "ENV_LFXO_CTUNE_KEY");
    return SL_STATUS_IO;
  }

  while (nvm3_repackNeeded(NVM3_DEFAULT_HANDLE))
  {
    nvm3_repack(NVM3_DEFAULT_HANDLE);
  }

  return SL_STATUS_OK;
}

sl_status_t env_get_lfxo_ctune(unsigned int *val)
{
  Ecode_t ecode;
  unsigned int current;

  ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                        ENV_LFXO_CTUNE_KEY,
                        &current,
                        sizeof(current));
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_INFO,
                  "Failed to read key %s, ignoring.\n",
                  "ENV_LFXO_CTUNE_KEY");
    return SL_STATUS_FAIL;
  }

  *val = current;

  return SL_STATUS_OK;
}

sl_status_t env_clear_lfxo_ctune(void)
{
  Ecode_t ecode;

  ecode = nvm3_deleteObject(NVM3_DEFAULT_HANDLE, ENV_LFXO_CTUNE_KEY);
  if (ecode != ECODE_NVM3_OK) {
    app_log_level(APP_LOG_LEVEL_INFO,
                  "Failed to delete key %s, ignoring.\n",
                  "ENV_LFXO_CTUNE_KEY");
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

static void env_device_set_name_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: env device set name <name>\n");
  app_log_nl();
}

static void env_device_set_name(sl_cli_command_arg_t *arguments)
{
  env_device_name_t padded_name;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    env_device_set_name_help();
    return;
  }

  env_pad_name_from(padded_name, sl_cli_get_argument_string(arguments, 0));

  ret = env_set_device_name(padded_name);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void env_device_set_type_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: env device set type <type>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <type> is one of the following:\n");

  for (i = 0; i < ENV_DEVICE_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_type_names[i]);
  }

  app_log_nl();
}

static void env_device_set_type(sl_cli_command_arg_t *arguments)
{
  env_device_type_t type;
  unsigned int i;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    env_device_set_type_help();
    return;
  }

  for (i = 0; i < ENV_DEVICE_LAST; i++)
  {
    if (strcmp(env_device_type_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_DEVICE_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid device type!\n");
    env_device_set_type_help();
    return;
  }

  type = (env_device_type_t)i;

  ret = env_set_device_type(type);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void env_device_set_address_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: env device set address <address>\n");
  app_log_nl();
}

static void env_device_set_address(sl_cli_command_arg_t *arguments)
{
  unsigned long addr;
  char *endp = NULL;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    env_device_set_address_help();
    return;
  }

  addr = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (addr > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    env_device_set_address_help();
    return;
  }

  ret = env_set_base_address(addr);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void env_device_info(sl_cli_command_arg_t *arguments)
{
  (void)arguments;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Device type: %-13s address: 0x%02x name: %-.*s\n",
                env_device_type_names[env_device_type],
                env_base_address,
                sizeof(env_device_name),
                env_device_name);

  app_log_nl();
}

static void reset_device(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  NVIC_SystemReset();
}

static const sl_cli_command_info_t cmd_env_device_set_name = \
  SL_CLI_COMMAND(env_device_set_name,
                 "set the device name",
                 "<name> - name to set",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_env_device_set_type = \
  SL_CLI_COMMAND(env_device_set_type,
                 "set the device type",
                 "<type> - type to set",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_env_device_set_address = \
  SL_CLI_COMMAND(env_device_set_address,
                 "set the device address",
                 "<address> - address to set",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_env_device_set[] = {
  { "name", &cmd_env_device_set_name, false, },
  { "type", &cmd_env_device_set_type, false, },
  { "address", &cmd_env_device_set_address, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_env_device_set = \
  SL_CLI_COMMAND_GROUP(&cmdtable_env_device_set,
                       "set the device attributes");

static const sl_cli_command_info_t cmd_env_device_info = \
  SL_CLI_COMMAND(env_device_info,
                 "show info of this device",
                 "",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_env_device[] = {
  { "set", &cmd_env_device_set, false, },
  { "info", &cmd_env_device_info, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_env_device = \
  SL_CLI_COMMAND_GROUP(&cmdtable_env_device,
                       "device-related commands");

static sl_cli_command_entry_t cmdtable_env[] = {
  { "device", &cmd_env_device, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_env = \
  SL_CLI_COMMAND_GROUP(&cmdtable_env,
                       "environment management");

static const sl_cli_command_info_t cmd_reset = \
  SL_CLI_COMMAND(reset_device,
                 "reset the device",
                 "",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable[] = {
  { "env", &cmd_env, false, },
  { "reset", &cmd_reset, false, },
  { NULL, NULL, false, },
};

static sl_cli_command_group_t cmdgroup = {
  { NULL },
  false,
  cmdtable,
};

sl_status_t env_init(void)
{
  Ecode_t ecode;
  unsigned int i;
  unsigned int entries = 0;

  ecode = nvm3_initDefault();
  if (ecode != ECODE_NVM3_OK) {
    return SL_STATUS_IO;
  }

  for (i = 0; i < ARRAY_SIZE(env_keyset); i++)
  {
    env_keyset_t *keyset = &env_keyset[i];

    ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                          keyset->key,
                          keyset->var,
                          keyset->size);
    if (ecode != ECODE_NVM3_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to read key 0x%08x, using default.\n",
                    keyset->key);

      if (keyset->def) {
        memcpy(keyset->var, keyset->def, keyset->size);
      } else {
        memset(keyset->var, 0, keyset->size);
      }
    }
  }

  for (i = 0; i < ENV_MAX_FOLLOWERS; i++)
  {
    ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                          ENV_FOLLOWER_TYPE_KEY + i,
                          &env_follower_type[i],
                          sizeof(env_device_type));
    if (ecode != ECODE_NVM3_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to read key %s[%u], ignoring.\n",
                    "ENV_FOLLOWER_TYPE_KEY",
                    i);
      env_follower_type[i] = ENV_DEVICE_NONE;
      memset(env_follower_name[i], 0, sizeof(env_device_name));
      continue;
    }

    ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                          ENV_FOLLOWER_ADDR_KEY + i,
                          &env_follower_address[i],
                          sizeof(env_base_address));
    if (ecode != ECODE_NVM3_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to read key %s[%u], ignoring.\n",
                    "ENV_FOLLOWER_ADDR_KEY",
                    i);
      env_follower_type[i] = ENV_DEVICE_NONE;
      memset(env_follower_name[i], 0, sizeof(env_device_name));
      continue;
    }

    ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                          ENV_FOLLOWER_NAME_KEY + i,
                          &env_follower_name[i],
                          sizeof(env_device_name));
    if (ecode != ECODE_NVM3_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to read key %s[%u], ignoring.\n",
                    "ENV_FOLLOWER_NAME_KEY",
                    i);
      env_follower_type[i] = ENV_DEVICE_NONE;
      memset(env_follower_name[i], 0, sizeof(env_device_name));
      continue;
    }

    env_pad_name(env_follower_name[i]);
  }

  entries = 0;
  while (entries < ENV_MAX_FOLLOWERS)
  {
    if ((env_follower_type[entries] == ENV_DEVICE_NONE)
        || (env_follower_address[entries] > SL_LIN_MAX_ENDPOINT)
        || (env_follower_name[entries][0] == '\0')) {
      unsigned int j;

      for (j = (entries + 1); j < ENV_MAX_FOLLOWERS; j++)
      {
        if ((env_follower_type[j] != ENV_DEVICE_NONE)
            && (env_follower_address[j] <= SL_LIN_MAX_ENDPOINT)
            && (env_follower_name[j][0] != '\0')) {
          env_move_follower(entries, j, false);
          entries++;
          break;
        } else if ((env_follower_type[j] != ENV_DEVICE_NONE)
                   || (env_follower_address[j] <= SL_LIN_MAX_ENDPOINT)
                   || (env_follower_name[j][0] != '\0')) {
          env_clear_follower_by_index(j);
        }
      }

      if (j >= ENV_MAX_FOLLOWERS) {
        if ((env_follower_type[entries] != ENV_DEVICE_NONE)
            || (env_follower_address[entries] <= SL_LIN_MAX_ENDPOINT)
            || (env_follower_name[entries][0] != '\0')) {
          env_clear_follower_by_index(entries);
        }
        break;
      }
    } else {
      unsigned int j;

      for (j = (entries + 1); j < ENV_MAX_FOLLOWERS; j++)
      {
        if ((env_follower_address[entries] == env_follower_address[j])
            || (strncmp(env_follower_name[entries], env_follower_name[j],
                        sizeof(env_device_name)) == 0)) {
          env_clear_follower_by_index(j);
        }
      }

      entries++;
    }
  }

  env_set_follower_count(entries);

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                          ENV_LOCATION_KEY + i,
                          &env_location[i],
                          sizeof(env_location[0]));
    if (ecode != ECODE_NVM3_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to read key %s[%u], ignoring.\n",
                    "ENV_LOCATION_KEY",
                    i);
      env_location[i] = SL_LIN_INVALID_ENDPOINT;
    }
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    ecode = nvm3_readData(NVM3_DEFAULT_HANDLE,
                          ENV_LOCATION_PARAMS_KEY + i,
                          &env_location_params[i],
                          sizeof(env_location_params[0]));
    if (ecode != ECODE_NVM3_OK) {
      app_log_level(APP_LOG_LEVEL_INFO,
                    "Failed to read key %s[%u], ignoring.\n",
                    "ENV_LOCATION_PARAMS_KEY",
                    i);
      env_location_params[i].x = 0;
      env_location_params[i].y = 0;
      env_location_params[i].coeff = 0;
      env_location_params[i].offset = 0;
    }
  }

  sl_cli_command_add_command_group(sl_cli_inst_handle, &cmdgroup);

  return SL_STATUS_OK;
}
