/***************************************************************************//**
 * @file
 * @brief CGM Record Access Control Point characteristic
 * all the function is tested according CGMS test specification
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
#include "sl_bt_cgm.h"
#include "sl_bt_cgm_racp.h"
#include "sl_bt_cgm_measurement.h"

/** @brief Glucose measurement current record number*/
uint16_t records_num = 4;

// Glucose RACP characteristic indication enable
bool racp_indicate_enabled = false;
// in case the collector may send abort operation op code to stop the procedure
bool abort_operation = false;


/**************************************************************************//**
 * 4.10 Record Access – Report Number of Stored Records
 * This test group contains test cases to verify compliant operation
 * when the Lower Tester uses Record Access Control Point (RACP)
 * ‘Report Number of Stored Records’ procedures.
 * When the Report Number of Stored Records Op Code is written to the Record
 * Access Control Point, the Server shall calculate and respond with a record
 * count in UINT16 format based on filter criteria, Operator and Operand values
 *****************************************************************************/
void sl_bt_cgm_report_num_records(sl_bt_msg_t *evt)
{
  uint8_t operator = evt->data.evt_gatt_server_attribute_value.value.data[1];
  uint8_t connection = evt->data.evt_gatt_server_attribute_value.connection;
  switch (operator){
    case OPERATOR_ALL_RECORDS:
      app_log("report all records num\n");
      sl_bt_cgm_report_num_all_records(connection);
      break;
    //CGMS/SEN/RAN/BV-02-C [Report Number of Stored Records –
      //‘Greater than or equal to Time Offset’]
    case OPERATOR_GREATER_EQUAL:
      app_log("report number of stored records greater than\n");
      sl_bt_cgm_report_num_records_greater(evt);
      break;
    default:
      break;
  }
}

/**************************************************************************//**
 * 4.10 CGMS/SEN/RAN/BV-01-C [Report Number of Stored Records – ‘All records’]
 * 4.10 CGMS/SEN/RAN/BV-03-C[Report Number of Stored Records–‘No records found’]
 *****************************************************************************/
void sl_bt_cgm_report_num_all_records(uint8_t connection)
{
  sl_status_t sc = SL_STATUS_FAIL;
  uint8_t cgm_report_all_num_records_data[] = {RSP_NUMBER_OF_STORED_RECORDS,\
                                               OPERATOR_NULL, 0x00, 0x00};
  cgm_report_all_num_records_data[2] = records_num & 0xff;
  cgm_report_all_num_records_data[3] = records_num >> 8;
  sc = sl_bt_gatt_server_send_indication(
      connection,
      gattdb_record_access_control_point,
      sizeof(cgm_report_all_num_records_data), cgm_report_all_num_records_data
      );
  if (sc) {
    app_log_warning("Failed to report all records number\n");
  }
}

/**************************************************************************//**
 *4.10  CGMS/SEN/RAN/BV-02-C [Report Number of Stored Records –
 * ‘Greater than or equal to Time Offset’]
 *****************************************************************************/
void sl_bt_cgm_report_num_records_greater(sl_bt_msg_t *evt)
{
  sl_status_t sc = SL_STATUS_FAIL;
  uint8_t offset = evt->data.evt_gatt_server_attribute_value.value.data[3] | \
                   evt->data.evt_gatt_server_attribute_value.value.data[4] << 8;
  app_log("report records number greater than %d\n", offset);
  uint8_t reply[] = {RSP_NUMBER_OF_STORED_RECORDS, OPERATOR_NULL, 0x00, 0x00};
  records_num = records_num - 1;
  reply[2] = records_num & 0xff;
  reply[3] = records_num  >> 8;
  sc = sl_bt_gatt_server_send_indication(
      connection,
      gattdb_record_access_control_point,
      sizeof(reply), reply
      );
  if (sc) {
    app_log_warning("Failed to report records number greater than %d\n",offset);
  }
}

/**************************************************************************//**
 * 4.11 Record Access - Delete Stored Records
 * This test group contains test cases to verify compliant operation
 * when the Lower Tester uses Record Access Control Point (RACP)
 * ‘Delete Stored Records’ procedures.
 * When the Delete Stored Records Op Code is written to the Record Access
 * Control Point, the Server may delete the specified patient records based on
 * Operator and Operand values. Deletion of records may be a permanent deletion
 * of records from the patient database.
 *****************************************************************************/
