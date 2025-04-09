/***************************************************************************//**
 * @file ui.c
 * @brief UI module
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

#ifndef DISABLE_LCD

#include "ui.h"

#include "lcd.h"

#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE                 16

#define ROW_NAME                    0
#define ROW_ADDRESS                 1
#define ROW_RSSI                    2
#define ROW_ANGLE                   3
#define ROW_DISTANCE                4

#define ROW_RSSI_LOC                6

#define FMT_NAME                    "Name : %-.*s", sizeof(env_device_name)
#define FMT_ADDRESS                 "Addr : 0x%02x"
#define FMT_RSSI                    "RSSI : %+3d dBm"
#define FMT_RSSI_NONE               "RSSI : ------"
#define FMT_ANGLE                   "Angle: %+.3f deg"
#define FMT_DISTANCE                "Dist : %.3f"

#define FMT_RSSI_LOC                "%-.3s RSSI: %+3d"
#define FMT_RSSI_LOC_NONE           "%-.3s RSSI: ------"

static const char rssi_location[ENV_LOCATION_LAST][3] = {
#if !defined(ENV_LOCATION_LITE)
  "Fr ",
#endif
  "FrR",
#if !defined(ENV_LOCATION_LITE)
  "CeR",
#endif
  "ReR",
#if !defined(ENV_LOCATION_LITE)
  "Re ",
#endif
  "ReL",
#if !defined(ENV_LOCATION_LITE)
  "CeL",
#endif
  "FrL",
  "Ce ",
};

void ui_set_location_rssi(env_device_location_t location, int rssi)
{
  int len;
  char buffer[BUFFER_SIZE];

  if (location >= ENV_LOCATION_CENTER) {
    return;
  }

  len = snprintf(buffer,
                 BUFFER_SIZE,
                 (rssi == -128) ? FMT_RSSI_LOC_NONE : FMT_RSSI_LOC,
                 rssi_location[location],
                 rssi);
  if (len > BUFFER_SIZE) {
    len = BUFFER_SIZE;
  }

  lcd_clear_row(ROW_RSSI_LOC + location);
  lcd_write_row(ROW_RSSI_LOC + location, buffer, len);
}

void ui_clear_location_rssi(env_device_location_t location)
{
  if (location >= ENV_LOCATION_CENTER) {
    return;
  }

  lcd_clear_row(ROW_RSSI_LOC + location);
}

void ui_set_rssi(int rssi)
{
  int len;
  char buffer[BUFFER_SIZE];

  len = snprintf(buffer,
                 BUFFER_SIZE,
                 (rssi == -128) ? FMT_RSSI_NONE : FMT_RSSI,
                 rssi);
  if (len > BUFFER_SIZE) {
    len = BUFFER_SIZE;
  }

  lcd_clear_row(ROW_RSSI);
  lcd_write_row(ROW_RSSI, buffer, len);
}

void ui_clear_rssi(void)
{
  lcd_clear_row(ROW_RSSI);
}

void ui_set_name(env_device_name_t name)
{
  int len;
  char buffer[BUFFER_SIZE];

  len = snprintf(buffer, BUFFER_SIZE, FMT_NAME, name);
  if (len > BUFFER_SIZE) {
    len = BUFFER_SIZE;
  }

  lcd_clear_row(ROW_NAME);
  lcd_write_row(ROW_NAME, buffer, len);
}

void ui_set_address(unsigned int address)
{
  int len;
  char buffer[BUFFER_SIZE];

  len = snprintf(buffer, BUFFER_SIZE, FMT_ADDRESS, address);
  if (len > BUFFER_SIZE) {
    len = BUFFER_SIZE;
  }

  lcd_clear_row(ROW_ADDRESS);
  lcd_write_row(ROW_ADDRESS, buffer, len);
}

void ui_set_angle(float angle)
{
  int len;
  char buffer[BUFFER_SIZE];

  len = snprintf(buffer, BUFFER_SIZE, FMT_ANGLE, angle);
  if (len > BUFFER_SIZE) {
    len = BUFFER_SIZE;
  }

  lcd_clear_row(ROW_ANGLE);
  lcd_write_row(ROW_ANGLE, buffer, len);
}

void ui_clear_angle(void)
{
  lcd_clear_row(ROW_ANGLE);
}

void ui_set_distance(float distance)
{
  int len;
  char buffer[BUFFER_SIZE];

  len = snprintf(buffer, BUFFER_SIZE, FMT_DISTANCE, distance);
  if (len > BUFFER_SIZE) {
    len = BUFFER_SIZE;
  }

  lcd_clear_row(ROW_DISTANCE);
  lcd_write_row(ROW_DISTANCE, buffer, len);
}

void ui_clear_distance(void)
{
  lcd_clear_row(ROW_DISTANCE);
}

#endif
