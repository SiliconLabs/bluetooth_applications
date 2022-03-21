/***************************************************************************//**
 * @file
 * @brief CGM Specific Ops Control Point characteristic behavior
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
#ifndef SL_BT_CGM_SOPS_H_
#define SL_BT_CGM_SOPS_H_

#include "sl_bt_cgm.h"

/**
 * @addtogroup Specific Ops Control Point
 * @{
 *
 * @brief about all the Specific Ops Control Point
 *
 * include defines, functions
 */

typedef struct
{
  uint16_t high_alert_value;
  uint16_t low_alert_value;
  uint16_t hypo_alert_level;
  uint16_t hyper_alert_level;
  uint16_t rate_decrease_alert_value;
  uint16_t rate_increase_alert_value;
}sl_bt_cgm_init_value_t;
extern sl_bt_cgm_init_value_t cgm_init;

typedef struct
{
    uint16_t  concentration;  /**< Glucose Concentration mg/dL. */
    uint16_t  cal_time;           /**< Calibration Time minutes. */
    uint8_t   sample_location;/**< Sample Location */
    uint16_t  next_cal;       /**< Next Calibration minutes */
    uint16_t  record_num;     /**< Calibration Data Record Number */
    uint8_t   status;         /**< Calibration Status */
} sl_bt_cgms_cal_value_t;


extern sl_bt_cgms_cal_value_t calibration_value[];


/** @brief The Measurement Interval is the time interval
 * between two CGM Measurement notifications.
 * Default: 2 minutes
 * can be changed by CGM Specific Ops – ‘Set CGM Communication Interval’
 * There are two ways that a client can receive notifications of the
 * CGM Measurement Characteristic value: periodic and requested.
 * this is periodic notification */
#define GLUCOSE_DEFAULT_COMM_INTERVAL_SEC   2
extern uint8_t periodic_comm_interval;
extern sl_simple_timer_t cgm_periodic_timer;

/** @brief Start Session procedure
 * the Server shall calculate and store the Session Start Time using the time
 * of the client and its own current relative time value.
 * The Session Start procedure will clear the CGM measurement database,
 * so the Session Start Time characteristic is cleared too.
 * the session stop also clear the database
 * When the Op Code for Start Session is written to the CGM Specific
 * Ops Control Point, the sensor deletes all data from the previous session
 * and starts the measurement.
 **/
extern bool session_started;

/** @brief Specific Ops Control Point All opcodes*/
#define SET_CGM_COMM_INTEVAL              0x01 /** < Set CGM Communication
                                                     Interval */
#define GET_CGM_COMM_INTEVAL              0x02 /** < Get CGM Communication
                                                     Interval */
#define GET_CGM_COMM_INTEVAL_RSP          0x03 /** < Get CGM Communication
                                                     Interval response opcode*/
#define SET_CAL_VALUE                     0x04 /** < Set Glucose Calibration
                                                     value */
#define GET_CAL_VALUE                     0x05 /** < Get Glucose Calibration
                                                     value */
#define GET_CAL_VALUE_RSP                 0x06 /** < Get Glucose Calibration
                                                     value response*/
#define SET_PATIENT_HIGH_ALERT_VALUE      0x07 /** < Set Patient High Alert
                                                     Level */
#define GET_PATIENT_HIGH_ALERT_VALUE      0x08 /** < Get Patient High Alert
                                                     Level */
#define GET_PATIENT_HIGH_ALERT_VALUE_RSP  0x09 /** < Get Patient High Alert
                                                     Level response code*/
#define SET_PATIENT_LOW_ALERT_VALUE       0x0A /** < Set Patient Low Alert
                                                     Level */
#define GET_PATIENT_LOW_ALERT_VALUE       0x0B /** < Get Patient Low Alert
                                                     Level */
#define GET_PATIENT_LOW_ALERT_VALUE_RSP   0x0C /** < Get Patient Low Alert
                                                     Level response code*/
#define SET_HYPO_ALERT_VALUE              0x0D /** < Set Hypo Alert Level */
#define GET_HYPO_ALERT_LEVEL              0x0E /** < Get Hypo Alert Level */
#define GET_HYPO_ALERT_LEVEL_RSP          0x0F /** < Get Hypo Alert Level
                                                     response opcode*/
