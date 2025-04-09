/**************************************************************************//**
 * @file   pulse_oximeter.c
 * @brief  Pulse oximeter service
 * @version 1.1.0
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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

/*******************************************************************************
 *******************************   INCLUDES   **********************************
 ******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "hrm/ble/config/ble_att_handler.h"
#include "gatt_db.h"
#include "sl_bt_api.h"
#include "pulse_oximeter.h"
#include "hrm/app/hrm_app.h"
#include "hrm/ble/config/app_timer.h"
#include "sl_sleeptimer.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
// TODO:: Fill const values
#define ATT_WRITE_NOT_PERMITTED 0x03

/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/
typedef struct
{
  plxSpotcheckMeasurement_t plx_spot_check_measurement;
  plxContinuousMeasurement_t plx_continuous_measurement;
  plxFeatures_t plx_features;
  plxProcedure_t plx_record_access_control_point;
} pulse_oximeter_t;

/*******************************************************************************
 *****************************   LOCAL DATA   **********************************
 ******************************************************************************/
static pulse_oximeter_t service_data;
static bool notifications_enabled = false;
static sl_sleeptimer_timer_handle_t pulse_oximeter_timer;
static void pulse_oximeter_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                          void *data);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/
/*******************************************************************************
 * @brief
 *   Service Pulse Oximeter initialization
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_init(void)
{
  // TODO:: Add suitable initialization for service

  // ctsDateTime_t time = {2020, 7, 31, 15, 27, 40};
  // service_data.plx_spot_check_measurement.timestamp = time;
  service_data.plx_spot_check_measurement.flags = 0;  // Only present SpO2 and
                                                      //   PR
  service_data.plx_spot_check_measurement.SpO2PRSpotcheck.SpO2 = 0;
  service_data.plx_spot_check_measurement.SpO2PRSpotcheck.PR = 0;

  service_data.plx_continuous_measurement.flags = 0;  // Only present Normal
                                                      //   SpO2 and PR
  service_data.plx_continuous_measurement.SpO2PRNormal.SpO2 = 0;
  service_data.plx_continuous_measurement.SpO2PRNormal.PR = 0;

  service_data.plx_features.supportedFeatures = 0;  // None

  service_data.plx_record_access_control_point.opCode = 0;
  service_data.plx_record_access_control_point.operand.numberOfRecords = 0;
  service_data.plx_record_access_control_point.operand.responseCode.reqOpCode =
    0;
  service_data.plx_record_access_control_point.operand.responseCode.rspCodeValue
    = 0;
  service_data.plx_record_access_control_point.plxOperator = 0;
}

/*******************************************************************************
 * @brief
 *   Function to handle read data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_read_callback(sl_bt_msg_t *evt)
{
  uint16_t characteristicSize = 0;
  const uint8_t *characteristicPtr = NULL;

  // TODO:: Add your own code here.

  switch (evt->data.evt_gatt_server_user_read_request.characteristic)
  {
    // PLX features value read
    case gattdb_plx_features:
    {
      characteristicSize = sizeof(service_data.plx_features);
      characteristicPtr = (const uint8_t *)&service_data.plx_features;
    }
    break;

    // Do nothing
    default:
      break;
  }

  // Send response
  ble_att_send_data(evt->data.evt_gatt_server_user_read_request.connection,
                    evt->data.evt_gatt_server_user_read_request.characteristic,
                    characteristicPtr, characteristicSize);
}

/*******************************************************************************
 * @brief
 *   Function to handle write data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_write_callback(sl_bt_msg_t *evt)
{
  uint8_t responseCode = 0;
  switch (evt->data.evt_gatt_server_user_write_request.characteristic)
  {
    // Record access control point characteristic written
    case gattdb_record_access_control_point:
    {
      memcpy((uint8_t *)&service_data.plx_record_access_control_point,
             evt->data.evt_gatt_server_user_write_request.value.data,
             evt->data.evt_gatt_server_user_write_request.value.len);
      // TODO:: Add your own code here.
    }
    break;

    // Write operation not permitted by default
    default:
    {
      responseCode = ATT_WRITE_NOT_PERMITTED;
    }
    break;
  }

  // TODO:: Add your own code here.

  // Send response
  sl_bt_gatt_server_send_user_write_response(
    evt->data.evt_gatt_server_user_write_request.connection,
    evt->data.evt_gatt_server_user_write_request.characteristic,
    responseCode);
}

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_disconnect_event(sl_bt_msg_t *evt)
{
  (void)evt;
  // TODO:: Add your own code here.
  // stop timer for indicate and notify
  if (notifications_enabled == true) {
    notifications_enabled = false;
    sl_sleeptimer_stop_timer(&pulse_oximeter_timer);
  }
}

/*******************************************************************************
 * @brief
 *   Function to handle gecko_evt_gatt_server_characteristic_status_id event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_characteristic_status(sl_bt_msg_t *evt)
{
  uint8_t send_data[5];

  send_data[0] = service_data.plx_continuous_measurement.flags; // 0

  service_data.plx_continuous_measurement.SpO2PRNormal.SpO2 =
    (uint16_t)hrm_get_spo2();
  service_data.plx_continuous_measurement.SpO2PRNormal.PR =
    (uint16_t)hrm_get_heart_rate();
  send_data[1] = service_data.plx_continuous_measurement.SpO2PRNormal.SpO2
                 & 0xff;
  send_data[2] =
    (service_data.plx_continuous_measurement.SpO2PRNormal.SpO2 >> 8) & 0xff;
  send_data[3] = service_data.plx_continuous_measurement.SpO2PRNormal.PR & 0xff;
  send_data[4] =
    (service_data.plx_continuous_measurement.SpO2PRNormal.PR >> 8) & 0xff;

  // Notification or Indication status changed for PLX continuous measurement
  if ((evt->data.evt_gatt_server_characteristic_status.characteristic
       == gattdb_plx_continuous_measurement)
      && (evt->data.evt_gatt_server_characteristic_status.status_flags
          == sl_bt_gatt_server_client_config)) {
    //  Notification or Indication - enabled
    if (evt->data.evt_gatt_server_characteristic_status.client_config_flags) {
      // Start a software timer 500ms interval
      sl_sleeptimer_start_periodic_timer(&pulse_oximeter_timer,
                                         16384,
                                         pulse_oximeter_timer_callback,
                                         (void *)NULL, 0, 0);
      // TODO:: Add your own code here.
      // sl_bt_gatt_server_send_characteristic_notification(0xFF,
      sl_bt_gatt_server_send_notification(
        evt->data.evt_gatt_server_characteristic_status.connection,
        evt->data.evt_gatt_server_characteristic_status.characteristic,
        5,
        send_data);
      notifications_enabled = true;
    } else { // Notification or Indication - disabled
      // TODO:: Add your own code here.
      notifications_enabled = false;
      // Stop the software timer
      sl_sleeptimer_stop_timer(&pulse_oximeter_timer);
    }
  }
}

/*******************************************************************************
 * @brief
 *   Function to update SpO2 data
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_send_new_data(uint8_t connect)
{
  uint8_t send_data[5];

  send_data[0] = service_data.plx_continuous_measurement.flags; // 0

  service_data.plx_continuous_measurement.SpO2PRNormal.SpO2 =
    (uint16_t)hrm_get_spo2();
  service_data.plx_continuous_measurement.SpO2PRNormal.PR =
    (uint16_t)hrm_get_heart_rate();
  send_data[1] = service_data.plx_continuous_measurement.SpO2PRNormal.SpO2
                 & 0xff;
  send_data[2] =
    (service_data.plx_continuous_measurement.SpO2PRNormal.SpO2 >> 8) & 0xff;
  send_data[3] = service_data.plx_continuous_measurement.SpO2PRNormal.PR & 0xff;
  send_data[4] =
    (service_data.plx_continuous_measurement.SpO2PRNormal.PR >> 8) & 0xff;

  if (notifications_enabled == true) {
    sl_bt_gatt_server_send_notification(connect,
                                        gattdb_plx_continuous_measurement,
                                        5,
                                        send_data);
  }
}

static void pulse_oximeter_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                          void *data)
{
  (void)handle;
  (void)data;

  sl_bt_external_signal(PULSE_OXIMETER_TIMER);
}
