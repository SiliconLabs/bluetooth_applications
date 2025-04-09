/***************************************************************************//**
 * @file ui.h
 * @brief UI module
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef UI__H
#define UI__H

#include "env.h"

#ifndef DISABLE_LCD

void ui_set_name(env_device_name_t name);
void ui_set_address(unsigned int address);
void ui_set_rssi(int rssi);
void ui_clear_rssi(void);
void ui_set_angle(float angle);
void ui_clear_angle(void);
void ui_set_distance(float distance);
void ui_clear_distance(void);
void ui_clear_location_rssi(env_device_location_t location);
void ui_set_location_rssi(env_device_location_t location, int rssi);

#else

static inline void ui_set_name(env_device_name_t name)
{
  (void)name;
}

static inline void ui_set_address(unsigned int address)
{
  (void)address;
}

static inline void ui_set_rssi(int rssi)
{
  (void)rssi;
}

static inline void ui_clear_rssi(void)
{
}

static inline void ui_set_angle(float angle)
{
  (void)angle;
}

static inline void ui_clear_angle(void)
{
}

static inline void ui_set_distance(float distance)
{
  (void)distance;
}

static inline void ui_clear_distance(void)
{
}

static inline void ui_clear_location_rssi(env_device_location_t location)
{
  (void)location;
}

static inline void ui_set_location_rssi(env_device_location_t location,
                                        int rssi)
{
  (void)location;
  (void)rssi;
}

#endif

#endif
