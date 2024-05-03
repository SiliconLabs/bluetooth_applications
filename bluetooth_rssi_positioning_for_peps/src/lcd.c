/***************************************************************************//**
 * @file lcd.c
 * @brief LCD Display functions
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
