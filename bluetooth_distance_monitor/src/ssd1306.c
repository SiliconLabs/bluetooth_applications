/***************************************************************************//**
 * @file ssd1306.c
 * @brief SSD1306 interface
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
#include <ssd1306_i2c.h>
#include <string.h>
#include "ssd1306.h"
#include "ssd1306_config.h"

/** ssd1306 instance. */
static ssd1306_t ssd1306_instance = {
  .width = SSD1306_DISPLAY_WIDTH,
  .height = SSD1306_DISPLAY_HEIGHT,
};

/** buffer used to initialize ssd1306. */
const uint8_t cmd_buff[] = {
      SSD1306_DISPLAYOFF, /* 0xAE Set OLED Display Off */

      SSD1306_SETDISPLAYCLOCKDIV, /* 0xD5 Set Display Clock Divide Ratio/Oscillator Frequency */
      0x80, /* the suggested ratio 0x80 */

      SSD1306_SETMULTIPLEX, /* 0xA8 Set Multiplex Ratio */
      0x2F, /* = oled_height/8 - 1 */

      SSD1306_SETDISPLAYOFFSET, /* 0xD3 Set Display Offset */
      0x00,

      SSD1306_SETSTARTLINE, /* 0x40 set start line address - CHECK */

      SSD1306_CHARGEPUMP, /* 0x8D Set Charge Pump */
      0x14, /* 0x14 Enable Charge Pump */

      SSD1306_NORMALDISPLAY, /* 0xA6 set normal color
                                0xA7 set inverse color */

      SSD1306_DISPLAYALLON_RESUME,  /* 0xA4 Output follows RAM content;
                                       0xA5 Output ignores RAM content */

      SSD1306_SETSEGMENTREMAP, /* 0xA1 set segment re-map 0 to 127
                                  0xA0 Mirror horizontally */

      SSD1306_COMSCANDEC, /* 0xC8 Set COM Output Scan Direction
                             0xC0 Mirror vertically */

      SSD1306_SETCOMPINS, /* 0xDA Set COM Pins Hardware Configuration */
      0x12,

      SSD1306_SETCONTRAST, /* 0x81 Set Contrast Control */
      0x8F,

      SSD1306_SETPRECHARGE, /* 0xD9 Set Pre-Charge Period */
      0xF1,

      SSD1306_SETVCOMDETECT, /* 0xDB Set VCOMH Deselect Level */
      0x40,

      SSD1306_DEACTIVATE_SCROLL, /* Stop scroll */

      SSD1306_DISPLAYON    /*  0xAF Set OLED Display On */
};

/** Flag to monitor is this driver has been initialized. The ssd1306_instance
 *  is only valid after initialized=true. */
static bool initialized = false;


