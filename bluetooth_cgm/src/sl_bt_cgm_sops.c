/***************************************************************************//**
 * @file
 * @brief CGM specific Ops characteristic
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
#include "sl_bt_cgm_sops.h"
#include "sl_bt_cgm_characteristic.h"

/* When the Op Code for Start Session is written to the CGM Specific
 * Ops Control Point, the sensor deletes all data from the previous session
 * and starts the measurement.*/
bool session_started = false;

/**@brief Glucose Calibration value */
#define CGM_CAL_VALUE_LEN 12

sl_bt_cgms_cal_value_t calibration_value[MAX_CALIBRATION_NUM] = \
                         {{0x004F, 0x0005, 0x06, 0x0005, 0x0000, 0x00}, {0x00}};
uint16_t cal_num = 0;

typedef struct
{
  uint8_t op_code;
  uint8_t operand;
  uint8_t response_code;
}sl_bt_cgm_sops_t;

typedef struct
{
  uint8_t opcode;
  sl_bt_cgms_cal_value_t value;
}sl_bt_cgms_cal_value_rsp_t;

/* init CGM SOPS value*/
sl_bt_cgm_init_value_t cgm_init = {130, 100, 60, 200, 4, 4};

/* retrieving data */
void cgm_dump_cal_value(uint8_t sl_cgm_cal_value[], uint16_t index)
{
  sl_cgm_cal_value[0] = 0x06;
  sl_cgm_cal_value[1] = calibration_value[index].concentration & 0xff;
  sl_cgm_cal_value[2] = calibration_value[index].concentration >> 8;
  sl_cgm_cal_value[3] = calibration_value[index].cal_time & 0xff;
  sl_cgm_cal_value[4] = calibration_value[index].cal_time >> 8;
  sl_cgm_cal_value[5] = calibration_value[index].sample_location;
  sl_cgm_cal_value[6] = calibration_value[index].next_cal & 0xff;
  sl_cgm_cal_value[7] = calibration_value[index].next_cal >> 8;
  sl_cgm_cal_value[8] = calibration_value[index].record_num & 0xff;
  sl_cgm_cal_value[9] = calibration_value[index].record_num >> 8;
  sl_cgm_cal_value[10] = calibration_value[index].status;
  if (SL_BT_CGM_E2E_CRC_SUPPORTED){
      uint16_t crc = sli_bt_cgm_crc16(0xFFFF, sl_cgm_cal_value, 11);
      sl_cgm_cal_value[11] = UINT16_TO_BYTE0(crc);
      sl_cgm_cal_value[12] = UINT16_TO_BYTE1(crc);
  }else{
    sl_cgm_cal_value[11] = 0xFF;
    sl_cgm_cal_value[12] = 0xFF;
  }
}

/**************************************************************************//**
 * 4.15.0 CGMS/SEN/CGMCP/BV-02-C [CGM Specific Ops –
 * ‘Get CGM Communication Interval with/without E2E-CRC’]
 *****************************************************************************/
void sl_bt_cgm_sops_get_comm_interval(void)
{
   uint8_t rsp[] = {GET_CGM_COMM_INTEVAL_RSP,0x00,0xff,0xff};
   rsp[1] = periodic_comm_interval;
   if (SL_BT_CGM_E2E_CRC_SUPPORTED){
     uint16_t crc = sli_bt_cgm_crc16(0xFFFF, rsp, 2);
     rsp[2] = UINT16_TO_BYTE0(crc);
     rsp[3] = UINT16_TO_BYTE1(crc);
   }
   sl_status_t sc;
   sc = sl_bt_gatt_server_send_indication(
     connection,
     gattdb_cgm_specific_ops_control_point,
     sizeof(rsp),
     rsp
     );
   if(sc != SL_STATUS_OK){
     app_log("sl_bt_cgm_sops_get_comm_interval 0x%04X\n", sc);
   }
}

/**************************************************************************//**
 * 4.15.1 CGM Specific Ops – ‘Set CGM Communication Interval’
 *****************************************************************************/
