/***************************************************************************//**
 * @file
 * @brief UI functions for WSTK display
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
 ******************************************************************************/

#include <string.h>
#include <stdio.h>
#include "em_types.h"
#include "glib.h"
#include "dmd/dmd.h"
#include "sl_smartwatch_ui.h"
#include "app_assert.h"
#include "sl_memlcd_display.h"
#include "sl_sensor_rht.h"

// ----------------------------------------------------------------------------
// Configuration

#if !defined(DISPLAY_SETTINGS)
// default values
#define COL_OFFSET                  0
#define ROW_OFFSET                  1
#define FONT_TYPE                   ((GLIB_Font_t *)&GLIB_FontNormal8x8)
#define BUFFER_SIZE                 17
#endif
// ----------------------------------------------------------------------------
// Private variables

int16_t row_to_print = 0;
static GLIB_Context_t glibContext;

uint8_t max_rows_on_display;
uint8_t row_height;
uint8_t col_width;

// ----------------------------------------------------------------------------
// Private function declarations

/**************************************************************************//**
 *  Clears given row number
 *
 *  @param[in]  row The row number to be cleared
 *****************************************************************************/
static void clear_row(int16_t row)
{
  EMSTATUS status;
  GLIB_Rectangle_t rect;
  rect.xMin = 0;
  rect.yMin = ROW_OFFSET + (row * row_height);
  rect.xMax = SL_MEMLCD_DISPLAY_WIDTH - COL_OFFSET - 1;
  rect.yMax = rect.yMin + row_height - ROW_OFFSET - 1;

  status = GLIB_setClippingRegion(&glibContext,
      (const GLIB_Rectangle_t*) &rect);
  app_assert(DMD_OK == status, "Failed to set clipping region\n");
  status = GLIB_clearRegion(&glibContext);
  app_assert(DMD_OK == status, "Failed to clear clipping region\n");
  status = GLIB_resetClippingRegion(&glibContext);
  app_assert(GLIB_OK == status, "Failed to reset clipping region\n");
  status = GLIB_applyClippingRegion(&glibContext);
  app_assert(DMD_OK == status, "Failed to apply clipping region\n");

  LOG("row number %d cleared", row);
  LOGLN();
}

/**************************************************************************//**
 *  private function to print a wrapped string on the display.
 *
 *  @param[in]  str  A pointer to the character array to be printed.
 *  @param[in]  row The row number to be printed on
 *****************************************************************************/
static void write_row_wrapped(char *str, int16_t row)
{
  EMSTATUS status;
  int32_t x, y, x0, y0;
  bool opaque = false;
  uint8_t characters_printed = 0;

  x0 = COL_OFFSET;
  y0 = ROW_OFFSET + (row - 1) * row_height;
  x = x0;
  y = y0;

  row--;
  row_to_print--;

  uint8_t max_char_row = (uint8_t) ((SL_MEMLCD_DISPLAY_WIDTH - COL_OFFSET)
      / col_width);

  LOG("printing on display:\n");
  // Loops through the string and prints char for char
  for (uint32_t stringIndex = 0; stringIndex < strlen(str); stringIndex++) {
    if (str[stringIndex] == '\n') {
      characters_printed = characters_printed
          - (characters_printed % max_char_row) + max_char_row;
      continue;
    } else if (characters_printed % max_char_row == 0) {
      row++;
      row_to_print++;
      clear_row(row);
      x = x0;
      y = y + row_height;
    }

    status = GLIB_drawChar(&glibContext, str[stringIndex], x, y, opaque);
    if (status == GLIB_ERROR_INVALID_CHAR) {
      continue;
    } else {
      app_assert(status <= GLIB_ERROR_NOTHING_TO_DRAW,
          "Failed to draw character\n");
      characters_printed++;
    }
    x += col_width;
  }
  LOG("%s", str);
  LOGLN();
  row++;
  row_to_print++;
}

/**************************************************************************//**
 *  private function to print wrapped string on the display shifted on X and Y axis.
 *
 *  @param[in]   str  A pointer to the character array to be printed.
 *  @param[in]  row The row number to be printed on
 *  @param[in]   xOffset The number of pixels to shift on the X axis. Positive values prints the string shifted towards right and negative values towards left.
 *  @param[in]   yOffset The number of pixels to shift on the Y axis. Positive values prints the string shifted downwards and negative values upwards.
 *****************************************************************************/
static void write_row_with_offset(char *str, int16_t row, int16_t xOffset,
    int16_t yOffset)
{
  EMSTATUS status;
  int32_t x, y, x0, y0;
  bool opaque = false;
  uint8_t rows_to_skip = 0, characters_printed = 0;
  int16_t pixels_to_skip = 0;

  // Limiting offset values
  xOffset = xOffset % SL_MEMLCD_DISPLAY_WIDTH;
  yOffset = yOffset % SL_MEMLCD_DISPLAY_HEIGHT;

  if (yOffset < 0) {
    rows_to_skip = (uint8_t) (abs(yOffset) / row_height);
  } else if (yOffset > 0) {
    row += (uint8_t) (yOffset / row_height);
    row_to_print += (uint8_t) (yOffset / row_height);
  } else {
    rows_to_skip = 0;
  }

  pixels_to_skip = yOffset % row_height;

  x0 = COL_OFFSET + xOffset;
  y0 = ROW_OFFSET + (row - 1) * row_height;
  x = x0;
  y = y0 + pixels_to_skip;

  row--;
  row_to_print--;

  uint8_t max_char_row = (uint8_t) ((SL_MEMLCD_DISPLAY_WIDTH - COL_OFFSET)
      / col_width);

  LOG("printing on display:\n");
  // Loops through the string and prints char for char
  for (uint32_t stringIndex = 0; stringIndex < strlen(str); stringIndex++) {
    if (str[stringIndex] == '\n') {
      characters_printed = characters_printed
          - (characters_printed % max_char_row) + max_char_row;
      continue;
    } else if (characters_printed % max_char_row == 0) {
      row++;
      row_to_print++;
      if (row >= rows_to_skip) {
        x = x0;
        y = y + row_height;
      }
    }
    if (row >= rows_to_skip) {
      status = GLIB_drawChar(&glibContext, str[stringIndex], x, y, opaque);
      if (status == GLIB_ERROR_INVALID_CHAR) {
        continue;
      } else {
        app_assert(status <= GLIB_ERROR_NOTHING_TO_DRAW,
            "Failed to draw character\n");
        characters_printed++;
      }
    } else {
      characters_printed++;
    }
    x += col_width;
  }
  LOG("%s", str);
  LOGLN();
  row++;
  row_to_print++;
}

