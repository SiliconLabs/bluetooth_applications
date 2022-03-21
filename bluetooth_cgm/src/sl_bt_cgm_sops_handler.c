/***************************************************************************//**
 * @file
 * @brief CGM specific ops characteristic
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
#include "sl_bt_cgm_measurement.h"

/* SOPS characteristic indication enable flag */
bool sops_enabled = false;
/* If a CGM session is running, the CGM Sensor measures the glucose level
 * periodically in a device specific interval (Measurement time interval).*/
uint8_t periodic_comm_interval = GLUCOSE_DEFAULT_COMM_INTERVAL_SEC;


/**************************************************************************//**
 * CGM - CGM specific Ops characteristic
 * indication changed callback
 * Called when indication of CGM SPOS is enabled/disabled by the client.
 *****************************************************************************/
void sl_bt_cgm_sops_indication_handler(sl_bt_msg_t *evt)
{
  sl_bt_evt_gatt_server_characteristic_status_t status = \
      evt->data.evt_gatt_server_characteristic_status;
  // client characteristic configuration changed by remote GATT client
  if (sl_bt_gatt_server_client_config == status.status_flags) {
      if (sl_bt_gatt_server_indication == status.client_config_flags) {
          app_log("SOPS enable indicate\n");
          sops_enabled = true;
      }
      else if(sl_bt_gatt_server_disable == status.client_config_flags) {
          app_log("SOPS disable indicate\n");
          sops_enabled = false;
      }
   }
  // confirmation of indication received from remove GATT client
  else if (sl_bt_gatt_server_confirmation == status.status_flags) {
    app_log("SOPS receive indication confirm\n");
  }
}

