/***************************************************************************//**
 * @file app_ble_events.h
 * @brief Application BLE Events
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
#ifndef APP_BLE_EVENTS_H_
#define APP_BLE_EVENTS_H_

#ifdef __cplusplus
extern "C" {
#endif

// BLE External event identifiers
#define MD_LAST_REQ_TIMEOUT_TIMER_EVENT     (1 << 0)
#define MD_ACC_WAKEUP_EVENT                 (1 << 1)
#define MD_WAKEUP_PERIOD_TIMER_EVENT        (1 << 2)
#define MD_NOTIFY_TIMER_EVENT               (1 << 3)
#define MD_NOTIFY_BREAK_TIMER_EVENT         (1 << 4)

#define BLE_CHAR_ACCESS_TYPE_READ           0
#define BLE_CHAR_ACCESS_TYPE_WRITE          1

#ifdef __cplusplus
}
#endif

#endif /* APP_BLE_EVENTS_H_ */
