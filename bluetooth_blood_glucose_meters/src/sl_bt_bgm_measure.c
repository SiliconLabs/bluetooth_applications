/***************************************************************************//**
 * @file
 * @brief BGM measurement characteristic
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
#include "sl_simple_button_instances.h"
#include "app_timer.h"
#include "sl_bt_bgm.h"

// Glucose measurement characteristic notification enable
bool measure_enabled = false;
static bool bgm_in_process = false;
// Glucose measurement data
#if STATUS_ANNUNCIATION_SUPPORTED
uint8_t sl_glucose_records[MAX_RECORD_NUM][17] = { 0 };
#else
uint8_t sl_glucose_records[MAX_RECORD_NUM][15] = { 0 };
#endif

static app_timer_t bgm_report_all_timer;
void sl_bt_bgm_report_all_timer_cb(app_timer_t *timer, void *data);

/**************************************************************************//**
 * BGM - BGM Measurement
 * notification changed callback
 * Called when notification of BGM measurement is enabled/disabled by
 * the client.
 *****************************************************************************/
void sl_bt_bgm_measurement_notification_handler(sl_bt_msg_t *evt)
{
  sl_bt_evt_gatt_server_characteristic_status_t status = \
    evt->data.evt_gatt_server_characteristic_status;
  // client characteristic configuration changed by remote GATT client
  if (sl_bt_gatt_server_client_config == status.status_flags) {
    if (sl_bt_gatt_server_notification == status.client_config_flags) {
      app_log("measurement enable notification\n");
      measure_enabled = true;
    }  // notification disabled.
    else if (sl_bt_gatt_server_disable == status.client_config_flags) {
      app_log("measurement disable notification\n");
      measure_enabled = false;
    }
  }
}

/**************************************************************************//**
 * add a glucose measurement record
 *****************************************************************************/
void sli_bt_bgm_generate_record(uint16_t index)
{
  seq_num++;
  uint8_t temp[15] = { 0 };
  // 1. flag
  temp[0] = TIME_OFFSET_PRESENT | CON_TYPE_SAMPLE_PRESENT | UINT_MOLL_PRESENT;
  // if unit to kg/L
  // temp[0] = TIME_OFFSET_PRESENT | CON_TYPE_SAMPLE_PRESENT;
#if STATUS_ANNUNCIATION_SUPPORTED
  temp[0] = temp[0] | SENSOR_STATUS_PRESENT;
#endif
  // if measurement context is followed by measurement
  // temp[0] = temp[0] | MEASURE_CONTEXT_PRESENT;
  // 2. seq num
  temp[1] = seq_num, temp[2] = 0;
  // 3. base time 0xE507(year 2021), 7(month July), 5(day 5), 12 30
  //   5(12h:30m:5s)
  temp[3] = 0xE5, temp[4] = 7, temp[5] = 7, temp[6] = 5, temp[7] = 12;
  temp[8] = 30, temp[9] = 5;
  // 4. time offset
  temp[10] = 0, temp[11] = 0;

  // 5. Glucose Concentration field, here use a simulated algorithm
  temp[12] = 50 + records_num % 10; temp[13] = 192;
  // if unit is kg/L
  // temp[12] = 60 + seq_num; temp[13] = 176;
  // 6. location + type
  temp[14] = 0x11;
  // 7. Annunciation uint16_t
#if STATUS_ANNUNCIATION_SUPPORTED
#endif
  memcpy(sl_glucose_records[index], temp, sizeof(temp));
  records_num++;
}

