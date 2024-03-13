/***************************************************************************//**
 * @file
 * @brief UI functions for WSTK display (header)
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

#ifndef SL_SMARTWATCH_UI_H
#define SL_SMARTWATCH_UI_H

// <o SMARTWATCH_UI_LOG_ENABLE> Enable logging
// <i> Enables UI logging.
// <i> Default: 1
#define SMARTWATCH_UI_LOG_ENABLE          1

#if defined(SMARTWATCH_UI_LOG_ENABLE) && SMARTWATCH_UI_LOG_ENABLE
#define LOG(...) app_log_info(__VA_ARGS__)
#define LOGLN()  app_log_nl()
#else // SMARTWATCH_UI_LOG_ENABLE
#define LOG(...)
#define LOGLN()
#endif // SMARTWATCH_UI_LOG_ENABLE

/**************************************************************************//**
 *  Initialize the UI.
 *****************************************************************************/
void sl_smartwatch_ui_init(void);

/**************************************************************************//**
 * Updates the screen. Call this to update the display after writing to it.
 *****************************************************************************/
void sl_smartwatch_ui_update(void);

/**************************************************************************//**
 *  Clears entire screen
 *****************************************************************************/
void sl_smartwatch_ui_clear_screen(void);

/**************************************************************************//**
 *  Returns the number of rows it will take to print given number of characters
 *   based on the selected font and display size. Display should be initialized
 *   before calling this.
 *
 * @param[in] charCount number of characters.
 * @return number of rows required to print given number of characters.
 *****************************************************************************/
uint8_t sl_smartwatch_ui_char_to_rows(uint8_t charCount);

/**************************************************************************//**
 *  Prints a wrapped string on the display. When the maximum characters in a
 *  row are printed, the cursor moves to the next line. Multiple calls to this
 *  function will print string on next line each time.
 *
 *  @param[in] data  A pointer to the character array to be printed.
 *****************************************************************************/
void sl_smartwatch_ui_print_text_wrapped(char *data);

/**************************************************************************//**
 *  Prints wrapped string on the display shifted on X and Y axis. A global
 *  variable in sl_smartwatch_ui.c named 'row_to_print' is updated to keep a
 *  track of last printed row.
 *
 *  @param[in] data  A pointer to the character array to be printed.
 *  @param[in] xOffset The number of pixels to shift on the X axis. Positive
 *   values prints the string shifted towards right and negative values towards
 *   left.
 *  @param[in] yOffset The number of pixels to shift on the Y axis. Positive
 *   values prints the string shifted downwards and negative values upwards.
 *****************************************************************************/
void sl_smartwatch_ui_print_text_with_offset(char *data, int16_t xOffset,
                                             int16_t yOffset);

/**************************************************************************//**
 *  Clears the screen and prints time on line zero and date on line one
 *
 *  @param[in] data  A pointer to the character array to be printed.
 *****************************************************************************/
void sl_smartwatch_ui_print_time(char *data);

#endif /* SL_SMARTWATCH_UI_H */