#define SET_HYPER_ALERT_LEVEL             0x10 /** < Set Hyper Alert Level */
#define GET_HYPER_ALERT_LEVEL             0x11 /** < Set Hyper Alert Level */
#define GET_HYPER_ALERT_LEVEL_RSP         0x12 /** < Get Hyper Alert Level RSP*/
#define SET_RATE_DECREASE_ALERT_LEVEL     0x13 /** < Set Rate of Decrease Alert
                                                     Level */
#define GET_RATE_DECREASE_ALERT_LEVEL     0x14 /** < Get Rate of Decrease Alert
                                                     Level */
#define GET_RATE_DECREASE_ALERT_LEVEL_RSP 0x15 /** < Get Rate of Decrease Alert
                                                     Level response */
#define SET_RATE_INCREASE_ALERT_LEVEL     0x16 /** < Set Rate of Increase Alert
                                                     Level */
#define GET_RATE_INCREASE_ALERT_LEVEL     0x17 /** < Get Rate of Increase Alert
                                                     Level */
#define GET_RATE_INCREASE_ALERT_LEVEL_RSP 0x18 /** < Get Rate of Increase Alert
                                                     Level response*/
#define RESET_DEVICE_SPECIFIC_ALERT       0x19 /** < Reset Device Specific
                                                     Alert */
#define START_THE_SESSION                 0x1A /** < start session */
#define STOP_THE_SESSION                  0x1B /** < stop session */
/** @brief Specific Ops Control Point All response opcodes*/
#define RSP_SUCCESS                       0x01 /** < response success */
#define RSP_PROCEDURE_NOT_COMPLETED       0x04 /** < Procedure not completed */
#define RSP_PARAMETER_OUT_OF_RANGE        0x05
#define RSP_CODE_SOPS                     0x1C

/**************************************************************************//**
 * handle SOPS CCCD
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_sops_indication_handler(sl_bt_msg_t *evt);
/**************************************************************************//**
 * handle SOPS operation procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_handle_sops(sl_bt_msg_t *evt);
/**************************************************************************//**
 * indicate the CGM Specific Ops Control Point with a Response Code Op Code
 * and a Response Value.
 * @param[in] opcode
 * @param[in] response_code
 *****************************************************************************/
void sl_bt_cgm_send_sops_indicate(uint8_t opcode, uint8_t response_code);
/**************************************************************************//**
 * 3.7.2.1 CGM get Communication Interval procedures
 *****************************************************************************/
void sl_bt_cgm_sops_get_comm_interval(void);
/**************************************************************************//**
 * 3.7.2.1 set CGM Communication Interval procedures
 * @param[in] interval the Server shall change its communication interval time
 *            according to this interval
 *****************************************************************************/
void sl_bt_cgm_set_comm_interval(uint8_t interval);
/**************************************************************************//**
 * 3.7.2.2 get Glucose Calibration procedures
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_get_cal_value(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.2 set Glucose Calibration procedures
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_set_cal_value(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.3 get Patient High/Low Alert Level procedures
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_get_alert_level(sl_bt_msg_t *evt);
/**************************************************************************//**
 * set Patient High Alert Level procedures
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_set_high_alert_level(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.3 set Patient Low Alert Level procedures
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_set_low_alert_level(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.4 set Hypo Alert procedures, get Hypo Alert is handled in
 * sl_bt_cgm_handle_sops
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_set_hypo_alert_level(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.5 set Hyper Alert procedures, get Hyper Alert is handled in
 * sl_bt_cgm_handle_sops
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_set_hyper_alert_level(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.6 set Rate of Decrease Alert Level procedures
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_set_rate_decrease_alert_level(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.6 set Rate of Increase Alert Level procedures
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_set_rate_increase_alert_level(sl_bt_msg_t *evt);
/**************************************************************************//**
 * 3.7.2.7 Reset Device Specific Alert procedure
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_cgm_reset_device_special_alert(void);

/** @} */ // end addtogroup Specific Ops Control Point



#endif /* SL_BT_CGM_SOPS_H_ */
