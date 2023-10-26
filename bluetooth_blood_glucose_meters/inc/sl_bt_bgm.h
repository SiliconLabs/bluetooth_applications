/***************************************************************************//**
 * @file
 * @brief BGM header file
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
 ******************************************************************************/
#ifndef SL_BT_BGM_H_
#define SL_BT_BGM_H_

#include "stdbool.h"
#include "gatt_db.h"
#include "sl_status.h"
#include "sl_bt_api.h"
#include "app_log.h"
#include "app_assert.h"
#include "sl_sleeptimer.h"

/**
 * @addtogroup bluetooth
 * @{
 *
 * @brief bluetooth related
 *
 */

/** @brief current connection handle*/
extern uint8_t connection;
extern uint8_t security_level;

/** @brief AD type value when create customer advertisement*/
#define AD_PUBLIC_ADDRESS         0x17
#define AD_RANDOM_ADDRESS         0x18

/** @brief oneshot timer for After 30 seconds (reduced power)
 *    Advertising Interval 1 s to 2.5 s*/
#define SL_BGM_FAST_ADV_TIMEOUT   30 * 1000

#define SIGNAL_FAST_ADV_TIMEOUT   1
#define SIGNAL_REPORT_ALL         2

/**************************************************************************//**
 * Bluetooth stack event handler.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_bgm_on_event(sl_bt_msg_t *evt);

/** @} */ // end addtogroup bluetooth

// Glucose feature
// Multiple-Bond Supported
#define SL_BT_BGM_FEATURE_MULTI_BOND_SUPPORT    true

#define SL_BT_BGM_FEATURE_MULTI_BOND            0x04

/**************************************************************************//**
 *    GLS/SEN/CR/BV-01-C [Characteristic Read – Glucose Feature]
 *****************************************************************************/
void sl_bt_bgm_read_feature(sl_bt_msg_t *evt);

/**
 * @addtogroup  measurement characteristic functions
 * @{
 *
 * @brief  measurement characteristic functions
 *
 * A couple of measurement characteristic functions.
 */
extern bool measure_enabled;
#define BGM_DEFUALT_RECORDS_NUM 5

/** @brief Glucose measurement records max number */
#define MAX_RECORD_NUM          50
// Glucose measurement current record number
extern uint16_t records_num;
// Glucose sequence number
extern uint8_t seq_num;

/* the BGM measurement struct:
 * 1. Flags field (containing units of glucose, the existence of context
 *                 information and used to show presence of optional fields),
 *                 1 byte
 * 2. Sequence Number field, 2 byets, stored in NVM, can not cleared
 * 3. Base Time field (time of the measurement),
 *    7 bytes, internal real-time clock
 *    there is no Base Time field in CGM, CGM has session start time field.
 *
 * depending upon the contents of the Flags field, below fields are optional
 * 4. Time Offset field, 2bytes, The Time offset field is included in first
 *    transmitted record and at least any time when it changes.
 *    The Time Offset field is defined as a 16-bit signed integer representing
 *    the number of minutes the user-facing time differs from the Base Time.
 * 5. Glucose Concentration field, 4 bytes
 * 6. Type-Sample Location field, 1byte
 * 7. Sensor Status Annunciation field.
 * */
/* the Flags field struct: for example 0000 0111
 * bit0: time offset
 * bit1: Glucose Concentration Field and Type-Sample Location Field
 * bit2: the unit is in base units of mol/L
 *       (typically displayed in units of mmol/L)
 * bit3:Sensor Status Annunciation Field
 * bit4: context
 * */
#define STATUS_ANNUNCIATION_SUPPORTED       false
#define TIME_OFFSET_PRESENT                 0x01
#define CON_TYPE_SAMPLE_PRESENT             0x02
#define UINT_MOLL_PRESENT                   0x04
#define SENSOR_STATUS_PRESENT               0x08
#define MEASURE_CONTEXT_PRESENT             0x10
// Glucose measurement data
#if SL_BT_BGM_STATUS_ANNUNCIATION_SUPPORTED
extern uint8_t sl_glucose_records[MAX_RECORD_NUM][17];
#else
extern uint8_t sl_glucose_records[MAX_RECORD_NUM][15];
#endif