// ----------------------------------------------------------------------------
// Global functions

/**************************************************************************//**
 * Initialize the UI.
 *****************************************************************************/
void sl_smartwatch_ui_init(void)
{
  EMSTATUS status;

  // Initialize the DMD module for the DISPLAY device driver.
  status = DMD_init(0);
  app_assert(DMD_OK == status, "Failed to init display driver\n");

  status = GLIB_contextInit(&glibContext);
  app_assert(DMD_OK == status, "Failed to init display context\n");

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  status = GLIB_setFont(&glibContext, FONT_TYPE);
  app_assert(GLIB_OK == status, "Error while setting font\n");

  status = GLIB_clear(&glibContext);
  app_assert(GLIB_OK == status, "Error while clearing the display\n");

  status = sl_sensor_rht_init();
  app_assert(SL_STATUS_OK == status, "Error initializing the RHT sensor\n");

  //setting global parameters for display configuration
  max_rows_on_display = (uint8_t) (SL_MEMLCD_DISPLAY_HEIGHT
      / (glibContext.font.fontHeight + glibContext.font.lineSpacing));
  row_height = (uint8_t) (glibContext.font.fontHeight
      + glibContext.font.lineSpacing);
  col_width = (uint8_t) (glibContext.font.fontWidth
      + glibContext.font.charSpacing);

  sl_smartwatch_ui_clear_screen();
  sl_smartwatch_ui_update();

  LOG("LCD initialization successful");
  LOGLN();
}

/**************************************************************************//**
 * Updates the screen. Call this to update the display after writing to it.
 *****************************************************************************/
void sl_smartwatch_ui_update(void)
{
  EMSTATUS status;
  status = DMD_updateDisplay();
  app_assert(DMD_OK == status, "Failed to update the display\n");
}

/**************************************************************************//**
 *  Clears entire screen
 *****************************************************************************/
void sl_smartwatch_ui_clear_screen(void)
{
  EMSTATUS status;
  row_to_print = 0;              // cursor back to its initial position
  status = GLIB_clear(&glibContext);      // clear entire screen
  app_assert(DMD_OK == status, "Failed to clear the display\n");
}

/**************************************************************************//**
 *  Returns the number of rows it will take to print given number of characters
 *   based on the selected font and display size. Display should be initialized
 *   before calling this.
 *****************************************************************************/
uint8_t sl_smartwatch_ui_char_to_rows(uint8_t charCount)
{
  uint8_t max_char_row = (uint8_t) (SL_MEMLCD_DISPLAY_WIDTH / (col_width));
  uint8_t no_of_rows;
  if (charCount % max_char_row) {
    no_of_rows = (uint8_t) ((charCount / max_char_row) + 1);
  } else {
    no_of_rows = (uint8_t) (charCount / max_char_row);
  }
  return no_of_rows;
}

/**************************************************************************//**
 *  Prints a wrapped string on the display. When the maximum characters in a
 *  row are printed, the cursor moves to the next line. Multiple calls to this
 *  function will print string on next line each time.
 *****************************************************************************/
void sl_smartwatch_ui_print_text_wrapped(char *data)
{
  clear_row(row_to_print);
  write_row_wrapped(data, row_to_print);
}

/**************************************************************************//**
 *  Prints wrapped string on the display shifted on X and Y axis. A global
 *  variable in sl_smartwatch_ui.c named 'row_to_print' is updated to keep a
 *  track of last printed row.
 *****************************************************************************/
void sl_smartwatch_ui_print_text_with_offset(char *data, int16_t xOffset,
    int16_t yOffset)
{
  clear_row(row_to_print);
  write_row_with_offset(data, row_to_print, xOffset, yOffset);
}

/**************************************************************************//**
 *  Clears the screen and prints time on line zero and date on line one
 *****************************************************************************/
void sl_smartwatch_ui_print_time(char *data)
{
  EMSTATUS status;
  int32_t x, y, x0, y0;
  bool opaque = false;

  row_to_print = 0;
  clear_row(row_to_print);

  x0 = COL_OFFSET;
  y0 = ROW_OFFSET + row_to_print * row_height;
  x = x0;
  y = y0;

  LOG("printing time on display: ");
  LOGLN();
  // Loops through the string and prints character by character
  for (uint32_t stringIndex = 0; stringIndex < strlen(data); stringIndex++) {
    if (data[stringIndex] == '\n') {
      x = x0;
      y = y + row_height;
      row_to_print++;
      clear_row(row_to_print);
      continue;
    }
    status = GLIB_drawChar(&glibContext, data[stringIndex], x, y, opaque);
    app_assert(status <= GLIB_ERROR_NOTHING_TO_DRAW,
        "Failed to draw character\n");
    x += (col_width);
  }
  LOG("%s", data);
  LOGLN();
}
