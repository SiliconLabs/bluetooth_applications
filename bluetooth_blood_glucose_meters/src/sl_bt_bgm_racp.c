/***************************************************************************//**
 * @file
 * @brief BGM service procedures, include feature read
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

/**************************************************************************//**
 * 4.6 GLS/SEN/CR/BV-01-C [Characteristic Read – Glucose Feature]
 * GLS/SEN/CR/BV-02-C [Characteristic Read – Glucose Feature - Multiple Bonds]
 *****************************************************************************/
void sl_bt_bgm_read_feature(sl_bt_msg_t *evt)
{
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  uint16_t sent_len;
  size_t len = 2;
  uint8_t att_errorcode = 0;
  uint8_t value[] = { 0x00, 0x00 };
#if SL_BT_BGM_FEATURE_MULTI_BOND_SUPPORT
  value[1] |= SL_BT_BGM_FEATURE_MULTI_BOND;
#endif
  sl_bt_gatt_server_send_user_read_response(connection,
                                            gattdb_glucose_feature,
                                            att_errorcode,
                                            len,
                                            value,
                                            &sent_len);
}

/**************************************************************************//**
 * 4.9 Service Procedures – Report Stored Records
 *****************************************************************************/
void sl_bt_bgm_report_record(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  uint8_t operator = array[1];
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  uint8_t filter_type = 0;
  uint16_t operand1 = 0, operand2 = 0;
  switch (operator) {
    case OPERATOR_NULL:
      sl_bt_bgm_invalid_operator(connection);
      return;
    case OPERATOR_ALL_RECORDS:
      app_log("report all records\n");
      if (len != 2) {
        sl_bt_bgm_invalid_operand_type1(connection);
        return;
      }
      sl_bt_bgm_report_all_records(connection);
      break;
    case OPERATOR_LESS_OR_EQUAL:
      app_log("report less than record\n");
      if (len != 5) {
        sl_bt_bgm_invalid_operand_type1(connection);
        return;
      }
      filter_type = array[2];
      operand1 = array[3] | array[4] << 8;
      sl_bt_bgm_report_record_less_than(connection, operand1);
      break;
    case OPERATOR_GREATER_EQUAL:
      app_log("report greater than record\n");
      filter_type = array[2];
      if ((filter_type == 0x01) && (len != 5)) {
        sl_bt_bgm_invalid_operand_type1(connection);
        return;
      }
      operand1 = array[3] | array[4] << 8;
      if ((filter_type == FILTER_TYPE_SEQ_NUM) && (operand1 > seq_num)) {
        sl_bt_bgm_send_racp_indication(connection,
                                       REPORT_STORED_RECORDS, NO_RECORDS_FOUND);
        return;
      }
      if ((filter_type != FILTER_TYPE_SEQ_NUM)
          && (filter_type != FILTER_TYPE_USER_FACING_TIME)) {
        sl_bt_bgm_send_racp_indication(connection,
                                       REPORT_STORED_RECORDS,
                                       OPERAND_NOT_SUPPORTED);
        return;
      }
      sl_bt_bgm_report_records_greater_than(connection, filter_type, operand1);
      break;
    case OPERATOR_WITHIN_RANGE:
      app_log("report within range of sequence number value pair\n");
      if (len != 7) {
        sl_bt_bgm_invalid_operand_type1(connection);
        return;
      }
      filter_type = array[2];
      operand1 = array[3] | array[4] << 8;
      operand2 = array[5] | array[6] << 8;
      if (operand2 < operand1) {
        sl_bt_bgm_invalid_operand_type2(connection);
        return;
      }
      if ((operand2 > records_num) || (operand1 > records_num)) {
        sl_bt_bgm_invalid_operand_type2(connection);
        return;
      }
      sl_bt_bgm_report_records_within_range(connection, filter_type,
                                            operand1, operand2);
      break;
    case OPERATOR_FIRST:
      app_log("read first record\n");
      sl_bt_bgm_report_first_record(connection);
      break;
    case OPERATOR_LAST:
      app_log("read last record\n");
      sl_bt_bgm_report_last_record(connection);
      break;
    default:
      app_log("read record unsupported operator\n");
      sl_bt_bgm_unsupported_operator(connection);
      break;
  }
}

