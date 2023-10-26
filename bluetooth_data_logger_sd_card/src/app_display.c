/***************************************************************************//**
 * @file app_display.c
 * @brief Application display module
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
#include "app_log.h"
#include "app_assert.h"
#include "glib.h"
#include "app_display.h"
#include "sl_i2cspm_instances.h"
#include "micro_oled_ssd1306.h"

#define LINE_MAX_CHAR (10)

static glib_context_t glib_context;

static const unsigned char silicon_labs_logo_64x23[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x80, 0xff, 0x1f, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xf0,
  0xff, 0x7f, 0x00, 0x3e, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x7f, 0x00, 0x7f,
  0x00, 0x00, 0x00, 0xff, 0xff, 0x3f, 0x80, 0x7f, 0x00, 0x06, 0x80, 0xff,
  0xff, 0xff, 0xe0, 0xff, 0x80, 0x03, 0xc0, 0xff, 0x01, 0xf8, 0xff, 0x7f,
  0xe0, 0x01, 0xc0, 0x03, 0x00, 0x00, 0xfe, 0x7f, 0xf0, 0x01, 0x00, 0x01,
  0x00, 0x00, 0xf8, 0x3f, 0xf8, 0x03, 0x00, 0xf0, 0x07, 0x00, 0xe0, 0x3f,
  0xfc, 0x03, 0x00, 0x00, 0x20, 0x00, 0xe0, 0x1f, 0xfe, 0x0f, 0x00, 0x00,
  0xc0, 0x00, 0xe0, 0x07, 0xfe, 0xff, 0x00, 0x00, 0xfc, 0x00, 0xe0, 0x03,
  0xff, 0xc3, 0xff, 0xff, 0xff, 0x00, 0xf0, 0x00, 0x7e, 0x00, 0xfe, 0xff,
  0x7f, 0x00, 0x38, 0x00, 0x3e, 0x00, 0xff, 0xff, 0x3f, 0x00, 0x0c, 0x00,
  0x1c, 0x80, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x18, 0x00, 0xff, 0xff,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

/***************************************************************************//**
 * Initialize the AIR QUALITY application.
 ******************************************************************************/
void app_display_init(void)
{
  /* Initialize the OLED display */
  ssd1306_init(sl_i2cspm_qwiic);
  glib_init(&glib_context);

  /* Fill lcd with background color */
  glib_clear(&glib_context);

  /* Show the Silabs'Logo */
  glib_draw_xbitmap(&glib_context,
                    0, 0, silicon_labs_logo_64x23,
                    64, 23, GLIB_WHITE);
  glib_update_display();
  sl_sleeptimer_delay_millisecond(1000);
  glib_clear(&glib_context);

  glib_draw_string(&glib_context, "DATA", 0, 0);
  glib_draw_string(&glib_context, "LOGGER", 28, 0);

  glib_draw_line(&glib_context, 0, 11, 63, 11, GLIB_WHITE);
  glib_draw_line(&glib_context, 0, 47, 63, 47, GLIB_WHITE);
  glib_update_display();
}

static inline void oled_line_printf(int32_t y0, const char *fmt, ...)
{
  char number_str[LINE_MAX_CHAR + 1];
  char tmp[LINE_MAX_CHAR + 3];
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprintf(number_str, sizeof(number_str), fmt, args);
  va_end(args);

  if (n < LINE_MAX_CHAR) {
    const char blank[] = "            ";
    int n_blank = (LINE_MAX_CHAR - n) / 2;
    snprintf(tmp, sizeof(tmp), "%.*s%s%.*s",
             n_blank,
             blank,
             number_str,
             LINE_MAX_CHAR - n - n_blank,
             blank);
    glib_draw_string(&glib_context, tmp, 0, y0);
  } else {
    glib_draw_string(&glib_context, number_str, 0, y0);
  }
}

/***************************************************************************//**
 * Display sensor data.
 ******************************************************************************/
void app_display_show_sensor_rht_data(uint16_t rh,
                                      uint8_t rh_decimal[static 2],
                                      int16_t t,
                                      uint8_t t_decimal[static 2])
{
  // To keep the number in the middle of the screen
  glib_fill_rect(&glib_context, 0, 20, 40, 8, GLIB_BLACK);
  glib_fill_rect(&glib_context, 0, 32, 40, 8, GLIB_BLACK);
  oled_line_printf(20, "%d.%d%d \xf8" "C",
                   t,
                   t_decimal[0],
                   t_decimal[1]);
  oled_line_printf(32, "%d.%d%d %% ",
                   rh,
                   rh_decimal[0],
                   rh_decimal[1]);
  glib_update_display();
}