void sl_bt_cgm_set_comm_interval(uint8_t interval)
{
  sl_status_t sc;
  //4.15.3.0 CGM Specific Ops – ‘Disable communication interval’
  if(interval == 0){
    app_log("interval is 0, disable the periodic communication\n");
    periodic_comm_interval = 0;
    if(session_started == true){
      session_started = false;
      sc = sl_simple_timer_stop(&cgm_periodic_timer);
      if(sc != SL_STATUS_OK){
        app_log("sl_simple_timer_stop failed 0x%04X\n", sc);
      }
    }
  //4.15.2 CGM Specific Ops –
  //‘Set smallest CGM Communication Interval supported by Device’
  }else if(interval == 0xFF){
    app_log("interval is 0xFF, change the comm interval to smallest.\n");
    periodic_comm_interval = GLUCOSE_DEFAULT_COMM_INTERVAL_SEC;
  }else{
    app_log("set CGM communication interval %d\n", interval);
    periodic_comm_interval = interval;
  }
}

/**************************************************************************//**
 * 4.15.3.1 CGMS/SEN/CGMCP/BV-06-C
 * [CGM Specific Ops – ‘Get Glucose Calibration Value’ Type 1]
 *****************************************************************************/
void sl_bt_cgm_get_cal_value(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t index = array[1] | array[2] << 8;
  //4.15.3.2 CGMS/SEN/CGMCP/BV-07-C [CGM Specific Ops –
  //‘Get Glucose Calibration Value’ Type 2]
  if(index == 0xFFFF){
    app_log("get latest calibration data %d\n", cal_num);
    uint8_t sl_cgm_cal_value[1 + CGM_CAL_VALUE_LEN];
    cgm_dump_cal_value(sl_cgm_cal_value, cal_num);
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_indication(
          connection,
          gattdb_cgm_specific_ops_control_point,
          sizeof(sl_cgm_cal_value) - 2,
          sl_cgm_cal_value
          );
        if(sc != SL_STATUS_OK){
          app_log("sl_bt_cgm_get_cal_value 0x%04X\n", sc);
        }
  //4.15.3.3 CGMS/SEN/CGMCP/BI-01-C [CGM Specific Ops –
  //‘Get Glucose Calibration Value’ Type 3]
  }else if(index == 0xFFFE){
    //Response Code for ‘Parameter out of Range’ (0x05).
    sl_bt_cgm_send_sops_indicate(GET_CAL_VALUE, RSP_PARAMETER_OUT_OF_RANGE);
  }else{
    sl_status_t sc;
    uint8_t sl_cgm_cal_value[1 + CGM_CAL_VALUE_LEN];
    cgm_dump_cal_value(sl_cgm_cal_value, index);
    sc = sl_bt_gatt_server_send_indication(
                                          connection,
                                          gattdb_cgm_specific_ops_control_point,
                                          sizeof(sl_cgm_cal_value) - 2,
                                          sl_cgm_cal_value
                                          );
    if(sc != SL_STATUS_OK){
      app_log("sl_bt_cgm_get_cal_value 0x%04X\n", sc);
    }
  }
}

/**************************************************************************//**
 * 4.15.3.4 CGMS/SEN/CGMCP/BV-08-C
 * [CGM Specific Ops – ‘Set Glucose Calibration value’]
 *****************************************************************************/
void sl_bt_cgm_set_cal_value(sl_bt_msg_t *evt)
{
  cal_num ++;
  if(cal_num > MAX_CALIBRATION_NUM){
    app_log("out of calibration value store memory\n");
    return;
  }
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t record_num = (array[8] | array[9] << 8);
  calibration_value[record_num].concentration = array[1] | array[2] << 8;
  calibration_value[record_num].cal_time = array[3] | array[4] << 8;
  calibration_value[record_num].sample_location = array[5];
  calibration_value[record_num].next_cal = array[6] | array[7] << 8;
  calibration_value[record_num].record_num = array[8] | array[9] << 8;
  calibration_value[record_num].status = array[10];
  sl_bt_cgm_send_sops_indicate(SET_CAL_VALUE, RSP_SUCCESS);
}

/**************************************************************************//**
 * 4.15.3.5 CGMS/SEN/CGMCP/BV-09-C
 * [CGM Specific Ops – ‘Get Patient High Alert Level’]
 * 4.15.3.8 CGMS/SEN/CGMCP/BV-11-C
 * [CGM Specific Ops – ‘Get Patient Low Alert Level’]
 *****************************************************************************/