void sl_bt_cgm_delete_record(sl_bt_msg_t *evt)
{
  uint8_t operator = evt->data.evt_gatt_server_attribute_value.value.data[1];
  switch (operator){
    case OPERATOR_ALL_RECORDS:
      app_log("delete all records\n");
      sl_bt_cgm_delete_all_records();
      break;
    case OPERATOR_WITHIN_RANGE:
      app_log("delete records within range of\n");
      sl_bt_cgm_delete_records_within_range(evt);
      break;
    default:
      break;
  }
}

/**************************************************************************//**
 * 4.11 CGMS/SEN/RAD/BV-01-C [Delete Stored Records – ‘All records’]
 *****************************************************************************/
void sl_bt_cgm_delete_all_records(void)
{
  records_num = 0;
  sl_bt_cgm_send_racp_indication(DELETE_STORED_RECORDS, RSP_CODE_SUCCEED);
}

/**************************************************************************//**
 * 4.11 CGMS/SEN/RAD/BV-02-C
 * [Delete Stored Records–‘Within range of (inclusive) Time Offset value pair’]
 *****************************************************************************/
void sl_bt_cgm_delete_records_within_range(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t min = array[3] | array[4] << 8;
  uint16_t max = array[5] | array[6] << 8;
  records_num = records_num - (max - min);
  sl_bt_cgm_send_racp_indication(DELETE_STORED_RECORDS, RSP_CODE_SUCCEED);
}


/**************************************************************************//**
 * 4.12 Record Access – Report Stored Records
 * This test group contains test cases to verify compliant operation
 * when the Lower Tester uses Record Access Control Point (RACP)‚
 * 'Report Stored Records’ procedures.
 *****************************************************************************/
void sl_bt_cgm_report_record(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  uint8_t opcode = array[0];
  uint8_t operator = array[1];
  uint8_t filter_type;
  uint16_t high;
  switch (operator){
    case OPERATOR_NULL:
      app_log("Invalid Operator 0x%02X\n", operator);
      sl_bt_cgm_send_racp_indication(opcode, INVALID_OPERATOR);
      break;
    case OPERATOR_ALL_RECORDS:
      if(len != 2){
    //CGMS/SEN/CBE/BI-04-C [General Error Handling – ‘Invalid Operand’ – Type 1]
        sl_bt_cgm_send_racp_indication(opcode, INVALID_OPERAND_TYPE_2);
        break;
      }
      app_log("report all records\n");
      sl_bt_cgm_report_all_records();
      break;
    case OPERATOR_LESS_OR_EQUAL:
      sl_bt_cgm_report_records_less_than(evt);
      break;
    case OPERATOR_GREATER_EQUAL:
      filter_type = array[2];
      if(filter_type != 0x01){
       //CGMS/SEN/RAE/BI-01-C [RACP specific Errors – ‘Unsupported Filter Type’]
           // Response Code Value for ‘Operand not supported’ (0x09).
        sl_bt_cgm_send_racp_indication(opcode, OPERAND_NOT_SUPPORT);
        break;
      }
      high = array[3] | array[4] << 8;
      app_log("report greater than or equal to Time offset records num\n");
      if(high > records_num){
        // CGMS/SEN/RAR/BV-03-C [Report Stored Records –
        // ‘Greater than or equal to Time Offset’]
        app_log("No Records found, greater than max record number\n");
        sl_bt_cgm_send_racp_indication(opcode, RSP_CODE_NO_RECORD_FOUND);
        break;
      }
      sl_bt_cgm_report_records_greater_than(evt);
      break;
    case OPERATOR_WITHIN_RANGE:
      app_log("report records within range of\n");
      sl_bt_cgm_report_records_within_range(evt);
      break;
    case OPERATOR_FIRST:
      app_log("report first record\n");
      sl_bt_cgm_report_first_record();
      break;
    case OPERATOR_LAST:
      app_log("report last record\n");
      sl_bt_cgm_report_last_record();
      break;
    default:
      // CGMS/SEN/CBE/BI-03-C [General Error Handling – ‘Unsupported Operator’]
      app_log("Operator 0x%02X not support\n", operator);
      sl_bt_cgm_send_racp_indication(opcode, UNSUPPORTED_OPERATOR);
      break;
  }
}

