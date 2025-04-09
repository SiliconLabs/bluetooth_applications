/***************************************************************************//**
 * @file thermostat_app.h
 * @brief Thermostat application code
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

#ifndef THERMOSTAT_APP_H_
#define THERMOSTAT_APP_H_

#include <stdint.h>
#include "sl_bt_api.h"

/***************************************************************************//**
 * @brief
 *  Initialize the thermostat application.
 *
 ******************************************************************************/
void thermostat_app_init(void);

/***************************************************************************//**
 * @brief
 *    Handle bluetooth gatt user read request event,
 *    that is used by people counting module.
 *
 * @param[in] data
 *    User read request data.
 *
 ******************************************************************************/
void thermostat_set_bt_connection_handle(uint8_t connection);

/***************************************************************************//**
 * @brief
 *    Reset bluetooth handle to invalid handle
 *    to disable characteristic notification
 *
 ******************************************************************************/
void thermostat_reset_bt_connection_handle(void);

/***************************************************************************//**
 * @brief
 *    Handle bluetooth gatt user write request event,
 *    that is used by people counting module.
 *
 * @param[in] data
 *    User write request data.
 *
 ******************************************************************************/
void thermostat_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data);

/***************************************************************************//**
 * @brief
 *    Handle bluetooth gatt user read request event,
 *    that is used by people counting module.
 *
 * @param[in] data
 *    User read request data.
 *
 ******************************************************************************/
void thermostat_process_evt_gatt_server_user_read_request(
  sl_bt_evt_gatt_server_user_read_request_t *data);

/***************************************************************************//**
 * @brief
 *    Handle bluetooth event external signal,
 *    that trigger by people counting module.
 *
 * @param[in] extsignals
 *    Event flags.
 *
 ******************************************************************************/
void thermostat_process_evt_external_signal(uint32_t extsignals);

#endif /* THERMOSTAT_APP_H_ */