void sl_bt_cgm_get_alert_level(sl_bt_msg_t *evt)
{
  uint8_t sl_sops_indication_reply[] = {0x00,0x00,0x00};
  //4.15.3.5
  if(evt->data.evt_gatt_server_attribute_value.value.data[0] == 0x08){
    sl_sops_indication_reply[0] = GET_PATIENT_HIGH_ALERT_VALUE_RSP;
    sl_sops_indication_reply[1] = cgm_init.high_alert_value & 0xff;
    sl_sops_indication_reply[2] = cgm_init.high_alert_value >> 8;
  //4.13.3.8
  }else if(evt->data.evt_gatt_server_attribute_value.value.data[0] == 0x0B){
      sl_sops_indication_reply[0] = GET_PATIENT_LOW_ALERT_VALUE_RSP;
      sl_sops_indication_reply[1] = cgm_init.low_alert_value & 0xff;
      sl_sops_indication_reply[2] = cgm_init.low_alert_value >> 8;
      app_log("low is %d, first is %d, second is %d\n", \
              cgm_init.low_alert_value, sl_sops_indication_reply[1],\
              sl_sops_indication_reply[2]);
  }
  sl_status_t sc;
  sc = sl_bt_gatt_server_send_indication(
    connection,
    gattdb_cgm_specific_ops_control_point,
    sizeof(sl_sops_indication_reply),
    sl_sops_indication_reply
    );
  if(sc != SL_STATUS_OK){
    app_log("sl_bt_cgm_get_alert_level 0x%04X, connection %d\n",\
             sc, connection);
    while(sc == SL_STATUS_NO_MORE_RESOURCE){
      sc = sl_bt_gatt_server_send_indication(
        connection,
        gattdb_cgm_specific_ops_control_point,
        sizeof(sl_sops_indication_reply),
        sl_sops_indication_reply
        );
    }
  }
}

/**************************************************************************//**
 * 4.15.3.6 CGMS/SEN/CGMCP/BV-10-C
 * [CGM Specific Ops – ‘Set Patient High Alert Level’]
 * 4.15.3.7 CGMS/SEN/CGMCP/BI-02-C
 * [CGM Specific Ops – ‘Set invalid Patient High Alert Level’]
 *****************************************************************************/
void sl_bt_cgm_set_high_alert_level(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t high_alert_value = array[1] | array[2] << 8;
  //4.13.3.7
  if(high_alert_value > 0x700){
    sl_bt_cgm_send_sops_indicate(SET_PATIENT_HIGH_ALERT_VALUE,
                                 RSP_PARAMETER_OUT_OF_RANGE);
  //4.13.3.6
  }else{
    cgm_init.high_alert_value = high_alert_value;
    sl_bt_cgm_send_sops_indicate(SET_PATIENT_HIGH_ALERT_VALUE, RSP_SUCCESS);
  }
}

/**************************************************************************//**
 * 4.15.3.9 CGMS/SEN/CGMCP/BV-12-C
 * [CGM Specific Ops – ‘Set Patient Low Alert Level’]
 * 4.15.3.10 CGMS/SEN/CGMCP/BI-03-C
 * [CGM Specific Ops – ‘Set invalid Patient Low Alert Level’]
 *****************************************************************************/
void sl_bt_cgm_set_low_alert_level(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t low_alert_value = array[1] | array[2] << 8;
  //4.15.3.10
  if(low_alert_value > 0x700){
    sl_bt_cgm_send_sops_indicate(SET_PATIENT_LOW_ALERT_VALUE,
                                 RSP_PARAMETER_OUT_OF_RANGE);
  //4.15.3.9
  }else{
    cgm_init.low_alert_value = low_alert_value;
    sl_bt_cgm_send_sops_indicate(SET_PATIENT_LOW_ALERT_VALUE, RSP_SUCCESS);
  }
}

