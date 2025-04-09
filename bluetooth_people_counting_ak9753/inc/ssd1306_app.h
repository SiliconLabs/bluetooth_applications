/***************************************************************************//**
 * @file ssd1306_app.h
 * @brief Oled application code.
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

#ifndef SSD1306_APP_H_
#define SSD1306_APP_H_

/***************************************************************************//**
 * @brief
 *  Initialize the OLED display.
 *
 ******************************************************************************/
void ssd1306_app_init(void);

/***************************************************************************//**
 * @brief
 *  Show the number of people count and people entered the room so far on the
 *  OLED display
 *
 * @param[in] people_count
 *  The number of people count
 *
 * @param[in] people_entered_so_far
 *  The number of people entered the room so far
 *
 ******************************************************************************/
void ssd1306_show_people_count(uint32_t people_count,
                               uint32_t people_entered_so_far);

#endif /* SSD1306_APP_H_ */
