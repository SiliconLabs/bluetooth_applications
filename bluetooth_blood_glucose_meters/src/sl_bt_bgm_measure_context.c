/***************************************************************************//**
 * @file
 * @brief BGM measure context characteristic
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
#include "sl_bt_bgm.h"

/*
 * Glucose Measurement characteristic value and may or may not be followed by a
 * corresponding Glucose Measurement Context characteristic value. If the
 * Glucose Measurement characteristic value is followed by a Glucose Measurement
 * Context characteristic value, the Context Information Flag in the Flags field
 * of the Glucose Measurement characteristic value shall be set to 1 and the
 * values of the Sequence Number fields shall be the same.
 */
// used when send measure context in bgm_measure.c
bool measure_context_enabled = false;

/**************************************************************************//**
 * BGM Measurement characteristic CCCD changed.
 *****************************************************************************/
void sl_bt_bgm_measurement_context_handler(sl_bt_msg_t *evt)
{
  sl_bt_evt_gatt_server_characteristic_status_t status = \
    evt->data.evt_gatt_server_characteristic_status;
  // client characteristic configuration changed by remote GATT client
  if (sl_bt_gatt_server_client_config == status.status_flags) {
    if (sl_bt_gatt_server_notification == status.client_config_flags) {
      if (sl_bt_gatt_disable != status.client_config_flags) {
        app_log("measure context enable notification\n");
        measure_context_enabled = true;
      } else {
        app_log("disable measure context notification\n");
        measure_context_enabled = false;
      }
    }
  }
}