/**************************************************************************//**
 * 4.15.3.12 CGMS/SEN/CGMCP/BV-14-C [CGM Specific Ops – ‘Set Hypo Alert Level’]
 * 4.15.3.13 CGMS/SEN/CGMCP/BI-04-C
 * [CGM Specific Ops – ‘Set invalid Hypo Alert Level’]
 *****************************************************************************/
void sl_bt_cgm_set_hypo_alert_level(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t hypo_alert_level = array[1] | array[2] << 8;
  //4.15.3.13
  if(hypo_alert_level > 100){
    sl_bt_cgm_send_sops_indicate(SET_HYPO_ALERT_VALUE,
                                 RSP_PARAMETER_OUT_OF_RANGE);
  }else{
    //4/15/3/12
    cgm_init.hypo_alert_level = hypo_alert_level;
    sl_bt_cgm_send_sops_indicate(SET_HYPO_ALERT_VALUE, RSP_SUCCESS);
  }
}

/**************************************************************************//**
 * 4.15.3.15 CGMS/SEN/CGMCP/BV-16-C
 * [CGM Specific Ops – ‘Set Hyper Alert Level’]
 * 4.15.3.16 CGMS/SEN/CGMCP/BI-05-C
 * [CGM Specific Ops – ‘Set invalid Hyper Alert Level’]
 *****************************************************************************/
void sl_bt_cgm_set_hyper_alert_level(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t hyper_alert_level = array[1] | array[2] << 8;
  if(hyper_alert_level > 1000){
    sl_bt_cgm_send_sops_indicate(SET_HYPER_ALERT_LEVEL,
                                 RSP_PARAMETER_OUT_OF_RANGE);
  }else{
    cgm_init.hyper_alert_level = hyper_alert_level;
    sl_bt_cgm_send_sops_indicate(SET_HYPER_ALERT_LEVEL, RSP_SUCCESS);
  }
}

/**************************************************************************//**
 * 4.15.3.18 CGMS/SEN/CGMCP/BV-18-C
 * [CGM Specific Ops – ‘Set Rate of Decrease Alert Level’]
 * 4.15.3.19 CGMS/SEN/CGMCP/BI-06-C
 * [CGM Specific Ops – ‘Set invalid Rate of Decrease Alert Level’]
 *****************************************************************************/
void sl_bt_cgm_set_rate_decrease_alert_level(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t level = array[1] | array[2] << 8;
  if(level > 100){
    sl_bt_cgm_send_sops_indicate(SET_RATE_DECREASE_ALERT_LEVEL,
                                 RSP_PARAMETER_OUT_OF_RANGE);
  }else{
    cgm_init.rate_decrease_alert_value = level;
    sl_bt_cgm_send_sops_indicate(SET_RATE_DECREASE_ALERT_LEVEL, RSP_SUCCESS);
  }
}

/**************************************************************************//**
 * 4.15.3.21 CGMS/SEN/CGMCP/BV-20-C
 * [CGM Specific Ops – ‘Set Rate of Increase Alert Level’]
 * 4.15.3.22 CGMS/SEN/CGMCP/BI-07-C
 * [CGM Specific Ops – ‘Set invalid Rate of Increase Alert Level’]
 *****************************************************************************/
void sl_bt_cgm_set_rate_increase_alert_level(sl_bt_msg_t *evt)
{
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint16_t level = array[1] | array[2] << 8;
  if(level > 1000){
    sl_bt_cgm_send_sops_indicate(SET_RATE_INCREASE_ALERT_LEVEL,
                                   RSP_PARAMETER_OUT_OF_RANGE);
  }else{
    cgm_init.rate_increase_alert_value = level;
    sl_bt_cgm_send_sops_indicate(SET_RATE_INCREASE_ALERT_LEVEL, RSP_SUCCESS);
  }
}

/**************************************************************************//**
 * 4.15.3.23 CGMS/SEN/CGMCP/BV-21-C
 * [CGM Specific Ops – ‘Reset Device Specific Alert’]
 *****************************************************************************/
void sl_bt_cgm_reset_device_special_alert(void)
{
  cgm_status[2] = cgm_status[2] &  ~(SL_BT_CGM_STATUS_DEVICE_SEPCIFIC_ALERT);
  sl_bt_cgm_send_sops_indicate(RESET_DEVICE_SPECIFIC_ALERT, 0x01);
}
