/***************************************************************************//**
 * @file glib.c
 * @brief Silicon Labs Graphics Library
 *******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
*
* EVALUATION QUALITY
* This code has been minimally tested to ensure that it builds with the specified
* dependency versions and is suitable as a demonstration for evaluation purposes only.
* This code will be maintained at the sole discretion of Silicon Labs.
*
******************************************************************************/

/* Standard C header files */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "glib.h"
#include "ssd1306.h"

/** Define the default font. An application can override the default font
 *  by defining GLIB_NO_DEFAULT_FONT and by providing a custom
 *  GLIB_DEFAULT_FONT macro that points to a @ref GLIB_Font_t structure
 *  that should be used as a default font. */
#ifndef GLIB_NO_DEFAULT_FONT
#define GLIB_DEFAULT_FONT       ((glib_font_t *)&glib_font_6x8)
#endif

/* This glib_frame_buffer is large enough to store one full frame. */
static uint8_t glib_frame_buffer[(SSD1306_DISPLAY_WIDTH * SSD1306_DISPLAY_HEIGHT) / 8];

/* Pointer to memory lcd to use. */
static const ssd1306_t *oled = NULL;

/* Dimensions of the display */
static glib_display_geometry_t dimensions;

/**************************************************************************//**
 * @brief
 *   Initialization function for the glib.
 *
 * @return
 *  Returns GLIB_OK on success, or else error code
*****************************************************************************/
glib_status_t glib_init(void)
{
  sl_status_t status;

  if (oled != NULL) {
    return GLIB_OK;
  }

  /* Initialize the memory lcd. */
  status = ssd1306_init();
  if (status != SL_STATUS_OK) {
    return GLIB_ERROR_DRIVER_NOT_INITIALIZED;
  }

  /* Retrieve the memory lcd. */
  oled = ssd1306_get();
  if (oled == NULL) {
    return GLIB_ERROR_DRIVER_NOT_INITIALIZED;
  }

  /* Set up dimensions of the display */
  dimensions.xSize = oled->width;
  dimensions.ySize = oled->height;

  return GLIB_OK;
}


/**************************************************************************//**
*  @brief
*  Clears the display with the background color of the glib_context_t
*
*  @param pContext
*  Pointer to a GLIB_Context_t which holds the background color.
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_clear(glib_context_t *pContext)
{
  uint32_t i;

  /* Check arguments */
  if (pContext == NULL) {
    return GLIB_ERROR_INVALID_ARGUMENT;
  }

  /* Fill the display with the background color of the glib_context_t  */
  for (i = 0; i < sizeof(glib_frame_buffer); i++) {
      glib_frame_buffer[i] = (pContext->backgroundColor == Black) ? 0x00 : 0xFF;
  }

  return GLIB_OK;
}


/**************************************************************************//**
*  @brief
*  Draws a pixel at x, y using foregroundColor defined in the glib_context_t.
*
*  @param pContext
*  Pointer to a glib_context_t which holds the foreground color.
*  @param x
*  X-coordinate
*  @param y
*  Y-coordinate
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_draw_pixel(glib_context_t *pContext, int32_t x, int32_t y)
{
  /* Check arguments */
  if (pContext == NULL) {
    return GLIB_ERROR_INVALID_ARGUMENT;
  }

  if ((x > dimensions.xSize)
   || (y > dimensions.ySize)) {
      return GLIB_ERROR_INVALID_REGION;
  }

  if (pContext->foregroundColor == White) {
    glib_frame_buffer[x + (y / 8) * dimensions.xSize] |= 1 << (y % 8);
  }
  else {
    glib_frame_buffer[x + (y / 8) * dimensions.xSize] &= ~(1 << (y % 8));
  }

  return GLIB_OK;
}

/**************************************************************************//**
*  @brief
*  Earse a pixel at x, y using foregroundColor defined in the glib_context_t.
*
*  @param pContext
*  Pointer to a glib_context_t which holds the foreground color and clipping region
*  @param x
*  X-coordinate
*  @param y
*  Y-coordinate
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_earse_pixel(glib_context_t *pContext, int32_t x, int32_t y)
{

  /* Check arguments */
  if (pContext == NULL) {
    return GLIB_ERROR_INVALID_ARGUMENT;
  }

  if ((x > dimensions.xSize)
   || (y > dimensions.ySize)) {
      return GLIB_ERROR_INVALID_REGION;
  }

  if (pContext->foregroundColor == White) {
    glib_frame_buffer[x + (y / 8) * dimensions.xSize] &= ~(1 << (y % 8));
  }
  else {
    glib_frame_buffer[x + (y / 8) * dimensions.xSize] |= 1 << (y % 8);
  }

  return GLIB_OK;
}

