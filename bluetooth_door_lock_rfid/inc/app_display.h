/***************************************************************************//**
 * @file app_display
 * @brief OLED DOOR LOCK App Display
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
#ifndef APP_DISPLAY_H_
#define APP_DISPLAY_H_

/***************************************************************************//**
 * @brief
 *    Update display function
 *
 * @param[in] string_midle
 *    Middle string of the Oled display.
 * @param[in] string_lower
 *    Lower string of the Oled display.
 *
 ******************************************************************************/
void app_display_update(const char *string_midle, const char *string_lower);

/***************************************************************************//**
 * @brief
 *    Show card function
 *
 * @param[in] location
 *    Location of the card to display.
 * @param[in] id_tag[6]
 *    Array contain the Card ID.
 *
 ******************************************************************************/
void app_display_show_card(uint8_t location, uint8_t *id_tag);

/***************************************************************************//**
 * @brief
 *    App display init function
 *
 ******************************************************************************/
void app_display_init(void);

#endif /* APP_DISPLAY_H */
