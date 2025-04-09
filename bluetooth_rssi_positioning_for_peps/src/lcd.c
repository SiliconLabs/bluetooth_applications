/***************************************************************************//**
 * @file lcd.c
 * @brief LCD Display functions
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

#include "lcd.h"

#include "sl_board_control.h"
#include "em_assert.h"
#include "glib.h"
#include "dmd.h"

#include "app_log.h"

#include <string.h>

#define COL_OFFSET                  2
#define ROW_OFFSET                  2
#define ROW_SPACING                 9
#define FONT_TYPE                   ((GLIB_Font_t *)&GLIB_FontNormal8x8)

static GLIB_Context_t glibContext;

sl_status_t lcd_init(void)
{
  unsigned int ret;

  /* Enable the memory lcd */
  ret = sl_board_enable_display();
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to enable the display!\n");
    return ret;
  }

  /* Initialize the DMD support for memory lcd display */
  ret = DMD_init(0);
  if (ret != DMD_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to initialize the display IO!\n");
    return SL_STATUS_IO;
  }

  /* Initialize the glib context */
  ret = GLIB_contextInit(&glibContext);
  if (ret != GLIB_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to initialize the display driver!\n");
    return SL_STATUS_IO;
  }

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  /* Fill lcd with background color */
  GLIB_clear(&glibContext);

  /* Use Narrow font */
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);

  DMD_updateDisplay();

  return SL_STATUS_OK;
}

void lcd_clear_row(unsigned int row)
{
  GLIB_Rectangle_t rect;

  rect.xMin = 0;
  rect.yMin = ROW_OFFSET + row * ROW_SPACING;
  rect.xMax = glibContext.pDisplayGeometry->xSize - 1;
  rect.yMax = rect.yMin + ROW_SPACING - 1;

  GLIB_setClippingRegion(&glibContext, (const GLIB_Rectangle_t *)&rect);
  GLIB_clearRegion(&glibContext);
  GLIB_resetClippingRegion(&glibContext);
  GLIB_applyClippingRegion(&glibContext);
}

void lcd_write_row(unsigned int row, const char *str, int len)
{
  if (len < 0) {
    len = strlen(str);
  }

  if (len) {
    unsigned int y = ROW_OFFSET + row * ROW_SPACING;

    GLIB_drawString(&glibContext, str, len, COL_OFFSET, y, false);
  }
}

void lcd_update(void)
{
  DMD_updateDisplay();
}

#endif