/* BGM has two filter types,
 * CGM only have one filter type: Time offset,
 */
#define FILTER_TYPE_SEQ_NUM            0x01
#define FILTER_TYPE_USER_FACING_TIME   0x02

/**@brief Glucose measurement context flags */
#define SL_BT_BGM_CONTEXT_CARB         0x01
#define SL_BT_BGM_CONTEXT_MEAL         0x02
#define SL_BT_BGM_CONTEXT_TESTER       0x04
#define SL_BT_BGM_CONTEXT_EXERCISE     0x08
#define SL_BT_BGM_CONTEXT_MED          0x10
#define SL_BT_BGM_CONTEXT_MED_KG       0x00  // used with SL_BT_BGM_CONTEXT_MED
#define SL_BT_BGM_CONTEXT_MED_L        0x20  // used with SL_BT_BGM_CONTEXT_MED
#define SL_BT_BGM_CONTEXT_HBA1C        0x40

/***************************************************************************//**
 * BGM - BGM Measurement
 * notification changed callback
 * Called when notification of BGM measurement is enabled/disabled by
 * the client.
 ******************************************************************************/
void sl_bt_bgm_measurement_notification_handler(sl_bt_msg_t *evt);

/**************************************************************************//**
 * BGM Measurement characteristic CCCD changed.
 *****************************************************************************/
void sl_bt_bgm_measurement_context_handler(sl_bt_msg_t *evt);

/** @} */ // end addtogroup measurement characteristic

/**
 * @addtogroup Record Access Control Point
 * @{
 *
 * @brief about all the Record Access Control Point
 *
 * inlcude defines, functions
 */
// Glusose abort operation - Report Stored Records
extern bool bgm_abort_operation_flag;
extern bool racp_enabled;

/*
 * RACP procedures include Op codes, operators and operand
 * operands include Filter Type and filter parameters
 */

/** @brief Record Access Control Point All opcodes*/
#define REPORT_STORED_RECORDS           0x01
#define DELETE_STORED_RECORDS           0x02
#define ABORT_OPERATION                 0x03
#define REPORT_NUMBER_OF_STORED_RECORDS 0x04
#define RSP_NUMBER_OF_STORED_RECORDS    0x05
#define RSP_CODE                        0x06
//

/** @brief Record Access Control Point All operators*/
#define OPERATOR_NULL                   0x00
#define OPERATOR_ALL_RECORDS            0x01
#define OPERATOR_LESS_OR_EQUAL          0x02
#define OPERATOR_GREATER_EQUAL          0x03
#define OPERATOR_WITHIN_RANGE           0x04
#define OPERATOR_FIRST                  0x05
#define OPERATOR_LAST                   0x06

#define RSP_CODE_SUCCEED                0x01
#define OPCODE_NOT_SUPPORTED            0x02
#define INVALID_OPERATOR                0x03
#define OPERATOR_NOT_SUPPORT            0x04
#define INVALID_OPERAND                 0x05
#define NO_RECORDS_FOUND                0x06
#define OPERAND_NOT_SUPPORTED           0x09
#define PROCEDURE_ALREADY_IN_PROCESSED  0x80 /** <Procedure Already In
                                              *   Progress*/

/**************************************************************************//**
 * handle RACP characteristic CCCD.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_bgm_racp_indication_handler(sl_bt_msg_t *evt);

/**************************************************************************//**
 * RACP event handler.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_bgm_racp_handler(sl_bt_msg_t *evt);

/**************************************************************************//**
 * the Server transmits an indication of the Record Access Control Point
 * characteristic
 * @param[in] connection handle
 * @param[in] opcode set to the corresponding Response Code
 * @param[in] error
 *****************************************************************************/
