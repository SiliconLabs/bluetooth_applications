/***************************************************************************//**
 * @file
 * @brief CGM session start time characteristic,
 *        session run time characteristic,
 *        status characteristic,
 *        feature characteristic.
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
#ifndef SL_BT_CGM_CHARACTERISTIC_H_
#define SL_BT_CGM_CHARACTERISTIC_H_


/**
 * @addtogroup other characteristics
 * @{
 *
 * @brief other characteristics read/write
 *
 */

/* The CGM Status allows the client to actively request the status from the CGM
 * Sensor, particularly when the CGM measurement is not running and the status
 * cannot be given in the measurement result in the Status Annunciation Field.*/
/** @brief status struct:
 *         time offset 2 octets, status 3 octets, CRC 2 octets*/
#define STATUS_LEN 7
#define SL_BT_CGM_STATUS_DEVICE_SEPCIFIC_ALERT  (0x01 << 4)
#define SL_BT_CGM_STATUS_SESSION_STOPPED        (0x01 << 0)
extern uint8_t cgm_status[];

/** @brief
 * The CGM Feature characteristic is used to describe the supported features
 * of the Server.
 * feature field struct:
 *         Feature 3 bytes,
 *         Type-Sample Location Field 1 byte,
 *         CRC 2 bytes.*/
extern uint8_t feature[];
#define FEATURE_LEN 6
/** @brief
 * In a multiple-bond case, the handling of the Control Point shall be
 * consistent across all bonds
 * (i.e., there is a single database that is shared by all clients).
 * */
#define SL_BT_CGM_MULTI_BOND_SUPPORTED                                         0
#define SL_BT_CGM_MULTI_SESSION_SUPPORTED                                      1
#define SL_BT_CGM_E2E_CRC                                           (0x01 << 12)
#define SL_BT_CGM_MULTI_BOND                                        (0x01 << 13)
#define SL_BT_CGM_MULTI_SESSION                                     (0x01 << 14)
#define SL_BT_CGM_SAMPLE_LOCATION                                           0x02
#define SL_BT_CGM_TYPE                                                      0x03
#define SL_BT_CGM_TREND_INFO_SUPPORTED                                         0
#define SL_BT_CGM_QUALITY_SUPPORTED                                            0
// Session start time
#define SL_BT_CGM_SST_LEN                                                      9
#define SL_BT_CGM_SST_WITHCRC_LEN                                             11
/**************************************************************************//**
 * initialize CGM database
 *****************************************************************************/
void sl_bt_cgm_init_database(void);
/**************************************************************************//**
 * write CGM Session Start Time
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_handle_sst_write(sl_bt_msg_t *evt);
/**************************************************************************//**
 * read CGM Session Start Time
 *****************************************************************************/
void sl_bt_cgm_handle_sst_read(void);
/**************************************************************************//**
 * read CGM status
 *****************************************************************************/
void sl_bt_cgm_handle_status_read(void);
/**************************************************************************//**
 * read CGM feature characteristic
 *****************************************************************************/
void sl_bt_cgm_handle_feature_read(void);
/**************************************************************************//**
 * read CGM Session run Time
 *****************************************************************************/
void sl_bt_cgm_handle_run_time_read(void);

/** @} */ // end addtogroup other characteristics



#endif /* SL_BT_CGM_CHARACTERISTIC_H_ */
