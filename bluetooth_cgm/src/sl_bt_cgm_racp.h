/***************************************************************************//**
 * @file
 * @brief Record Access Control Point characteristic behavior
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
#ifndef SL_BT_CGM_RACP_H_
#define SL_BT_CGM_RACP_H_

#include "sl_bt_cgm.h"

/**
 * @addtogroup Record Access Control Point
 * @{
 *
 * @brief about all the Record Access Control Point
 *
 * inlcude defines, functions
 */
/** @brief a CGM session is running and the client misses some CGM measurements
 * (e.g., due to link loss, or the CGM session is stopped),
 * the client may write to the Record Access Control Point characteristic to
 * request specific data from the patient record database, which triggers
 * immediate notifications of the CGM Measurement characteristic value.
 * This value is used only in report all records procedure(100ms), not the
 * measurement interval and communication interval
 */
#define REQUESTED_NOTIFICATION_INTERVAL 100
/** @brief Procedure Already In Progress, If a request with an Op Code other
 * than Abort Operation is written to the Control Point while the Server is
 * performing a previously triggered RACP operation (i.e., resulting from
 * invalid Client behavior), the Server shall return an error response with the
 * Attribute Protocol Application error code set to
 * Procedure Already In Progress */
extern bool procedure_in_progress;
/** @brief If the Op Code that was written to the Control Point characteristic
 * requests record notifications and the Client Characteristic Configuration
 * descriptor is not configured for notifications, the Server shall return an
 * error response with the Attribute Protocol Application error code set to
 * Client Characteristic Configuration Descriptor Improperly Configured.*/
extern bool racp_indicate_enabled;
/** @brief When the Abort Operation Op Code is written to the Record Access
 * Control Point, the Server shall stop any RACP procedures currently in
 * progress and shall make a best effort to stop sending any further data. */
extern bool abort_operation;

/*
 * RACP procedures command include Op codes, operators and operand
 * and operands include Filter Type and filter parameters
 */
/** @brief Record Access Control Point All opcodes*/
#define OP_NOT_SUPPORTED                0x00    /** < Op code not supported */
#define REPORT_STORED_RECORDS           0x01    /** < Report Stored Records */
#define DELETE_STORED_RECORDS           0x02    /** < Delete Stored Records */
#define ABORT_OPERATION                 0x03    /** < Abort operation
                                                      procedure */
#define REPORT_NUMBER_OF_STORED_RECORDS 0x04    /** < Report Number of Stored
                                                      Records */

// response data include opcode, opreator, operand
/** @brief Record Access Control Point(RACP) All response opcode*/
#define RSP_NOT_SUPPORT                 0x02    /** < Op code not supported */
#define RSP_NUMBER_OF_STORED_RECORDS    0x05    /** < Report Number of Stored
                                                      Records Response */
#define RSP_CODE_RACP                   0x06    /** < Response Code */

/** @brief Record Access Control Point All operators*/
#define OPERATOR_NULL                                                       0x00
#define OPERATOR_ALL_RECORDS                                                0x01
#define OPERATOR_LESS_OR_EQUAL                                              0x02
#define OPERATOR_GREATER_EQUAL                                              0x03
#define OPERATOR_WITHIN_RANGE                                               0x04
#define OPERATOR_FIRST                                                      0x05
#define OPERATOR_LAST                                                       0x06

#define RSP_CODE_NO_RECORD_FOUND        0x06    /** < used in RAR/BV-03-C*/
#define RSP_CODE_SUCCEED                0x01

/**************************************************************************//**
 * RACP event handler.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_racp_handler(sl_bt_msg_t *evt);
/**************************************************************************//**
 * handle RACP characteristic CCCD.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_racp_indication_handler(sl_bt_msg_t *evt);
/**************************************************************************//**
 * the Server transmits an indication of the Record Access Control Point
 * characteristic
 * @param[in] opcode set to the corresponding Response Code
 * @param[in] error
 *****************************************************************************/
void sl_bt_cgm_send_racp_indication(uint8_t opcode, uint8_t error);

/**************************************************************************//**
 * Report Number of Stored Records procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_report_num_records(sl_bt_msg_t *evt);
/**************************************************************************//**
 * Report Number of Stored Records – ‘All records’
 * @param[in] connection handle
 *****************************************************************************/
void sl_bt_cgm_report_num_all_records(uint8_t connection);
/**************************************************************************//**
 * Report Number of Stored Records – ‘Greater than or equal to Time Offset’
 * @param[in] evt Event coming from the Bluetooth stack.
 ******************************************************************************/
void sl_bt_cgm_report_num_records_greater(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Delete Stored Records procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_delete_record(sl_bt_msg_t *evt);
/**************************************************************************//**
 * [Delete Stored Records – ‘All records’]
 *****************************************************************************/
void sl_bt_cgm_delete_all_records(void);
/**************************************************************************//**
 * [Delete Stored Records–‘Within range of (inclusive) Time Offset value pair’]
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_delete_records_within_range(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Report Stored Records procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_report_record(sl_bt_msg_t *evt);
/**************************************************************************//**
 * Report Stored Records – ‘All records’
 *****************************************************************************/
void sl_bt_cgm_report_all_records(void);
/**************************************************************************//**
 * [Report Stored Records – ‘Less than or equal to Time Offset’]
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_report_records_less_than(sl_bt_msg_t *evt);
/**************************************************************************//**
 * [Report Stored Records – ‘Greater than or equal to Time Offset’]
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_report_records_greater_than(sl_bt_msg_t *evt);
/**************************************************************************//**
 * [Report Stored Records –‘Within range of (inclusive) Time Offset value pair’]
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_report_records_within_range(sl_bt_msg_t *evt);
/**************************************************************************//**
 * [Report Stored Records – ‘First record’]
 *****************************************************************************/
void sl_bt_cgm_report_first_record();
/**************************************************************************//**
 * [Report Stored Records – ‘last record’]
 *****************************************************************************/
void sl_bt_cgm_report_last_record();

/**************************************************************************//**
 * Abort Operation procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_abort_operation(sl_bt_msg_t *evt);

/** @} */ // end addtogroup Record Access Control Point



#endif /* SL_BT_CGM_RACP_H_ */
