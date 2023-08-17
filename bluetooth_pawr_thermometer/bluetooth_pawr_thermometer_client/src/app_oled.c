/***************************************************************************//**
 * @file app_oled.c
 * @brief client OLED SparkFun source file.
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
#include "app_oled.h"

#include "micro_oled_ssd1306.h"
#include "sl_i2cspm_instances.h"
#include "sl_sleeptimer.h"
#include "glib.h"

static glib_context_t glib_context;

/***************************************************************************//**
 * oled and glib initialize.
 ******************************************************************************/
void app_oled_init(void)
{
  ssd1306_init(sl_i2cspm_qwiic);

  /* Initialize the display */
  glib_init(&glib_context);

  glib_context.bg_color = GLIB_BLACK;
  glib_context.text_color = GLIB_WHITE;

  /* Use Narrow font */
  glib_set_font(&glib_context, NULL);

  /* Fill lcd with background color */
  glib_clear(&glib_context);

  /* Show Silabs logo for 3 seconds */
  glib_draw_xbitmap(&glib_context,
                    0, 14, silicon_labs_logo_64x23,
                    64, 23, GLIB_WHITE);
  glib_update_display();

  sl_sleeptimer_delay_millisecond(3000);
}

/***************************************************************************//**
 * screen display no data.
 ******************************************************************************/
void app_oled_display_no_data(void)
{
  glib_clear(&glib_context);

  // draw header
  glib_draw_string(&glib_context, "NO DEVICE", 7, 0);
  glib_draw_line(&glib_context, 0, 11, 63, 11, GLIB_WHITE);

  // draw middle
  glib_draw_string(&glib_context, "---", 24, 20);

  // draw footer
  glib_draw_string(&glib_context, "NO DATA", 12, 40);
  glib_draw_line(&glib_context, 0, 35, 63, 35, GLIB_WHITE);

  glib_update_display();
}

/***************************************************************************//**
 * screen display no data.
 ******************************************************************************/
void app_oled_display_data(uint8_t sync,
                           uint8_t temp,
                           uint8_t sample_count)
{
  char tmp_str[11];

  glib_clear(&glib_context);

  // draw header
  snprintf(tmp_str, sizeof(tmp_str), "SYNC[#%d]", sync);
  glib_draw_string(&glib_context, tmp_str, 2, 0);
  glib_draw_line(&glib_context, 0, 11, 63, 11, GLIB_WHITE);

  // display temperature value in the middle
  snprintf(tmp_str, sizeof(tmp_str), "%d  C", temp);
  glib_draw_string(&glib_context, tmp_str, 16, 22);
  glib_draw_string(&glib_context, ".", 33, 18);

  // draw footer
  glib_draw_line(&glib_context, 0, 35, 63, 35, GLIB_WHITE);
  snprintf(tmp_str, sizeof(tmp_str), "SAMPLE#%d", sample_count);
  glib_draw_string(&glib_context, tmp_str, 2, 40);

  glib_update_display();
}