// for GLS/SEN/SPT/BV-01-C [Service Procedure - Time Update]
void sli_bt_bgm_time_update(uint16_t index, int8_t hour)
{
  seq_num++;
  uint8_t temp[15] = { 0 };
  // 1. flag
  temp[0] = TIME_OFFSET_PRESENT | CON_TYPE_SAMPLE_PRESENT | UINT_MOLL_PRESENT;
  // if unit to kg/L
  // temp[0] = TIME_OFFSET_PRESENT | CON_TYPE_SAMPLE_PRESENT;
#if STATUS_ANNUNCIATION_SUPPORTED
  temp[0] = temp[0] | SENSOR_STATUS_PRESENT;
#endif
  // if measurement context is followed by measurement
  // temp[0] = temp[0] | MEASURE_CONTEXT_PRESENT;
  // 2. seq num
  temp[1] = seq_num, temp[2] = 0;
  // 3. base time 0xE507(year 2021), 7(month July), 5(day 5), 12 30
  //   5(12h:30m:5s)
  temp[3] = 0xE5, temp[4] = 7, temp[5] = 7, temp[6] = 5, temp[7] = 12;
  temp[8] = 30, temp[9] = 5;
  // 4. time offset
  if (hour == 1) {
    // +1 hour
    temp[10] = 60, temp[11] = 0;
  } else if (hour == -2) {
    // -2 hours
    temp[10] = 0xC4, temp[11] = 0xFF;
  }
  // 5. Glucose Concentration field, here use a simulated algorithm
  temp[12] = 50 + records_num % 10; temp[13] = 192;
  // if unit is kg/L
  // temp[12] = 60 + seq_num; temp[13] = 176;
  // 6. location + type
  temp[14] = 0x11;
  // 7. Annunciation uint16_t
#if STATUS_ANNUNCIATION_SUPPORTED
#endif
  memcpy(sl_glucose_records[index], temp, sizeof(temp));
  records_num++;
}

/* add 200 records at one time */
void sl_bt_bgm_add_200_measurement_records(void)
{
  for (uint16_t i = records_num; i < MAX_RECORD_NUM; i++) {
    app_log("add one record in loop\n");
    sli_bt_bgm_generate_record(i);
  }
}

/**************************************************************************//**
 * Simple Button
 * Button state changed callback
 * @param[in] handle Button event handle
 *****************************************************************************/
static volatile bool app_btn0_pressed = false;
#define SHORT_BUTTON_PRESS_DURATION    (250)
#define MEDIUM_BUTTON_PRESS_DURATION   (1000)
void sl_button_on_change(const sl_button_t *handle)
{
  if (&sl_button_btn0 != handle) {
    return;
  }
  static uint32_t timestamp = 0;
  uint32_t t_diff;
  // Button pressed.
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    app_log("button pressed\n");
    app_btn0_pressed = true;
    timestamp = sl_sleeptimer_get_tick_count();
  }
  // Button released.
  if (sl_button_get_state(handle) != SL_SIMPLE_BUTTON_RELEASED) {
    return;
  }
  t_diff = sl_sleeptimer_get_tick_count() - timestamp;
  app_log("button released\n");
  if (t_diff < sl_sleeptimer_ms_to_tick(SHORT_BUTTON_PRESS_DURATION)) {
    app_btn0_pressed = false;
    app_log("add one record\n");
    sli_bt_bgm_generate_record(records_num);
  } else if (t_diff < sl_sleeptimer_ms_to_tick(MEDIUM_BUTTON_PRESS_DURATION)) {
    app_log("add %d records\n", MAX_RECORD_NUM);
    sl_bt_bgm_add_200_measurement_records();
  } else if (t_diff < sl_sleeptimer_ms_to_tick(3000)) {
    app_log("update time 1 hours\n");
    sli_bt_bgm_time_update(records_num, 1);
  } else if (t_diff < sl_sleeptimer_ms_to_tick(6000)) {
    app_log("update time -2 hours\n");
    sli_bt_bgm_time_update(records_num, -2);
  }
}

/**************************************************************************//**
 * 4.9.1 GLS/SEN/SPR/BV-01-C [Report Stored Records - All records]
 *****************************************************************************/
