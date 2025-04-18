/***************************************************************************//**
 * @file app_events.h
 * @brief Application Events
 * @version 1.0.0
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
 *
 * # EXPERIMENTAL QUALITY
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/

#ifndef APP_EVENTS_H_
#define APP_EVENTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief
 *    BLE Event Handler: This function handles BLE user read/write requests for
 *    the configured characteristics.
 ******************************************************************************/
void app_event_handler_on_char_requests(uint8_t access_type, sl_bt_msg_t *evt);

/***************************************************************************//**
 * @brief
 *    BLE Event Handler: This function handles external events, like expiring
 *    timers, pressing buttons.
 ******************************************************************************/
void app_event_handler_on_external_event(sl_bt_msg_t *evt);

/***************************************************************************//**
 * @brief
 *    Event handler for notification status configuration.
 ******************************************************************************/
void app_event_handler_notification_status_changed(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENTS_H_ */