/**************************************************************************//**
*  @brief
*  Draws a char using the font supplied with the library.
*
*  @param pContext
*  Pointer to the glib_context_t
*
*  @param my_char
*  Char to be drawn
*
*  @param x
*  Start x-coordinate for the char (Upper left corner)
*
*  @param y
*  Start y-coordinate for the char (Upper left corner)
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_draw_char(glib_context_t *pContext, char my_char, int32_t x, int32_t y)
{
  uint32_t i, j;
  uint16_t  pixelData;

  /* Check arguments */
  if (pContext == NULL) {
    return GLIB_ERROR_INVALID_ARGUMENT;
  }

  /* Check input char */
  if ((my_char < ' ') || (my_char > '~')) {
    return GLIB_ERROR_INVALID_CHAR;
  }

  // Use the font to write
  for (i = 0; i < pContext->font.height; i++) {
    pixelData = pContext->font.data[(my_char - 32) * pContext->font.height + i];

    for (j = 0; j < pContext->font.width; j++) {
      if ((pixelData << j) & 0x8000) {
        glib_draw_pixel(pContext, x + j, y + i);
      }
      else {
        glib_earse_pixel(pContext, x + j, y + i);
      }
    }
  }

  return GLIB_OK;
}

/**************************************************************************//**
*  @brief
*  Draws a string using the font supplied with the library.
*
*  @param pContext
*  Pointer to a glib_context_t
*
*  @param str
*  Pointer to the string that is drawn
*
*  @param x0
*  Start x-coordinate for the string (Upper left corner)
*
*  @param y0
*  Start y-coordinate for the string (Upper left corner)
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_draw_string(glib_context_t *pContext, const char* str,
                         int32_t x0, int32_t y0)
{
  glib_status_t status;
  uint32_t drawnElements = 0;
  int32_t x;

  /* Check arguments */
  if (pContext == NULL || str == NULL) {
    return GLIB_ERROR_INVALID_ARGUMENT;
  }

  x = x0;

  /* Loops through the string and prints char for char */
  while (*str) {
    status = glib_draw_char(pContext, *str, x, y0);
    if (status != GLIB_OK) {
        // Char could not be written
        return status;
    }

    if (status == GLIB_OK) {
      drawnElements++;
    }

    /* Adjust x and y coordinate */
    x += (pContext->font.width + pContext->font.spacing);

    // Next char
    str++;
  }

  return ((drawnElements == 0) ? GLIB_ERROR_NOTHING_TO_DRAW : GLIB_OK);
}

/**************************************************************************//**
*  @brief
*  Set new font for the library. Note that GLIB defines a default font in glib.c.
*  Redefine GLIB_DEFAULT_FONT to change the default font.
*
*  @param pContext
*  Pointer to the glib_context_t
*
*  @param pFont
*  Pointer to the new font
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_set_font(glib_context_t *pContext, glib_font_t *pFont)
{
  /* Check arguments */
  if (pContext == NULL) {
    return GLIB_ERROR_INVALID_ARGUMENT;
  }

  if (pFont == NULL) {
    memset(&pContext->font, 0, sizeof(glib_font_t));
    return GLIB_ERROR_INVALID_ARGUMENT;
  } 
  else {
    memcpy(&pContext->font, pFont, sizeof(glib_font_t));
    return GLIB_OK;
  }
}

