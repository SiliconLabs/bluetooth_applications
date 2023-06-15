/***************************************************************************//**
 * @file client_rgb.h
 * @brief client led matrix file.
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
 * 1. The origin of this software must not be misrepresented{} you must not
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

#ifndef CLIENT_RGB_H
#define CLIENT_RGB_H

#include "sl_i2cspm_instances.h"
#include "glib.h"
#include "glib_font.h"
#include "sl_status.h"

#define DEFAULT_WIDTH_SLOT                2

/***************************************************************************//**
 * @brief
 *    Initialize the led matrix module.
 *
 * @param i2cspm  I2C instance
 * @return sl_status_t  status of function
 *
 ******************************************************************************/
sl_status_t client_rgb_init(glib_context_t *rgb_context, sl_i2cspm_t *i2cspm);

/***************************************************************************//**
 * @brief
 *    init glib object.
 *
 * @param rgb_context glib context object
 *
 ******************************************************************************/
void client_rgb_context_init(glib_context_t *rgb_context);

/***************************************************************************//**
 * @brief
 *    show glib content on display.
 *
 * @param rgb_context glib context object
 * @param content content need to display
 *
 ******************************************************************************/
void client_rgb_display(glib_context_t *rgb_context, const char *content);

/***************************************************************************//**
 * @brief
 *    clear buffer glib object.
 *
 * @param rgb_context glib context object
 *
 ******************************************************************************/
void client_rgb_context_clear(glib_context_t *rgb_context);

#endif /* CLIENT_RGB_H */
