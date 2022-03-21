/***************************************************************************//**
 * @file
 * @brief CGM Measurement characteristic behavior
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

#ifndef SL_BT_CGM_MEASUREMENT_H_
#define SL_BT_CGM_MEASUREMENT_H_

/**
 * @addtogroup measurement characteristic functions
 * @{
 *
 * @brief about all the measurement characteristic
 *
 */

/** @brief If a CGM session is running, the CGM Sensor measures the glucose
 *         level periodically in a device specific interval
 *         (Measurement time interval).
 *         default 1 minute(60s) */
#define MEASUREMENT_TIME_INTERVAL        60
/** @brief Glucose measurement characteristic notification enable
 *  Client Characteristic Configuration Descriptor Configured flag,
 *         only when CGM Measurement characteristic notification is enabled,
 *         the Lower Tester can start the procedure.
*/
extern bool measure_notification_enabled;

/* CGM measurement data struct:
 * 1. length: 1 byte
 * 2. flags: 1 byte
 * 3. Glucose Concentration: 2 bytes,
 * 4. time offset: relative time
 * 5. sensor_status_annunciation: 3 bytes optional
 * 6. trend information field: 2 bytes optional
 * 7. quality: 2 bytes optional
 * 8. CRC: 2 bytes optional
 * there is no Base Time field in CGM as BGM, so there is session start time
 * field. */
/* The Time Offset field is defined as a UINT 16 according to the byte
 * transmission order, defined in Section 1.6, representing the number of
 * minutes the user-facing time differs from the Session Start Time.
 * The default initial value shall be 0x0000. The Time Offset field shall be
 * incremented by the measurement interval with each CGM measurement record*/

/* CGM Measurement flags Field
1. bit0-Trend information
2. bit1 Quality
3. bit5-Warning
4. bit6-Cal/Temp
5. bit7-status */
/** @brief If the device supports CGM Trend information, the CGM Trend
 * Information supported bit in the CGM Features characteristic shall be
 * set to 1. The CGM Measurement record shall contain the CGM Trend Information
 * Field when the value of bit 0 in the Flags Field is set to 1. If set to 0
 * the CGM Measurement record shall not contain a CGM Trend Information Field.
 * bit0
 * */
#define SL_BT_CGM_FLAG_TREND_INFO_PRESENT                                   0x01
/** @brief If the CGM Quality feature is supported, the CGM Quality Supported
 * Feature bit shall be set to 1, otherwise it shall be set to 0.
 * If CGM Quality is supported, the CGM Measurement characteristic may contain
 * a CGM Quality field. bit 1*/
#define SL_BT_CGM_FLAGS_QUALITY_PRESENT                                     0x02
/** @brief The Sensor Status Annunciation field may form part of the CGM
 * Measurement record. This field has a variable length between 1 and 3 octets.
 * If one or more bits in the Sensor Status Annunciation field are set to “1”
 * the Sensor Status Annunciation field shall form part of every CGM Measurement
 * Record to which it applies.*/
// bit 5
#define SL_BT_CGM_STATUS_ANNUNCIATION_WARNING_PRESENT                       0x20
// bit 6
#define SL_BT_CGM_STATUS_ANNUNCIATION_CALTEMP_PRESENT                       0x40
// bit 7
#define SL_BT_CGM_STATUS_ANNUNCIATION_STATUS_PRESENT                        0x80

/**************************************************************************//**
 * Measurement characteristic event handler.
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_measurement_notification_handler(sl_bt_msg_t *evt);
/**************************************************************************//**
 * If a CGM session is running, the CGM Sensor measures the glucose level
 * periodically in a device specific interval (Measurement time interval).
 * When the CGM Communication Interval is set for periodic communication,
 * the CGM Sensor periodically sends notifications of the most recent CGM
 * measurements that have occurred since the last communication interval
 * notification.
 * @param[in] timer void
 * @param[in] data void
 *****************************************************************************/
void sl_bt_cgm_periodic_timer_cb(sl_simple_timer_t *timer, void *data);
/**************************************************************************//**
 * requested notification: Measurement characteristic send notification.
 *****************************************************************************/
void sl_bt_cgm_measurement_notificate(uint16_t index);

/** @} */ // end addtogroup measurement characteristic functions




#endif /* SL_BT_CGM_MEASUREMENT_H_ */