/**************************************************************************//**
 * 4.10 Service Procedures – Delete Stored Records
 *****************************************************************************/
void sl_bt_bgm_delete_record(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  if (len < 2) {
    sl_bt_bgm_invalid_operator(connection);
  }
  uint8_t operator = array[1];
  uint8_t filter_type = 0, operand1 = 0, operand2 = 0;
  switch (operator)
  {
    case OPERATOR_ALL_RECORDS:
      app_log("delete all records\n");
      sl_bt_bgm_delete_all_records(connection);
      break;
    case OPERATOR_WITHIN_RANGE:
      app_log("delete records within range\n");
      if (len != 7) {
        sl_bt_bgm_invalid_operand_type1(connection);
        return;
      }
      filter_type = array[2];
      operand1 = array[3] | array[4] << 8;
      operand2 = array[5] | array[6] << 8;
      if (filter_type != FILTER_TYPE_SEQ_NUM) {
        app_log("unsupported operand\n");
        sl_bt_bgm_unsupported_operand(connection);
      }
      if (operand2 < operand1) {
        sl_bt_bgm_invalid_operand_type2(connection);
        return;
      }
      if (operand2 > seq_num - 1) {
        sl_bt_bgm_invalid_operand_type2(connection);
        return;
      }
      sl_bt_bgm_delete_within_range_seq(connection, operand1, operand2);
      break;
    default:
      app_log("unknown report record opcode\n");
      sl_bt_bgm_unsupported_operator(connection);
      break;
  }
}

/**************************************************************************//**
 * 4.11 Service Procedures – Abort Operation
 * 4.11.1 GLS/SEN/SPA/BV-01-C [Abort Operation – Report Stored Records]
 *****************************************************************************/
void sl_bt_bgm_abort_operation(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t opcode = array[0];
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  if (len != 2) {
    sl_bt_bgm_invalid_operator(connection);
  }
  uint8_t operator = array[1];
  if (operator != OPERATOR_NULL) {
    sl_bt_bgm_unsupported_operator(connection);
  } else {
    bgm_abort_operation_flag = true;
    app_log("set bgm_abort_operation_flag true\n");
  }
  sl_bt_bgm_send_racp_indication(connection, opcode, RSP_CODE_SUCCEED);
}

/**************************************************************************//**
 * 4.12 Service Procedures – Report Number of Stored Records
 *****************************************************************************/
void sl_bt_bgm_report_num_records(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  uint8_t operator = array[1];
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  uint8_t filter_type = 0;
  uint16_t operand2 = 0;
  switch (operator) {
    // report all records
    case OPERATOR_ALL_RECORDS:
      if (len != 2) {
        sl_bt_bgm_invalid_operand_type1(connection);
        return;
      }
      sl_bt_bgm_report_num_all_records(connection);
      break;
    case OPERATOR_GREATER_EQUAL:
      if (len != 5) {
        sl_bt_bgm_invalid_operand_type1(connection);
        return;
      }
      filter_type = array[2];
      operand2 = array[3] | array[4] >> 8;
      if (filter_type != 0x01) {
        sl_bt_bgm_unsupported_operand(connection);
        return;
      }
      if (operand2 > records_num) {
        sl_bt_bgm_unsupported_operand(connection);
      }
      sl_bt_bgm_report_greater_num_records(connection, operand2);
      break;
    case OPERATOR_WITHIN_RANGE:
      app_log("report records number within range of\n");
      sl_bt_bgm_report_within_range(evt);
      break;
    default:
      sl_bt_bgm_unsupported_operator(connection);
      break;
  }
}

