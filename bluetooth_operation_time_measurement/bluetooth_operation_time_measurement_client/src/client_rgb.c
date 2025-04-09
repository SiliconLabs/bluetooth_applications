/***************************************************************************//**
 * @file client_rgb.c
 * @brief client rgb adafruit source file.
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
#include <client_rgb.h>
#include "app_assert.h"
#include "adafruit_is31fl3741.h"
#include "glib.h"
#include "glib_font.h"

/***************************************************************************//**
 * led matrix initialize.
 ******************************************************************************/
sl_status_t client_rgb_init(glib_context_t *rgb_context, sl_i2cspm_t *i2cspm)
{
  sl_status_t ret;

  ret = adafruit_is31fl3741_init(i2cspm);
  app_assert_status(ret);

  client_rgb_context_init(rgb_context);
  client_rgb_context_clear(rgb_context);
  return ret;
}

/***************************************************************************//**
 * glib object initialize.
 ******************************************************************************/
void client_rgb_context_init(glib_context_t *rgb_context)
{
  glib_init(rgb_context);
  glib_set_contrast(0xff);
  glib_set_bg_color(rgb_context, 0x0000);
  glib_enable_wrap(rgb_context, false);
  glib_enable_display(true);
  // 0xf800, 0xffe0, 0x07e0, 0x07ff, 0x001f, 0xf81f, 0xffff, 0x73c0
  glib_set_text_color(rgb_context, 0x07e0);
// glib_set_font(rgb_context, &glib_font_picopixel);
}

/***************************************************************************//**
 * display content.
 ******************************************************************************/
void client_rgb_display(glib_context_t *rgb_context, const char *content)
{
  glib_fill(rgb_context, rgb_context->bg_color);
  glib_draw_string(rgb_context, content, 1, 1);
  glib_update_display();
}

/***************************************************************************//**
 * clear content.
 ******************************************************************************/
void client_rgb_context_clear(glib_context_t *rgb_context)
{
  glib_clear(rgb_context);
}
