/***************************************************************************//**
 * @file app_display.c
 * @brief application display code
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

#include <string.h>
#include "sl_i2cspm_instances.h"
#include "glib.h"
#include "micro_oled_ssd1306.h"
#include "sl_bluetooth.h"
#include "stdio.h"
#include "glib_font.h"

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

static glib_context_t glib_context;

/***************************************************************************//**
 * App Display Update Function.
 ******************************************************************************/
void app_display_update(const char *string_midle, const char *string_lower)
{
  uint8_t x_offset;

  glib_clear(&glib_context);
  glib_status_t status;
  glib_set_font(&glib_context, NULL);
  glib_draw_string(&glib_context, " DOOR LOCK", 0, 2);
  glib_draw_line(&glib_context, 0, 11, 63, 11, GLIB_WHITE);
  glib_draw_line(&glib_context, 0, 35, 63, 35, GLIB_WHITE);
  glib_draw_string(&glib_context, string_midle, 8, 20);
  glib_set_font(&glib_context, (glib_gfx_font_t *) &glib_font_picopixel);
  x_offset = (64 - strlen(string_lower) * 4) / 2;
  glib_draw_string(&glib_context, string_lower, x_offset, 44);
  status = glib_update_display();

  if (GLIB_OK != status) {
    return;
  }
}

/***************************************************************************//**
 * App Display Show Card Function.
 ******************************************************************************/
void app_display_show_card(uint8_t location, uint8_t *id_tag)
{
  static char temp[13];
  static char temp_2[13];

  snprintf(temp, sizeof(temp), "%02X%02X%02X%02X%02X%02X",
           id_tag[0],
           id_tag[1],
           id_tag[2],
           id_tag[3],
           id_tag[4],
           id_tag[5]);
  snprintf(temp_2, sizeof(temp_2), "   %d.", location);
  app_display_update(temp_2, temp);
}

/***************************************************************************//**
 * App Display Init Function.
 ******************************************************************************/
void app_display_init(void)
{
  /* Initialize the display */
  ssd1306_init(sl_i2cspm_qwiic);
  glib_init(&glib_context);

  /* Fill lcd with background color */
  glib_clear(&glib_context);

  glib_draw_xbitmap(&glib_context,
                    0, 10, silicon_labs_logo_64x23,
                    64, 23, GLIB_WHITE);
  glib_update_display();
  sl_sleeptimer_delay_millisecond(1000);

/* Fill lcd with background color */
  glib_clear(&glib_context);

  /* Use Narrow font */
  glib_draw_string(&glib_context, " DOOR LOCK", 0, 2);
  glib_draw_line(&glib_context, 0, 11, 63, 11, GLIB_WHITE);
  glib_draw_line(&glib_context, 0, 35, 63, 35, GLIB_WHITE);
  glib_update_display();
  app_display_update(" LOCKED", "DISCONNECTED");
}
