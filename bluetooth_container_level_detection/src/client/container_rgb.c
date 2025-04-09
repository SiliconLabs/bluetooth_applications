/***************************************************************************//**
 * @file container_rgb.c
 * @brief container rgb adafruit source file.
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
#include "container_rgb.h"
#include "app_assert.h"
#include "adafruit_is31fl3741.h"
#include "glib.h"
#include "glib_font.h"

// default level definitions
#define MAX_HEIGHT_LEVEL_CONTAINER        9
#define EMPTY_LEVEL_CONTAINER             0

// start point definition for slots
#define DEFAULT_X_START_SLOT_0            10
#define DEFAULT_X_START_SLOT_1            7
#define DEFAULT_X_START_SLOT_2            4
#define DEFAULT_X_START_SLOT_3            1
#define DEFAULT_WIDTH_SLOT                2

// struct hold container's information
typedef struct container_t
{
  uint8_t x_Start;
  int level;
}container_t;

// container variable
static container_t container[4];
// glib object show container's information
static glib_context_t glib_context;

/***************************************************************************//**
 * led matrix initialize.
 ******************************************************************************/
sl_status_t container_rgb_init(sl_i2cspm_t *i2cspm)
{
  sl_status_t ret;

  ret = adafruit_is31fl3741_init(i2cspm);
  app_assert_status(ret);

  container[0].x_Start = DEFAULT_X_START_SLOT_0;
  container[1].x_Start = DEFAULT_X_START_SLOT_1;
  container[2].x_Start = DEFAULT_X_START_SLOT_2;
  container[3].x_Start = DEFAULT_X_START_SLOT_3;

  container_rgb_context_init(&glib_context);
  container_rgb_context_clear(&glib_context);
  return ret;
}

/***************************************************************************//**
 * update container's level.
 ******************************************************************************/
sl_status_t container_rgb_update_level(uint8_t slot, int level_value)
{
  glib_clear(&glib_context);
  container[slot].level = level_value;
  for (int slot = 0; slot < 4; slot++) {
    switch (container[slot].level)
    {
      case MAX_HEIGHT_LEVEL_CONTAINER:
        glib_draw_rect(&glib_context,
                       container[slot].x_Start,
                       0,
                       DEFAULT_WIDTH_SLOT,
                       9,
                       0xf800);
        break;
      case 1:

        glib_draw_rect(&glib_context,
                       container[slot].x_Start,
                       0,
                       DEFAULT_WIDTH_SLOT,
                       1,
                       0x07ff);
        break;
      case 0:
        break;
      default:
        glib_draw_rect(&glib_context,
                       container[slot].x_Start,
                       0,
                       DEFAULT_WIDTH_SLOT,
                       container[slot].level,
                       0x07e0);
        break;
    }
  }

  glib_update_display();
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * glib object initialize.
 ******************************************************************************/
void container_rgb_context_init(glib_context_t *g_context)
{
  glib_init(g_context);
  glib_set_rotation(g_context, 2);
  glib_set_contrast(0x8f);
  glib_set_bg_color(g_context, 0x0000);
  glib_enable_wrap(g_context, false);
  glib_enable_display(true);
  glib_set_text_color(g_context, 0xf800);
  glib_set_font(g_context, &glib_font_picopixel);
}

/***************************************************************************//**
 * display content.
 ******************************************************************************/
void container_rgb_display(glib_context_t *g_context, uint8_t *content)
{
  glib_write_char(g_context, *content);
}

/***************************************************************************//**
 * clear content.
 ******************************************************************************/
void container_rgb_context_clear(glib_context_t *g_context)
{
  glib_clear(g_context);
}