/**************************************************************************//**
 * simple timer callback function, for reported records
 * there are six types of report stored records
 * 1. All records
 * 2. Less than or equal to time offset
 * 3. Greater than or equal to time offset
 * 4. within range of(inclusive) time offset
 * above four types use sleep timer to send to avoid queue overflow
 * 5. First record
 * 6. Last record
 *****************************************************************************/
sl_simple_timer_t cgm_report_all_timer, cgm_report_less_timer;
sl_simple_timer_t cgm_report_greater_timer;
bool procedure_in_progress = false;
void sl_bt_cgm_report_all_timer_cb(sl_simple_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  sl_status_t sc = SL_STATUS_FAIL;
  static uint16_t num = 0;
  if(abort_operation == true)
  {
    app_log("abort operation in report records\n");
    sc = sl_simple_timer_stop(&cgm_report_all_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop failed 0x%04X\n", sc);
    }
    abort_operation = false;
    procedure_in_progress = false;
    num = 0;
    return;
  }
  app_log("send %d\n",num);
  sl_bt_cgm_measurement_notificate(num);
  num ++;
  if(num == records_num){
    app_log("finished send all records\n");
    num = 0;
    procedure_in_progress = false;
    sc = sl_simple_timer_stop(&cgm_report_all_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop failed 0x%04X\n", sc);
    }
    sl_bt_cgm_send_racp_indication(REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

/**************************************************************************//**
 * 4.12 Record Access – Report Stored Records
 * CGMS/SEN/RAR/BV-01-C [Report Stored Records – ‘All records’]
 * start a timer to send measurements of requested notifications
 *****************************************************************************/
void sl_bt_cgm_report_all_records(void)
{
  if(procedure_in_progress == false){
    procedure_in_progress = true;
  }else{
      return;
  }
  sl_status_t sc = SL_STATUS_FAIL;
  sc = sl_simple_timer_start(&cgm_report_all_timer,
                             REQUESTED_NOTIFICATION_INTERVAL,
                             sl_bt_cgm_report_all_timer_cb,
                             NULL,
                             true);
  if(sc != SL_STATUS_OK){
    app_log("cgm_report_timer failed 0x%04X\n", sc);
  }
}


/**************************************************************************//**
 * 4.12 CGMS/SEN/RAR/BV-02-C
 * [Report Stored Records – ‘Less than or equal to Time Offset’]
 *****************************************************************************/
void sl_bt_cgm_report_less_timer_cb(sl_simple_timer_t *timer, void *data)
{
  (void)timer;
  sl_status_t sc = SL_STATUS_FAIL;
  uint16_t max = *(uint16_t *)data;
  static uint16_t num = 0;
  if(abort_operation == true)
  {
    app_log("abort operation in report less records\n");
    sc = sl_simple_timer_stop(&cgm_report_less_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop in less than records failed 0x%04X\n", sc);
    }
    abort_operation = false;
    procedure_in_progress = false;
    num = 0;
    return;
  }
  app_log("send %d\n",num);
  sl_bt_cgm_measurement_notificate(num);
  num ++;
  if(num == max){
    app_log("finished send less than %d records\n", max);
    num = 0;
    procedure_in_progress = false;
    sc = sl_simple_timer_stop(&cgm_report_less_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop in less than records failed 0x%04X\n", sc);
    }
    sl_bt_cgm_send_racp_indication(REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

void sl_bt_cgm_report_records_less_than(sl_bt_msg_t *evt)
{
  uint16_t maximum = evt->data.evt_gatt_server_attribute_value.value.data[3] | \
                   evt->data.evt_gatt_server_attribute_value.value.data[4] << 8;

  if(procedure_in_progress == false){
    procedure_in_progress = true;
  }else{
      return;
  }
  sl_status_t sc = SL_STATUS_FAIL;
  sc = sl_simple_timer_start(&cgm_report_less_timer,
                             REQUESTED_NOTIFICATION_INTERVAL,
                             sl_bt_cgm_report_less_timer_cb,
                             &maximum,
                             true);
  if(sc != SL_STATUS_OK){
    app_log("cgm_report_less_timer failed 0x%04X\n", sc);
  }

}


/**************************************************************************//**
 * 4.12 CGMS/SEN/RAR/BV-03-C
 * [Report Stored Records – ‘Greater than or equal to Time Offset’]
 *****************************************************************************/
void sl_bt_cgm_report_greater_timer_cb(sl_simple_timer_t *timer, void *data)
{
  (void)timer;
  sl_status_t sc = SL_STATUS_FAIL;
  uint16_t min = *(uint16_t *)data;
  static uint16_t num = 0;
  static bool init = false;
  if(init == false){
      num = min;
      init = true;
  }
  if(abort_operation == true)
  {
    app_log("abort operation in report greater than records\n");
    sc = sl_simple_timer_stop(&cgm_report_greater_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop in less than records failed 0x%04X\n", sc);
    }
    abort_operation = false;
    procedure_in_progress = false;
    num = 0;
    return;
  }
  app_log("send %d\n",num);
  sl_bt_cgm_measurement_notificate(num);
  num ++;
  if(num == records_num){
    app_log("finished send greater than %d records\n", min);
    num = 0;
    procedure_in_progress = false;
    sc = sl_simple_timer_stop(&cgm_report_greater_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop in greater records failed 0x%04X\n", sc);
    }
    sl_bt_cgm_send_racp_indication(REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

void sl_bt_cgm_report_records_greater_than(sl_bt_msg_t *evt)
{
  uint16_t minimum = evt->data.evt_gatt_server_attribute_value.value.data[3] | \
                   evt->data.evt_gatt_server_attribute_value.value.data[4] << 8;

  if(procedure_in_progress == false){
     procedure_in_progress = true;
   }else{
       return;
   }

  sl_status_t sc = SL_STATUS_FAIL;
  sc = sl_simple_timer_start(&cgm_report_greater_timer,
                             REQUESTED_NOTIFICATION_INTERVAL,
                             sl_bt_cgm_report_greater_timer_cb,
                             &minimum,
                             true);
  if(sc != SL_STATUS_OK){
    app_log("cgm_report_greater_timer failed 0x%04X\n", sc);
  }
}

/**************************************************************************//**
 * 4.12 CGMS/SEN/RAR/BV-04-C
 * [Report Stored Records –‘Within range of (inclusive) Time Offset value pair’]
 *****************************************************************************/
sl_simple_timer_t cgm_report_within_timer;
void sl_bt_cgm_report_within_timer_cb(sl_simple_timer_t *timer, void *data)
{
  (void)timer;
  sl_status_t sc = SL_STATUS_FAIL;
  uint16_t min = *(uint16_t *)data;
  uint16_t max = *(uint16_t *)(data+sizeof(uint16_t));
  static uint16_t index = 0;
  static bool init = false;
  if(init == false){
      index = min;
      init = true;
  }

  if(abort_operation == true)
  {
    app_log("abort operation in report within records\n");
    sc = sl_simple_timer_stop(&cgm_report_within_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop in within failed 0x%04X\n", sc);
    }
    abort_operation = false;
    procedure_in_progress = false;
    return;
  }
  app_log("send index %d record\n", index);
  sl_bt_cgm_measurement_notificate(index);
  index ++;
  if(index == max){
    app_log("finished send within between %d and %d records\n", min, max);
    procedure_in_progress = false;
    sc = sl_simple_timer_stop(&cgm_report_less_timer);
    if(sc != SL_STATUS_OK){
      app_log("sl_simple_timer_stop in within range failed 0x%04X\n", sc);
    }
    sl_bt_cgm_send_racp_indication(REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

void sl_bt_cgm_report_records_within_range(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  uint8_t opcode = array[0];
  if(len != 7){
      app_log("wrong data, the length of command is wrong\n");
      return;
  }
  uint16_t min = array[3] | array[4] << 8;
  uint16_t max = array[5] | array[6] << 8;
  uint16_t data[] = {min, max};
  if(min > max){
    //CGMS/SEN/CBE/BI-05-C [General Error Handling – ‘Invalid Operand’ – Type 2]
    app_log("Invalid Operand Type 2\n");
    sl_bt_cgm_send_racp_indication(opcode, INVALID_OPERAND_TYPE_2);
    return;
  }

  if(procedure_in_progress == false){
     procedure_in_progress = true;
   }else{
       return;
   }

  sl_status_t sc = SL_STATUS_FAIL;
  sc = sl_simple_timer_start(&cgm_report_within_timer,
                             REQUESTED_NOTIFICATION_INTERVAL,
                             sl_bt_cgm_report_within_timer_cb,
                             data,
                             true);
  if(sc != SL_STATUS_OK){
    app_log("cgm_report_within_timer failed 0x%04X\n", sc);
  }

}

/**************************************************************************//**
 * 4.12 CGMS/SEN/RAR/BV-05-C [Report Stored Records – ‘First record’]
 *****************************************************************************/
void sl_bt_cgm_report_first_record()
{
  sl_status_t sc = SL_STATUS_FAIL;
  if(abort_operation == true)
  {
    app_log("abort operation\n");
    return;
  }
  uint8_t first_record[] = {0x0D,0xE3,04,00,00,00,01,01,01,00,00,00,00};
  sc = sl_bt_gatt_server_send_notification(connection,
                                           gattdb_cgm_measurement,
                                           sizeof(first_record),
                                           first_record
                                           );
  app_assert_status(sc);
  if(abort_operation == false)
  {
    sl_bt_cgm_send_racp_indication(REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }else{
    abort_operation = false;
  }
}


/**************************************************************************//**
 * 4.12 CGMS/SEN/RAR/BV-06-C [Report Stored Records – ‘Last record’]
 *****************************************************************************/
void sl_bt_cgm_report_last_record()
{
  sl_status_t sc = SL_STATUS_FAIL;
  if(abort_operation == true)
  {
    app_log("abort operation\n");
    return;
  }
  // TODO: call sl_bt_cgm_measurement_notificate instead
  uint8_t last_record[] = {0x0D,0xE3,0x08,00,0x08,00,01,01,01,00,00,00,00};
  sc = sl_bt_gatt_server_send_notification(connection,
                                           gattdb_cgm_measurement,
                                           sizeof(last_record),
                                           last_record
                                           );
  app_assert_status(sc);
  if(abort_operation == false)
  {
    sl_bt_cgm_send_racp_indication(REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }else{
    abort_operation = false;
  }
}


/**************************************************************************//**
 * 4.13 Record Access – Abort operation procedure
 * This test group contains test cases to verify compliant operation
 * when the Lower Tester uses Record Access Control Point (RACP)
 * 'Abort Operation’ procedure.
 * CGMS/SEN/RAA/BV-01-C [Abort Operation – ‘Report Stored Records’]
 * Once all RACP procedures have been stopped, the Server shall indicate the
 * Record Access Control Point with a Response Code Op Code and Response Code
 * Value in the Operand set to Success
 *****************************************************************************/
void sl_bt_cgm_abort_operation(sl_bt_msg_t *evt)
{
  abort_operation = true;
  procedure_in_progress = false;
  uint8_t operator = evt->data.evt_gatt_server_attribute_value.value.data[1];
  if(operator != OPERATOR_NULL){
    return;
  }
  sl_status_t sc = SL_STATUS_FAIL;
  uint8_t cgm_abort_operation_data[] = {RSP_CODE_RACP, OPERATOR_NULL, \
                                        ABORT_OPERATION, RSP_CODE_SUCCEED};
  sc = sl_bt_gatt_server_send_indication(
      connection,
      gattdb_record_access_control_point,
      sizeof(cgm_abort_operation_data),
      cgm_abort_operation_data
      );
  if (sc) {
    app_log_warning("Failed to send indication in abort operation 0x%04x\n",sc);
    while(sc == SL_STATUS_NO_MORE_RESOURCE){
      sc = sl_bt_gatt_server_send_indication(connection,
                                             gattdb_record_access_control_point,
                                             sizeof(cgm_abort_operation_data),
                                             cgm_abort_operation_data
                                             );
    }
  }
}