#define REQUESTED_NOTIFICATION_INTERVAL 100
void sl_bt_bgm_measurement_notificate(uint16_t index)
{
  sl_status_t sc = SL_STATUS_FAIL;
  uint8_t context[20] = { 0 };

  /* 4.8 Characteristic notification
   *    context[0] = context[0] | SL_BT_BGM_CONTEXT_CARB;
   *    context[1] = sl_glucose_records[i][1];  // seq num
   *    context[2] = sl_glucose_records[i][2];
   *    context[3] = 1;  //carb ID
   *    context[4] = 50; //carb value float
   *    context[5] = 208;
   *
   *    context[0] = context[0] | SL_BT_BGM_CONTEXT_MEAL;
   *    context[1] = sl_glucose_records[i][1];
   *    context[2] = sl_glucose_records[i][2];
   *    context[3] = 1; //meal breakfast
   *
   *    context[0] = context[0] | SL_BT_BGM_CONTEXT_TESTER;
   *    context[1] = sl_glucose_records[i][1];
   *    context[2] = sl_glucose_records[i][2];
   *    context[3] = 3; //lab test
   */
  context[0] = context[0] | SL_BT_BGM_CONTEXT_EXERCISE;
  context[0] = context[0] | SL_BT_BGM_CONTEXT_MED;
  context[0] = context[0] | SL_BT_BGM_CONTEXT_MED_L;
  context[0] = context[0] | SL_BT_BGM_CONTEXT_HBA1C;
  // sequence number
  context[1] = sl_glucose_records[index][1];
  context[2] = sl_glucose_records[index][2];
  context[3] = 1; // exercise duration
  context[4] = 0;
  context[6] = 5; // exercise intensity
  context[7] = 50; // 50 milligrams or 50 milliliters
  // context[8] = 160; // SFLOAT -6
  context[8] = 208; // SFLOAT -3
  context[9] = 50; // HbA1c 50 percent
  context[10] = 0; // sfloat 0

  sc = sl_bt_gatt_server_send_notification(
    connection,
    gattdb_glucose_measurement,
    sizeof(sl_glucose_records[0]),
    sl_glucose_records[index]
    );
  if (sc) {
    app_log_warning("send measurement fail in sending all records 0x%04lX\n",
                    sc);
    while (sc == SL_STATUS_NO_MORE_RESOURCE && sc != SL_STATUS_INVALID_HANDLE) {
      sc = sl_bt_gatt_server_send_notification(
        connection,
        gattdb_glucose_measurement,
        sizeof(sl_glucose_records[0]),
        sl_glucose_records[index]
        );
    }
  }
  sc = sl_bt_gatt_server_send_notification(
    connection,
    gattdb_glucose_measurement_context,
    sizeof(context),
    context
    );
  if (sc) {
    app_log_warning("send context fail in sending all records 0x%04lX\n", sc);
    while (sc == SL_STATUS_NO_MORE_RESOURCE) {
      sc = sl_bt_gatt_server_send_notification(
        connection,
        gattdb_glucose_measurement_context,
        sizeof(context),
        context
        );
    }
  }
}

void sl_bt_bgm_report_all_timer_cb(app_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  sl_bt_external_signal(SIGNAL_REPORT_ALL);
}