uint8_t rsp_report_num_records[] = { RSP_NUMBER_OF_STORED_RECORDS,
                                     OPERATOR_NULL, 0x00, 0x00 };

/**************************************************************************//**
 * 4.12.1 GLS/SEN/SPN/BV-01-C [Report Number of Stored Records - All records]
 * 4.12.3 GLS/SEN/SPN/BV-03-C[Report Number of Stored Records –No records found]
 *****************************************************************************/
void sl_bt_bgm_report_num_all_records(uint8_t connection)
{
  sl_status_t sc = SL_STATUS_FAIL;
  rsp_report_num_records[2] = records_num & 0xff;
  rsp_report_num_records[3] = records_num >> 8;
  sc = sl_bt_gatt_server_send_indication(
    connection,
    gattdb_record_access_control_point,
    sizeof(rsp_report_num_records),
    rsp_report_num_records
    );
  if (sc) {
    app_log_warning("Failed to report all records number\n");
  }
}

/**************************************************************************//**
 * 4.10.1 GLS/SEN/SPD/BV-01-C [Delete Stored Records - All records]
 *****************************************************************************/
void sl_bt_bgm_delete_all_records(uint8_t connection)
{
  const uint8_t rsp[] = { DELETE_STORED_RECORDS, OPERATOR_ALL_RECORDS };
  sl_bt_gatt_server_send_indication(
    connection,
    gattdb_record_access_control_point,
    sizeof(rsp),
    rsp
    );
  records_num = 0;
  // memset(sl_glucose_records, 0, sizeof(sl_glucose_records));
}

/**************************************************************************//**
 * 4.10.2 GLS/SEN/SPD/BV-02-C
 * [Delete Stored Records - Within range of Sequence Number value pair]
 *****************************************************************************/
void sl_bt_bgm_delete_within_range_seq(uint8_t connection,
                                       uint16_t low, uint16_t high)
{
  uint8_t rsp[] = { RSP_CODE, OPERATOR_NULL,
                    DELETE_STORED_RECORDS, RSP_CODE_SUCCEED };
  sl_bt_gatt_server_send_indication(
    connection,
    gattdb_record_access_control_point,
    sizeof(rsp),
    rsp
    );
  records_num = records_num - (high - low + 1);
}

/**************************************************************************//**
 * 4.12.2 GLS/SEN/SPN/BV-01-C
 * [Report Number of Stored Records - Greater than or equal to Sequence Number]
 *****************************************************************************/
void sl_bt_bgm_report_greater_num_records(uint8_t connection, uint16_t high)
{
  sl_status_t sc = SL_STATUS_FAIL;
  uint16_t temp = records_num - high + 1;
  rsp_report_num_records[2] = temp & 0xff;
  rsp_report_num_records[3] = temp >> 8;
  sc = sl_bt_gatt_server_send_indication(
    connection,
    gattdb_record_access_control_point,
    sizeof(rsp_report_num_records),
    rsp_report_num_records
    );
  if (sc) {
    app_log_warning("Failed to report greater/equal sequence number records\n");
  }
}

/**************************************************************************//**
 * can be used in GLS/SEN/SPD/BV-02-C
 * [Delete Stored Records - Within range of Sequence Number value pair]
 *****************************************************************************/
void sl_bt_bgm_report_within_range(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  uint16_t low = array[3] | array[4] >> 8;
  uint16_t high = array[5] | array[6] >> 8;
  sl_status_t sc = SL_STATUS_FAIL;
  uint16_t temp = high - low;
  rsp_report_num_records[2] = temp & 0xff;
  rsp_report_num_records[3] = temp >> 8;
  sc = sl_bt_gatt_server_send_indication(
    connection,
    gattdb_record_access_control_point,
    sizeof(rsp_report_num_records),
    rsp_report_num_records
    );
  if (sc) {
    app_log_warning("Failed to report all records number\n");
  }
}