void sl_bt_bgm_send_racp_indication(uint8_t connection,
                                    uint8_t opcode, uint8_t error);

/**************************************************************************//**
 * Report Stored Records procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_bgm_report_record(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Report Stored Records – ‘All records’
 *****************************************************************************/
void sl_bt_bgm_report_all_records(uint8_t connection);

/**************************************************************************//**
 * Report Stored Records – ‘All records’ callback
 *****************************************************************************/
void sl_bt_signal_report_all_cb(void);

/**************************************************************************//**
 * [Report Stored Records – ‘Less than or equal to Time Offset’]
 *****************************************************************************/
void sl_bt_bgm_report_record_less_than(uint8_t connection, uint16_t maximum);

/**************************************************************************//**
 * [Report Stored Records – ‘Greater than or equal to Time Offset’]
 *****************************************************************************/
void sl_bt_bgm_report_records_greater_than(uint8_t connection,
                                           uint8_t filter_type, uint16_t low);

/**************************************************************************//**
 * Report Stored Records - Within range of Sequence Number value pair
 *****************************************************************************/
void sl_bt_bgm_report_records_within_range(uint8_t connection,
                                           uint8_t filter_type,
                                           uint16_t low,
                                           uint16_t high);

/**************************************************************************//**
 * [Report Stored Records – ‘First record’]
 *****************************************************************************/
void sl_bt_bgm_report_first_record(uint8_t connection);

/**************************************************************************//**
 * [Report Stored Records – ‘last record’]
 *****************************************************************************/
void sl_bt_bgm_report_last_record(uint8_t connection);

/**************************************************************************//**
 * Delete Stored Records procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_bgm_delete_record(sl_bt_msg_t *evt);

/**************************************************************************//**
 * [Delete Stored Records – ‘All records’]
 *****************************************************************************/
void sl_bt_bgm_delete_all_records(uint8_t connection);

/**************************************************************************//**
 * [Delete Stored Records - Within range of Sequence Number value pair]
 *****************************************************************************/
void sl_bt_bgm_delete_within_range_seq(uint8_t connection,
                                       uint16_t low, uint16_t high);

/**************************************************************************//**
 * Abort Operation procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_bgm_abort_operation(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Report Number of Stored Records procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_bgm_report_num_records(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Report Number of Stored Records – ‘All records’
 * @param[in] connection handle
 *****************************************************************************/
void sl_bt_bgm_report_num_all_records(uint8_t connection);

/**************************************************************************//**
* Report Number of Stored Records – ‘Greater than or equal to Time Offset’
******************************************************************************/
void sl_bt_bgm_report_greater_num_records(uint8_t connection, uint16_t high);
void sl_bt_bgm_report_within_range(sl_bt_msg_t *evt);

/** @} */ // end addtogroup Record Access Control Point

/**
 * @addtogroup  error handling functions
 * @{
 *
 * @brief  error handling functions
 *
 * A couple of  error handling functions.
 */

#define GATT_NOT_INDICATED             0x81  /** < CCCD Improperly Configured */
#define GATT_SUCCEED                   0x00
#define OPERAND_NOT_SUPPORT            0x09
#define INVALID_OPERATOR               0x03
#define UNSUPPORTED_OPERATOR           0x04
#define INVALID_OPERAND_TYPE_2         0x05
#define ATT_SUCCEED                    0x00
#define ATT_OUT_OF_RANGE               0xFF
#define ATT_INSUFFICIENT_AUTH          0x0F

void sl_bt_bgm_unsupported_opcode(uint8_t connection, uint8_t opcode);
void sl_bt_bgm_invalid_operator(uint8_t connection);
void sl_bt_bgm_unsupported_operator(uint8_t connection);
void sl_bt_bgm_invalid_operand_type1(uint8_t connection);
void sl_bt_bgm_invalid_operand_type2(uint8_t connection);
void sl_bt_bgm_unsupported_operand(uint8_t connection);

/** @} */ // end addtogroup error handling

#endif /* SL_BT_BGM_H_ */
