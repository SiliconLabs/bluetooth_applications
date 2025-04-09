/***************************************************************************//**
 * @file client_oled_app.h
 * @brief client oled application header file.
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

#ifndef CLIENT_OLED_APP_H_
#define CLIENT_OLED_APP_H_

#include "sl_i2cspm_instances.h"
#include "glib.h"
#include "glib_font.h"
#include "micro_oled_ssd1306.h"
#include "app_timer.h"
#include "custom_font.h"

/***************************************************************************//**
 * @brief
 *    Typedef for holding display parameters.
 ******************************************************************************/
typedef struct {
  uint8_t param_text[32]; ///< Threshold parameter text
  uint8_t *first_letter; ///<  Actual first letter of the param_text
  uint8_t max_char_per_line; ///< Maximum length of characters to be displayed
  uint8_t text_length; ///< Length of the param_text
  app_timer_t timer_handle; ///< Timer handler for screen update
} disconnected_display_data_t;

/***************************************************************************//**
 * @brief
 *    Client Oled Application Initialize.
 *
 ******************************************************************************/
void client_oled_app_init(void);

/***************************************************************************//**
 * @brief
 *    Client Oled Application Display Value.
 *
 *  @param[in] radiation_value.
 *    Radiation value receive from the sender if the notification is turned on.
 *
 * @details
 *    This function shows the received radiation value from the sender.
 *
 ******************************************************************************/
void client_oled_app_radiation_display(float radiation_value);

/***************************************************************************//**
 * @brief
 *    Client Oled Application Disconnected Display.
 *
 * @details
 *    This function notifies users that the client is disconnecting to the
 *    sender by displaying disconnected on Oled.
 *
 ******************************************************************************/
void client_oled_app_disconnected_display(void);

/***************************************************************************//**
 * @brief
 *    Client Oled Application Configuration Mode Display.
 *
 * @details
 *    This function notifies users that the client is in configuration mode
 *    by displaying the information on Oled.
 *
 ******************************************************************************/
void client_oled_app_config_mode_display(void);

#endif /* CLIENT_OLED_APP_H_ */
