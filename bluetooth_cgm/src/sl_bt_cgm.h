/***************************************************************************//**
 * @file
 * @brief CGM header file
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
#ifndef SL_BT_CGM_H_
#define SL_BT_CGM_H_

#include "stdbool.h"
#include "gatt_db.h"
#include "sl_status.h"
#include "sl_bt_api.h"
#include "app_log.h"
#include "app_assert.h"
#include "sl_simple_timer.h"

// Macros.
#define UINT16_TO_BYTES(n)            ((uint8_t) (n)), ((uint8_t)((n) >> 8))
#define UINT16_TO_BYTE0(n)            ((uint8_t) (n))
#define UINT16_TO_BYTE1(n)            ((uint8_t) ((n) >> 8))
#define BYTES_TO_UINT16(n,m)          (n | m << 8)
/** @brief Glucose measurement current record number*/
extern uint16_t records_num;
#define MAX_CALIBRATION_NUM 10
/**
 * @addtogroup bluetooth
 * @{
 *
 * @brief bluetooth related
 *
 */
/** @brief switch this to true, it will expose all the IADC result
 * with the IADC service*/
#define CGM_ADC_PRESENT true
/** @brief current connection bonding handle*/
/* the Bond Management Service is Mandatory if multiple bonds are supported
 * by server, otherwise optional.
 * */
extern uint8_t bonding;
/** @brief current connection handle*/
extern uint8_t connection;
/** @brief AD type value when create customer advertisement*/
#define AD_PUBLIC_ADDRESS   0x17
#define AD_RANDOM_ADDRESS   0x18

/** @brief oneshot timer
Advertising Duration                 Parameter              Value
First 30 seconds (fast connection)   Advertising Interval   30 ms to 300 ms
After 30 seconds (reduced power)     Advertising Interval   1 s to 10.24 s
*/
#define SL_CGM_FAST_ADV_TIMEOUT                                          30*1000
/**************************************************************************//**
 * Bluetooth stack event handler.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_on_event(sl_bt_msg_t *evt);
/**************************************************************************//**
 * CRC padding at the end of the data
 * @param[in] crc CRC-CCITT generator polynomial g(D)=D16+D12+D5+1
 *            (i.e., 210041 in octal representation) with a seed of 0xFFFF
 * @param[in] data input to generate crc
 * @param[in] len the length of the input data
 * @return crc data in uint16 2 bytes
 *****************************************************************************/
uint16_t sli_bt_cgm_crc16(uint16_t crc, uint8_t *data, size_t len);
/** @brief
 * If the device supports E2E-safety (E2E-CRC Supported bit is set in
 * CGM Features), the measurement shall be protected by a CRC calculated
 * over all fields
 * */
#define SL_BT_CGM_E2E_CRC_SUPPORTED 0
/** @} */ // end addtogroup bluetooth

/**
 * @addtogroup ERROR
 * @{
 *
 * @brief error handling
 *
 */

#define GATT_NOT_INDICATED             0xFD  /** < CCCD Improperly Configured */
#define PROCEDURE_ALREADY_IN_PROCESSED 0xFE /** <Procedure Already In Progress*/
#define GATT_SUCCEED                   0x00
#define OPERAND_NOT_SUPPORT            0x09
#define INVALID_OPERATOR               0x03
#define UNSUPPORTED_OPERATOR           0x04
#define INVALID_OPERAND_TYPE_2         0x05
#define ATT_SUCCEED                    0x00
#define ATT_MISSING_CRC                0x80
#define ATT_INVALID_CRC                0x81
#define ATT_OUT_OF_RANGE               0xFF

/** @} */ // end addtogroup ERROR

#endif /* CGM_H_ */