/**************************************************************************//**
 send SOPS indication event
*****************************************************************************/
void sl_bt_cgm_send_sops_indicate(uint8_t opcode, uint8_t response_code)
{
  uint8_t sl_sops_indication_reply[] = {RSP_CODE_SOPS,0x00,0x00};
  sl_sops_indication_reply[1] = opcode;
  sl_sops_indication_reply[2] = response_code;
  sl_status_t sc;
  sc = sl_bt_gatt_server_send_indication(
    connection,
    gattdb_cgm_specific_ops_control_point,
    sizeof(sl_sops_indication_reply),
    sl_sops_indication_reply
    );
  if(sc != SL_STATUS_OK){
      app_log("sl_bt_cgm_send_sops_indicate 0x%04X.\n", sc);
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
 send SOPS response value(Hypo/Hyper alert value..)
 *****************************************************************************/
void sl_bt_cgm_send_sops_response(uint8_t opcode, uint16_t value)
{
  uint8_t sl_sops_indication_reply[] = {0x00,0x00,0x00};
  sl_sops_indication_reply[0] = opcode;
  sl_sops_indication_reply[1] = value & 0xff;
  sl_sops_indication_reply[2] = value >> 8;
  sl_status_t sc;
  sc = sl_bt_gatt_server_send_indication(
    connection,
    gattdb_cgm_specific_ops_control_point,
    sizeof(sl_sops_indication_reply),
    sl_sops_indication_reply
    );
  if(sc != SL_STATUS_OK){
    app_log("sl_bt_cgm_send_sops_response 0x%04X.\n", sc);
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
 * SOPS characteristic handler
 *****************************************************************************/
void sl_bt_cgm_handle_sops(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t operand;
  uint8_t *array = evt->data.evt_gatt_server_attribute_value.value.data;
  uint8_t len = evt->data.evt_gatt_server_attribute_value.value.len;
  for(int i = 0; i < len; i++){
    app_log("0x%02x ", array[i]);
  }
  app_log("\n");
  uint16_t crc = 0;
  if(SL_BT_CGM_E2E_CRC_SUPPORTED){
    crc = array[len - 2] | array[len - 1] << 8;
    if(crc != sli_bt_cgm_crc16(0xFFFF, array, len - 2)){
      sl_bt_gatt_server_send_user_write_response(connection,
                                       gattdb_cgm_specific_ops_control_point,
                                       ATT_INVALID_CRC);
      return;
    }else{
        sl_bt_gatt_server_send_user_write_response(connection,
                                         gattdb_cgm_specific_ops_control_point,
                                         ATT_SUCCEED);
    }
  }else{
      sl_bt_gatt_server_send_user_write_response(connection,
                                         gattdb_cgm_specific_ops_control_point,
                                         ATT_SUCCEED);
  }
  uint8_t opcode = array[0];
  switch (opcode) {
    //Once stopped, The CGM Sensor then deletes all data from previous session
    //and starts the measurement.
    case STOP_THE_SESSION:
      app_log("stop session\n");
      if(session_started == true){
        session_started = false;
        sc = sl_simple_timer_stop(&cgm_periodic_timer);
        if(sc != SL_STATUS_OK){
            app_log("sl_simple_timer_stop failed 0x%04X\n", sc);
        }
        cgm_status[2] = cgm_status[2] | SL_BT_CGM_STATUS_SESSION_STOPPED;
        sl_bt_cgm_send_sops_indicate(opcode, RSP_SUCCESS);
      }else{
        cgm_status[2] = cgm_status[2] |  SL_BT_CGM_STATUS_SESSION_STOPPED;
        sl_bt_cgm_send_sops_indicate(opcode, RSP_SUCCESS);
      }
      break;
    case START_THE_SESSION:
      app_log("start session\n");
      if(session_started == false){
        sc = sl_simple_timer_start(&cgm_periodic_timer,
                                   periodic_comm_interval * 1000,
                                   sl_bt_cgm_periodic_timer_cb,
                                   NULL,
                                   true);
        if(sc != SL_STATUS_OK){
            app_log("sl_simple_timer_start failed 0x%04X\n", sc);
        }
        session_started = true;
        cgm_status[2] = cgm_status[2] & ~SL_BT_CGM_STATUS_SESSION_STOPPED;
        sl_bt_cgm_send_sops_indicate(opcode, RSP_SUCCESS);
      }else{
          sl_bt_cgm_send_sops_indicate(opcode, RSP_PROCEDURE_NOT_COMPLETED);
          app_log("return Procedure not completed "
              "as the session is already started.\n");
      }
      break;
    // CGM Specific Ops – ‘Set CGM Communication Interval’
    case SET_CGM_COMM_INTEVAL:
      app_log("Set CGM Communication Interval procedure.\n");
      operand = array[1];
      sl_bt_cgm_set_comm_interval(operand);
      sl_bt_cgm_send_sops_indicate(opcode, RSP_SUCCESS);
      break;
    // CGMS/SEN/CGMCP/BV-02-C
    //[CGM Specific Ops – ‘Get CGM Communication Interval without E2E-CRC’]
    case GET_CGM_COMM_INTEVAL:
      app_log("Get CGM communication interval\n");
      sl_bt_cgm_sops_get_comm_interval();
      break;
   //CGMS/SEN/CGMCP/BV-08-C [CGM Specific Ops – ‘Set Glucose Calibration value’]
    case SET_CAL_VALUE:
      app_log("set glucose calibration value.\n");
      sl_bt_cgm_set_cal_value(evt);
      break;
    case GET_CAL_VALUE:
      // Get Glucose Calibration value
      sl_bt_cgm_get_cal_value(evt);
      break;
    case GET_PATIENT_HIGH_ALERT_VALUE:
      sl_bt_cgm_get_alert_level(evt);
      break;
    case SET_PATIENT_HIGH_ALERT_VALUE:
      sl_bt_cgm_set_high_alert_level(evt);
      break;
    case SET_PATIENT_LOW_ALERT_VALUE:
      sl_bt_cgm_set_low_alert_level(evt);
      break;
    case GET_PATIENT_LOW_ALERT_VALUE:
      sl_bt_cgm_get_alert_level(evt);
      break;
     /* 4.15.3.11 CGMS/SEN/CGMCP/BV-13-C
      * [CGM Specific Ops – ‘Get Hypo Alert Level’]*/
    case GET_HYPO_ALERT_LEVEL:
      sl_bt_cgm_send_sops_response(GET_HYPO_ALERT_LEVEL_RSP,
                                   cgm_init.hypo_alert_level);
      break;
    case SET_HYPO_ALERT_VALUE:
      sl_bt_cgm_set_hypo_alert_level(evt);
      break;
      /* 4.15.3.14 CGMS/SEN/CGMCP/BV-15-C
       * [CGM Specific Ops – ‘Get Hyper Alert Level’]*/
    case GET_HYPER_ALERT_LEVEL:
      sl_bt_cgm_send_sops_response(GET_HYPER_ALERT_LEVEL_RSP,
                                   cgm_init.hyper_alert_level);
      break;
    case SET_HYPER_ALERT_LEVEL:
      sl_bt_cgm_set_hyper_alert_level(evt);
      break;
      /* 4.15.3.17 CGMS/SEN/CGMCP/BV-17-C
       * [CGM Specific Ops – ‘Get Rate of Decrease Alert Level’] */
    case GET_RATE_DECREASE_ALERT_LEVEL:
      sl_bt_cgm_send_sops_response(GET_RATE_DECREASE_ALERT_LEVEL_RSP,
                                   cgm_init.rate_decrease_alert_value);
      break;
    case SET_RATE_DECREASE_ALERT_LEVEL:
      sl_bt_cgm_set_rate_decrease_alert_level(evt);
      break;
      /* 4.15.3.20 CGMS/SEN/CGMCP/BV-19-C
       * [CGM Specific Ops – ‘Get Rate of Increase Alert Level’] */
    case GET_RATE_INCREASE_ALERT_LEVEL:
      sl_bt_cgm_send_sops_response(GET_RATE_INCREASE_ALERT_LEVEL_RSP,
                                   cgm_init.rate_increase_alert_value);
      break;
    case SET_RATE_INCREASE_ALERT_LEVEL:
      sl_bt_cgm_set_rate_increase_alert_level(evt);
      break;
    case RESET_DEVICE_SPECIFIC_ALERT:
      sl_bt_cgm_reset_device_special_alert();
      break;
    default:
      break;
  }
}