/**************************************************************************//**
 * @brief
 *   Initialization function for the ssd1306 device driver.
 *
 * @return
 *   If all operations completed sucessfully SL_STATUS_OK is returned. On
 *   failure a different status code is returned specifying the error.
*****************************************************************************/
sl_status_t ssd1306_init(void)
{
  sl_status_t sc;
  const uint8_t* ptr = cmd_buff;

  ssd1306_i2c_init();
  initialized = true;
  sc =ssd1306_send_command(ptr, sizeof(cmd_buff));

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Draw total of rows to SSD1306.
 *
 * @param[in] data
 *   Pointer to the pixel matrix buffer to draw. The format of the buffer
 *   depends on the color mode of SSD1306.
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_draw(const void *data)
{
  sl_status_t sc = SL_STATUS_OK;
  unsigned int i;
  const uint8_t *ptr = data;
  uint8_t cmd;
  uint8_t no_pages = SSD1306_DISPLAY_HEIGHT / 8;

  /* Get start address to draw from */
  for ( i = 0; i < no_pages; i++ ) {
  /* Send update command and first line address */
    cmd = 0xB0 + i;  /* Set the current RAM page address. */
    sc += ssd1306_send_command(&cmd, 1);
    cmd = 0x00;  /* Set Lower Column Start Address for Page Addressing Mode */
    sc += ssd1306_send_command(&cmd, 1);
    cmd = 0x12;  /* Set Higher Column Start Address for Page Addressing Mode */
    sc += ssd1306_send_command(&cmd, 1);

  /* Send pixels for this page */
    sc += ssd1306_send_data(ptr, SSD1306_DISPLAY_WIDTH);
    ptr += SSD1306_DISPLAY_WIDTH;
  }
  if (sc != SL_STATUS_OK) {
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief
 *   Get a handle to SSD1306.
 *
 * @return
 *   Pointer to a SSD1306 structure or NULL if no SSD1306 is initialized
 *   yet.
 *****************************************************************************/
const ssd1306_t *ssd1306_get(void)
{
  if (initialized) {
    return &ssd1306_instance;
  }
  else {
    return NULL;
  }
}

/**************************************************************************//**
 * @brief
 *   Set a inversion color to SSD1306.
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_set_invert_color(void)
{
  sl_status_t sc;
  uint8_t cmd;

  cmd = SSD1306_INVERTDISPLAY;  //  0xA7 - set inverse color
  sc = ssd1306_send_command(&cmd, 1);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Set a normal color to SSD1306.
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_set_normal_color(void)
{
  sl_status_t sc;
  uint8_t cmd;

  cmd = SSD1306_NORMALDISPLAY;  //  0xA6 - set normal color
  sc = ssd1306_send_command(&cmd, 1);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Set a contrast to SSD1306.
 *
 * @param[in] value
 *   value to set contrast. Select 1 out of 256 contrast steps.
 *   Contrast increases as the value increases
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_set_contrast(uint8_t value)
{
  sl_status_t sc;
  uint8_t cmd_buff[2] = {
      SSD1306_SETCONTRAST,    //  0x81 Set Contrast Control
      value                   //  Contrast Step 0 to 255
  };

  sc = ssd1306_send_command(cmd_buff, 2);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Set a Right Horizontal Scroll to SSD1306.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_scroll_right(uint8_t start_page_addr, uint8_t end_page_addr)
{
  sl_status_t sc;
  uint8_t cmd_buff[8] = {
      SSD1306_RIGHT_HORIZONTAL_SCROLL, //   0x26 Right Horizontal scroll
      0x00,                            //  Dummy byte
      start_page_addr,                 //  Set start page address
      0x00,                            //  Set time interval between each scroll
      end_page_addr,                   //  Set end page address
      0x00,                            //  Dummy byte
      0xFF,                            //  Dummy byte
      SSD1306_ACTIVATE_SCROLL         // 0x2F Activate scroll
  };

  sc = ssd1306_send_command(cmd_buff, 8);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Set a Left Horizontal Scroll to SSD1306.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_scroll_left(uint8_t start_page_addr, uint8_t end_page_addr)
{
  sl_status_t sc;
  uint8_t cmd_buff[8] = {
      SSD1306_LEFT_HORIZONTAL_SCROLL, //   0x27 Left Horizontal scroll
      0x00,                            //  Dummy byte
      start_page_addr,                 //  Set start page address
      0x00,                            //  Set time interval between each scroll
      end_page_addr,                   //  Set end page address
      0x00,                            //  Dummy byte
      0xFF,                            //  Dummy byte
      SSD1306_ACTIVATE_SCROLL         // 0x2F Activate scroll
  };

  sc = ssd1306_send_command(cmd_buff, 8);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Set a Vertical and Right Horizontal Scroll to SSD1306.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_scroll_diag_right(uint8_t start_page_addr, uint8_t end_page_addr)
{
  sl_status_t sc;
  uint8_t cmd_buff[10] = {
      SSD1306_SET_VERTICAL_SCROLL_AREA, //   0xA3 Right Horizontal scroll
      0x00,                             //  Set No. of rows in top fixed area
      SSD1306_DISPLAY_HEIGHT,           //  Set No. of rows in scroll area
      SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL, //  0x29 Vertical and Right Horizontal Scroll
      0x00,                             //  Dummy byte
      start_page_addr,                  //  Set start page address
      0x00,                             //  Set time interval between each scroll
      end_page_addr,                    //  Set end page address
      0x01,                             //  Vertical scrolling offset
      SSD1306_ACTIVATE_SCROLL           // 0x2F Activate scroll
  };

  sc = ssd1306_send_command(cmd_buff, 10);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Set a Vertical and Left Horizontal Scroll to SSD1306.
 *
 * @param[in] start_page_addr
 *   Start page address
 *
 * @param[in] end_page_addr
 *   End page address
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_scroll_diag_left(uint8_t start_page_addr, uint8_t end_page_addr)
{
  sl_status_t sc;
  uint8_t cmd_buff[10] = {
      SSD1306_SET_VERTICAL_SCROLL_AREA, //   0xA3 Right Horizontal scroll
      0x00,                             //  Set No. of rows in top fixed area
      SSD1306_DISPLAY_HEIGHT,           //  Set No. of rows in scroll area
      SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL, //  0x2A Vertical and Left Horizontal Scroll
      0x00,                             //  Dummy byte
      start_page_addr,                  //  Set start page address
      0x00,                             //  Set time interval between each scroll
      end_page_addr,                    //  Set end page address
      0x01,                             //  Vertical scrolling offset
      SSD1306_ACTIVATE_SCROLL           // 0x2F Activate scroll
  };

  sc = ssd1306_send_command(cmd_buff, 10);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Stop scroll to SSD1306.
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_stop_scroll(void)
{
  sl_status_t sc;
  uint8_t cmd;

  cmd = SSD1306_DEACTIVATE_SCROLL;   //  0x2E Deactivate scroll
  sc = ssd1306_send_command(&cmd, 1);

  return sc;
}

/**************************************************************************//**
 * @brief
 *   Set the display ON/OFF.
 *
 * @param[in] on
 *   State of display
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *****************************************************************************/
sl_status_t ssd1306_enable_display(bool on)
{
  sl_status_t sc;
  uint8_t cmd;

  if (on == true) {
    cmd = SSD1306_DISPLAYON;
  }
  else {
    cmd = SSD1306_DISPLAYOFF;
  }

  sc =  ssd1306_send_command(&cmd, 1);

  return sc;
}