void sl_bt_signal_report_all_cb(void)
{
  static uint16_t num = 0;

  bgm_abort_operation_flag = false;

  if (bgm_abort_operation_flag == true) {
    bgm_abort_operation_flag = false;
    app_log("set bgm_abort_operation_flag back to false\n");
    bgm_in_process = false;
    (void)app_timer_stop(&bgm_report_all_timer);
    return;
  }

  app_log("send %d\n", num);
  sl_bt_bgm_measurement_notificate(num);

  num++;
  if (num == records_num) {
    app_log("finished send all records\n");
    num = 0;
    bgm_in_process = false;
    (void)app_timer_stop(&bgm_report_all_timer);
    sl_bt_bgm_send_racp_indication(connection,
                                   REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

void sl_bt_bgm_report_all_records(uint8_t connection)
{
  sl_status_t sc = SL_STATUS_FAIL;
  if (bgm_in_process == false) {
    bgm_in_process = true;
  } else {
    app_log("procedure already in process.\n");
    sl_bt_bgm_send_racp_indication(connection, REPORT_STORED_RECORDS,
                                   PROCEDURE_ALREADY_IN_PROCESSED);
    return;
  }
  if (records_num < 1) {
    app_log("no records\n");
    return;
  }
  sc = app_timer_start(&bgm_report_all_timer,
                       REQUESTED_NOTIFICATION_INTERVAL,
                       sl_bt_bgm_report_all_timer_cb,
                       NULL,
                       true);
  if (sc != SL_STATUS_OK) {
    app_log("bgm_report_timer failed 0x%04lX\n", sc);
  }
}

/**************************************************************************//**
 * 4.9.2 GLS/SEN/SPR/BV-02-C
 * [Report Stored Records - Less than or equal to Sequence Number]
 *****************************************************************************/
void sl_bt_bgm_report_record_less_than(uint8_t connection, uint16_t high)
{
  sl_status_t sc = SL_STATUS_FAIL;
  if (high > records_num) {
    high = records_num;
  }
  if (records_num > 0) {
    for (uint8_t i = 0; i < high; i++) {
      sc = sl_bt_gatt_server_send_notification(
        connection,
        gattdb_glucose_measurement,
        sizeof(sl_glucose_records[0]),
        sl_glucose_records[i]
        );
      app_log("send one record, high is %d, connect is %d\n", high, connection);
    }
  }
  if (sc) {
    app_log_warning("Failed to send less than records 0x%04lX\n", sc);
  } else {
    sl_bt_bgm_send_racp_indication(connection,
                                   REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

/**************************************************************************//**
 * 4.9.3 GLS/SEN/SPR/BV-03-C
 * [Report Stored Records - Greater than or equal to Sequence Number]
 *****************************************************************************/
void sl_bt_bgm_report_records_greater_than(uint8_t connection,
                                           uint8_t filter_type, uint16_t low)
{
  sl_status_t sc = SL_STATUS_FAIL;
  // sequence number filter type
  if (filter_type == FILTER_TYPE_SEQ_NUM) {
    if (records_num > 0) {
      for (uint16_t i = low - 1 ; i < records_num; i++) {
        sc = sl_bt_gatt_server_send_notification(
          connection,
          gattdb_glucose_measurement,
          sizeof(sl_glucose_records[0]),
          sl_glucose_records[i]
          );
      }
    }
  }
  // user facing time filter
  else if (filter_type == FILTER_TYPE_USER_FACING_TIME) {
    // bgm_unsupported_operand(connection);
    if (records_num > 0) {
      for (uint16_t i = 0 ; i < 2; i++) {
        sc = sl_bt_gatt_server_send_notification(
          connection,
          gattdb_glucose_measurement,
          sizeof(sl_glucose_records[0]),
          sl_glucose_records[i]
          );
      }
    }
  }
  // unsupported operand
  else {
    sl_bt_bgm_unsupported_operand(connection);
    return;
  }
  if (sc) {
    app_log_warning("Failed to report greater than records\n");
  } else {
    sl_bt_bgm_send_racp_indication(connection,
                                   REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

/**************************************************************************//**
 * 4.9.5 GLS/SEN/SPR/BV-05-C
 * [Report Stored Records - Within range of Sequence Number value pair]
 *****************************************************************************/
void sl_bt_bgm_report_records_within_range(uint8_t connection,
                                           uint8_t filter_type,
                                           uint16_t low,
                                           uint16_t high)
{
  (void)filter_type;
  sl_status_t sc = SL_STATUS_FAIL;
  if (records_num == 0) {
    return;
  }
  for (uint16_t i = low; i <= high; i++) {
    sc = sl_bt_gatt_server_send_notification(
      connection,
      gattdb_glucose_measurement,
      sizeof(sl_glucose_records[0]),
      sl_glucose_records[i]
      );
  }
  if (sc) {
    app_log_warning("Failed to send within records\n");
  } else {
    sl_bt_bgm_send_racp_indication(connection,
                                   REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

/**************************************************************************//**
 * 4.9.6 GLS/SEN/SPR/BV-06-C [Report Stored Records – First record]
 *****************************************************************************/
void sl_bt_bgm_report_first_record(uint8_t connection)
{
  sl_status_t sc = SL_STATUS_FAIL;
  if ((measure_enabled == true) && (records_num > 0)) {
    sc = sl_bt_gatt_server_send_notification(
      connection,
      gattdb_glucose_measurement,
      sizeof(sl_glucose_records[0]),
      sl_glucose_records[0]
      );
  }
  if (sc) {
    app_log_warning("Failed to send first record.\n");
  } else {
    sl_bt_bgm_send_racp_indication(connection,
                                   REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}

/**************************************************************************//**
 * 4.9.7 GLS/SEN/SPR/BV-07-C [Report Stored Records – Last record]
 *****************************************************************************/
void sl_bt_bgm_report_last_record(uint8_t connection)
{
  sl_status_t sc = SL_STATUS_FAIL;
  if ((measure_enabled == true) && (records_num > 0)) {
    sc = sl_bt_gatt_server_send_notification(
      connection,
      gattdb_glucose_measurement,
      sizeof(sl_glucose_records[0]),
      sl_glucose_records[records_num - 1]
      );
  }
  if (sc) {
    app_log_warning("Failed to send last record\n");
  } else {
    sl_bt_bgm_send_racp_indication(connection,
                                   REPORT_STORED_RECORDS, RSP_CODE_SUCCEED);
  }
}
