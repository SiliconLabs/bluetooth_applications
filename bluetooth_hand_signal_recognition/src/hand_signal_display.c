/***************************************************************************//**
 * @file hand_signal_display.c
 * @brief hand signal display file.
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
 * 1. The origin of this software must not be misrepresented{} you must not
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

#include "sl_sleeptimer.h"
#include "sl_i2cspm_instances.h"

#include "app_log.h"

#include "micro_oled_ssd1306.h"
#include "glib.h"
#include "glib_font.h"
#include "hand_signal_display.h"

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
 * Application Display Initialize.
 ******************************************************************************/
void hand_signal_oled_init(void)
{
  if (ssd1306_init(sl_i2cspm_inst0) == SL_STATUS_OK) {
    app_log("Sparkfun Oled Init Done\r\n");
  } else {
    app_log("Sparkfun Oled Init Failed\r\n");
  }
  // Initialize the display
  glib_init(&glib_context);

  glib_context.bg_color = GLIB_BLACK;
  glib_context.text_color = GLIB_WHITE;

  // Fill lcd with background color
  glib_clear(&glib_context);
  glib_draw_xbitmap(&glib_context,
                    0, 10, silicon_labs_logo_64x23,
                    64, 23, GLIB_WHITE);
  glib_update_display();
  sl_sleeptimer_delay_millisecond(2000);

  // Fill lcd with background color
  glib_clear(&glib_context);
}

/***************************************************************************//**
 * Show error when hand_signal_recognition_init error
 ******************************************************************************/
void hand_signal_display_error(void)
{
  glib_clear(&glib_context);
  glib_set_font(&glib_context, NULL);
  glib_draw_string(&glib_context, "HAND", 0, 0);
  glib_draw_string(&glib_context, "SIGNAL", 27, 0);
  glib_draw_line(&glib_context, 0, 10, 63, 10, GLIB_WHITE);
  glib_draw_string(&glib_context, "Initialize", 0, 14);
  glib_draw_string(&glib_context, "Error", 0, 26);
  glib_update_display();
}

/***************************************************************************//**
 * Application Hand Signal Display.
 * +++++++++++++++
 * + HAND SIGNAL +
 * +-------------+
 * +    ---      +
 * +-------------+
 * + SCORE: ---  +
 * +++++++++++++++
 ******************************************************************************/
void hand_signal_display_init(void)
{
  glib_clear(&glib_context);
  glib_draw_string(&glib_context, "HAND", 0, 0);
  glib_draw_string(&glib_context, "SIGNAL", 27, 0);
  glib_draw_line(&glib_context, 0, 10, 63, 10, GLIB_WHITE);
  glib_draw_string(&glib_context, "---", 23, 20);
  glib_draw_line(&glib_context, 0, 36, 63, 36, GLIB_WHITE);
  glib_draw_string(&glib_context, "SCORE:", 0, 39);
  glib_draw_string(&glib_context, "---", 38, 39);
  glib_update_display();
}

void hand_signal_display_update(uint8_t result, uint8_t score)
{
  uint8_t score_text_buffer[10];
  char *signal_text[] = { "  NOTHING ", "    UP    ", "   DOWN   " };

  snprintf((char *) score_text_buffer, sizeof(score_text_buffer), "%3d", score);
  glib_draw_string(&glib_context, signal_text[result], 0, 20);
  glib_draw_string(&glib_context, (char *) score_text_buffer, 38, 39);
  glib_update_display();
}