/**************************************************************************//**
*  @brief
*  Draws a line from x1, y1 to x2, y2 by Bresenhem's algorithm
*
*  @param pContext
*  Pointer to a glib_context_t in which the line is drawn.
*
*  @param x1
*  Start x-coordinate
*
*  @param y1
*  Start y-coordinate
*
*  @param x2
*  End x-coordinate
*
*  @param y2
*  End y-coordinate
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_draw_line(glib_context_t *pContext, uint8_t x1, uint8_t y1,
                                                     uint8_t x2, uint8_t y2)
{
  int32_t deltaX = abs(x2 - x1);
  int32_t deltaY = abs(y2 - y1);
  int32_t signX = ((x1 < x2) ? 1 : -1);
  int32_t signY = ((y1 < y2) ? 1 : -1);
  int32_t error = deltaX - deltaY;
  int32_t error2;

  glib_draw_pixel(pContext, x2, y2);

  while ((x1 != x2) || (y1 != y2)) {
    glib_draw_pixel(pContext, x1, y1);
    error2 = error * 2;
    if (error2 > -deltaY) {
      error -= deltaY;
      x1 += signX;
    }

    if (error2 < deltaX) {
      error += deltaX;
      y1 += signY;
    }
  }
  
  return GLIB_OK;
}

/**************************************************************************//**
*  @brief
*  Draws a rectangle from x1, y1 to x2, y2
*
*  @param pContext
*  Pointer to a glib_context_t in which the line is drawn.
*
*  @param x1
*  Start x-coordinate
*
*  @param y1
*  Start y-coordinate
*
*  @param x2
*  End x-coordinate
*
*  @param y2
*  End y-coordinate
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_draw_rectangle(glib_context_t *pContext, uint8_t x1, uint8_t y1,
                                                          uint8_t x2, uint8_t y2)
{
  glib_draw_line(pContext, x1,y1,x2,y1);
  glib_draw_line(pContext, x2,y1,x2,y2);
  glib_draw_line(pContext, x2,y2,x1,y2);
  glib_draw_line(pContext, x1,y2,x1,y1);

  return GLIB_OK;
}

/**************************************************************************//**
*  @brief
*  Draws a circle with center at x, y, and a radius
*
*  Draws a circle using the Bresenhem's algorithm. See Wikipedia for algorithm.
*
*  @param pContext
*  Pointer to a glib_context_t in which the circle is drawn. The circle is drawn using
*  the foreground color.
*
*  @param par_x
*  Center x-coordinate
*
*  @param par_y
*  Center y-coordinate
*
*  @param par_r
*  Radius of the circle
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_draw_circle(glib_context_t *pContext, uint8_t par_x, uint8_t par_y,
                                                uint8_t par_r)
{
  int32_t x = -par_r;
  int32_t y = 0;
  int32_t err = 2 - 2 * par_r;
  int32_t e2;

  if (par_x >= dimensions.xSize || par_y >= dimensions.ySize) {
    return GLIB_ERROR_INVALID_REGION;
  }

  do {
    glib_draw_pixel(pContext, par_x - x, par_y + y);
    glib_draw_pixel(pContext, par_x + x, par_y + y);
    glib_draw_pixel(pContext, par_x + x, par_y - y);
    glib_draw_pixel(pContext, par_x - x, par_y - y);
    e2 = err;
    if (e2 <= y) {
      y++;
      err = err + (y * 2 + 1);
      if(-x == y && e2 <= x) {
        e2 = 0;
      }
    }
    
    if(e2 > x) {
      x++;
      err = err + (x * 2 + 1);
    }
  } while(x <= 0);

  return GLIB_OK;
}

/**************************************************************************//**
*  @brief
*  Update the display device with contents of active glib_frame_buffer.
*
*  @return
*  Returns GLIB_OK is successful, error otherwise.
******************************************************************************/
glib_status_t glib_update_display(void)
{
  return ((ssd1306_draw(glib_frame_buffer) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_OUT_OF_MEMORY);
}

/**************************************************************************//**
*  @brief
*  Draws a bitmap
*
*  Sets up a bitmap that starts at 0,0 and draws bitmap.
*  The picture is a monochrome bitmap.
*
*  @param pContext
*  Pointer to a GLIB_Context_t in which the bitmap is drawn.

*  @param data
*  Bitmap data
*
*  @return
*  Returns GLIB_OK on success, or else error code
******************************************************************************/
glib_status_t glib_draw_bmp(glib_context_t *pContext, const uint8_t *data)
{
  (void) pContext;

  return ((ssd1306_draw(data) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_OUT_OF_MEMORY);
}

/**************************************************************************//**
 * @brief
 *   Set a inversion color to glib.
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_set_invert_color(void)
{
  return ((ssd1306_set_invert_color() == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Set a normal color to glib.
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_set_normal_color(void)
{
  return ((ssd1306_set_normal_color() == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Set a contrast to glib.
 *
 * @param[in] value
 *   value to set contrast. Select 1 out of 256 contrast steps.
 *   Contrast increases as the value increases
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_set_contrast(uint8_t value)
{
  return ((ssd1306_set_contrast(value) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Set a Right Horizontal Scroll to glib.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_scroll_right(uint8_t start_page_addr, uint8_t end_page_addr)
{
  return ((ssd1306_scroll_right(start_page_addr, end_page_addr) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Set a Left Horizontal Scroll to glib.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_scroll_left(uint8_t start_page_addr, uint8_t end_page_addr)
{
  return ((ssd1306_scroll_left(start_page_addr, end_page_addr) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Set a Vertical and Right Horizontal Scroll to glib.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_scroll_diag_right(uint8_t start_page_addr, uint8_t end_page_addr)
{
  return ((ssd1306_scroll_diag_right(start_page_addr, end_page_addr) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Set a Vertical and Left Horizontal Scroll to glib.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_scroll_diag_left(uint8_t start_page_addr, uint8_t end_page_addr)
{
  return ((ssd1306_scroll_diag_left(start_page_addr, end_page_addr) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Stop scroll to glib.
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_stop_scroll(void)
{
  return ((ssd1306_stop_scroll() == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}

/**************************************************************************//**
 * @brief
 *   Set the display ON/OFF to glib.
 *
 * @param[in] on
 *   State of OLED
 *
 * @return
 *   GLIB_OK if there are no errors.
 *****************************************************************************/
glib_status_t glib_enable_display(bool on)
{
  return ((ssd1306_enable_display(on) == SL_STATUS_OK) ? GLIB_OK : GLIB_ERROR_IO);
}
