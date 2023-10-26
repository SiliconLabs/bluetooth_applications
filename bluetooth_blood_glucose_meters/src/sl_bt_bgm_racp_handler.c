/***************************************************************************//**
 * @file
 * @brief BGM Record Access Control Point characteristic
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

bool racp_enabled = false;

/**************************************************************************//**
 * When a procedure is complete, the Server indicates the RACP characteristic
 * with the Op Code set to Response Code.
 *****************************************************************************/
void sl_bt_bgm_send_racp_indication(uint8_t connection,
                                    uint8_t opcode, uint8_t error)
{
  sl_status_t sc = SL_STATUS_FAIL;
  uint8_t buf[4] = { RSP_CODE, OPERATOR_NULL, 0x01, 0x01 };
  buf[2] = opcode;
  buf[3] = error;
  sc = sl_bt_gatt_server_send_indication(
    connection,
    gattdb_record_access_control_point,
    sizeof(buf),
    buf);
  if (sc) {
    app_log_warning("Failed to send indication in RACP 0x%04lX\n", sc);
  }
}

/**************************************************************************//**
 * BGM - BGM Record Access Control Point characteristic
 * indication changed callback
 * Called when indication of BGM RCAP is enabled/disabled by
 * the client.
 *****************************************************************************/
void sl_bt_bgm_racp_indication_handler(sl_bt_msg_t *evt)
{
  sl_bt_evt_gatt_server_characteristic_status_t status = \
    evt->data.evt_gatt_server_characteristic_status;
  // client characteristic configuration changed by remote GATT client
  if (sl_bt_gatt_server_client_config == status.status_flags) {
    if (sl_bt_gatt_server_indication == status.client_config_flags) {
      app_log("RACP enable indicate\n");
      racp_enabled = true;
    } else if (sl_bt_gatt_server_disable == status.client_config_flags) {
      app_log("RACP disable indicate\n");
      racp_enabled = false;
    }
  }
  // confirmation of indication received from remove GATT client
  else if (sl_bt_gatt_server_confirmation == status.status_flags) {
    app_log("RACP receive indication confirm\n");
  }
}

/**************************************************************************//**
 * a procedure is started when a write to the RACP characteristic is
 * successfully completed. This fucntion handle all the RACP write request
 *****************************************************************************/
void sl_bt_bgm_racp_handler(sl_bt_msg_t *evt)
{
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  for (int i = 0; i < len; i++) {
    app_log("0x%02x ", evt->data.evt_gatt_server_attribute_value.value.data[i]);
  }
  app_log("\n");
  uint8_t opcode = evt->data.evt_gatt_server_attribute_value.value.data[0];
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  if ((measure_enabled == false) || (racp_enabled == false)) {
    sl_bt_gatt_server_send_user_write_response(connection,
                                               gattdb_record_access_control_point,
                                               GATT_NOT_INDICATED);
    return;
  } else if (security_level == 1) {
    sl_bt_gatt_server_send_user_write_response(connection,
                                               gattdb_record_access_control_point,
                                               ATT_INSUFFICIENT_AUTH);
    return;
  } else {
    sl_bt_gatt_server_send_user_write_response(connection,
                                               gattdb_record_access_control_point,
                                               ATT_SUCCEED);
  }
  switch (opcode) {
    case REPORT_STORED_RECORDS:
      sl_bt_bgm_report_record(evt);
      break;
    case DELETE_STORED_RECORDS:
      sl_bt_bgm_delete_record(evt);
      break;
    case ABORT_OPERATION:
      sl_bt_bgm_abort_operation(evt);
      break;
    case REPORT_NUMBER_OF_STORED_RECORDS:
      sl_bt_bgm_report_num_records(evt);
      break;
    default:
      app_log("unsupported opcode\n");
      sl_bt_bgm_unsupported_opcode(connection, opcode);
      break;
  }
}
