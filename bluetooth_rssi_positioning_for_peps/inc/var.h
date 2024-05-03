/***************************************************************************//**
 * @file var.h
 * @brief Shared variable handling
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

#ifndef VAR__H
#define VAR__H

#include "env.h"

#include <stdint.h>
#include <stdbool.h>

#define VAR_MAX_LOCATION_MEASUREMENTS   10

extern int8_t var_central_rssi[ENV_LOCATION_LAST];
extern int8_t var_peripheral_rssi[ENV_LOCATION_LAST];
extern float var_angle[ENV_LOCATION_LAST];
extern double var_distance[ENV_LOCATION_LAST];

typedef struct var_location_measurement {
  float distance;
  int8_t central_rssi;
  int8_t peripheral_rssi;
} var_location_measurement_t;

typedef struct var_location_record {
  var_location_measurement_t measurement[VAR_MAX_LOCATION_MEASUREMENTS];
  unsigned int measurements;
} var_location_record_t;

extern var_location_record_t var_location_record[ENV_LOCATION_LAST];

void var_init(void);
bool var_location_measurement_add(env_device_location_t location,
                                  float distance,
                                  int8_t central_rssi,
                                  int8_t peripheral_rssi);
bool var_location_measurement_remove(env_device_location_t location,
                                     float distance);
bool var_location_measurement_flush(env_device_location_t location);

#endif
