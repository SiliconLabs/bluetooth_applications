/***************************************************************************//**
 * @file env.h
 * @brief NVM3-based environment variable storage
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: LicenseRef-MSLA
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement
 * By installing, copying or otherwise using this software, you agree to the
 * terms of the MSLA.
 *
 ******************************************************************************/

#ifndef ENV__H
#define ENV__H

#include <sl_status.h>

#define ENV_MAX_FOLLOWERS       8
#define ENV_DEVICE_NAME_LEN     8

typedef enum {
  ENV_DEVICE_NONE,
  ENV_DEVICE_PEPS_LEADER,
  ENV_DEVICE_PEPS_FOLLOWER,
  ENV_DEVICE_LAST,
} env_device_type_t;
extern const char * const env_device_type_names[ENV_DEVICE_LAST];

#define ENV_LOCATION_LITE

typedef enum {
#if !defined(ENV_LOCATION_LITE)
  ENV_LOCATION_FRONT,
#endif
  ENV_LOCATION_RIGHT_FRONT,
#if !defined(ENV_LOCATION_LITE)
  ENV_LOCATION_RIGHT_CENTER,
#endif
  ENV_LOCATION_RIGHT_REAR,
#if !defined(ENV_LOCATION_LITE)
  ENV_LOCATION_REAR,
#endif
  ENV_LOCATION_LEFT_REAR,
#if !defined(ENV_LOCATION_LITE)
  ENV_LOCATION_LEFT_CENTER,
#endif
  ENV_LOCATION_LEFT_FRONT,
  ENV_LOCATION_CENTER,
  ENV_LOCATION_LAST,
} env_device_location_t;
extern const char * const env_device_location_names[ENV_LOCATION_LAST];

typedef char env_device_name_t[ENV_DEVICE_NAME_LEN];

typedef struct env_location_params {
  float coeff;
  float offset;
  float x;
  float y;
} env_location_params_t;

extern env_device_name_t env_device_name;
sl_status_t env_set_device_name(const env_device_name_t name);

extern env_device_type_t env_device_type;
sl_status_t env_set_device_type(env_device_type_t type);

extern unsigned char env_base_address;
sl_status_t env_set_base_address(unsigned char addr);

extern unsigned char env_follower_count;
extern env_device_type_t env_follower_type[ENV_MAX_FOLLOWERS];
extern unsigned char env_follower_address[ENV_MAX_FOLLOWERS];
extern env_device_name_t env_follower_name[ENV_MAX_FOLLOWERS];
sl_status_t env_add_follower(unsigned char address,
                             env_device_type_t type,
                             const env_device_name_t name);
sl_status_t env_remove_follower_by_address(unsigned char address);
sl_status_t env_remove_follower_by_name(const env_device_name_t name);
sl_status_t env_remove_followers(void);

extern unsigned char env_location[ENV_LOCATION_LAST];
sl_status_t env_add_location_by_address(env_device_location_t location,
                                        unsigned char address);
sl_status_t env_add_location_by_name(env_device_location_t location,
                                     const env_device_name_t name);
sl_status_t env_remove_location_by_address(unsigned char address);
sl_status_t env_remove_location_by_name(const env_device_name_t name);
sl_status_t env_remove_location(env_device_location_t location);
sl_status_t env_remove_locations(void);

extern env_location_params_t env_location_params[ENV_LOCATION_LAST];
sl_status_t env_location_set_coord(env_device_location_t location,
                                   float x,
                                   float y);
sl_status_t env_location_set_calib(env_device_location_t location,
                                   float coeff,
                                   float offset);

sl_status_t env_init(void);

void env_pad_name(env_device_name_t name);
void env_pad_name_from(env_device_name_t out, const env_device_name_t name);

sl_status_t env_set_lfxo_ctune(unsigned int val);
sl_status_t env_get_lfxo_ctune(unsigned int *val);
sl_status_t env_clear_lfxo_ctune(void);

#endif
