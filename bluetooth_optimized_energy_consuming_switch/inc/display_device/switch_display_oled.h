/***************************************************************************//**
 * @file switch_display_oled.h
 * @brief switch display oled header file.
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

#ifndef SWITCH_DISPLAY_OLED_H_
#define SWITCH_DISPLAY_OLED_H_

#include "glib.h"
#include "glib_font.h"
#include "sl_status.h"

#define CIRCLE_WHITE 0
#define CIRCLE_BLACK 1

/***************************************************************************//**
 * switch_status_oled_init.
 * brief: display Oled initialization
 * return: SL_STATUS_OK if success or SL_STATUS_FAIL if error.
 ******************************************************************************/
sl_status_t switch_status_oled_init(void);

/***************************************************************************//**
 * switch_status_oled_update.
 * brief: switch status update to display
 * paramater: [in] circle_color: color for circle on oled.
 *            [in] switch_status: switch status.
 * return: null
 ******************************************************************************/
void switch_status_oled_update(uint8_t circle_color,
                               const char *switch_status);

#endif /* SWITCH_DISPLAY_OLED_H_ */
