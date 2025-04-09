/**************************************************************************//**
 * @file   heart_rate.c
 * @brief  Heart rate service
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
#include "sl_bt_api.h"
#include "gatt_db.h"
#include <string.h>
#include <stdio.h>
#include "heart_rate.h"
#include "hrm/ble/config/app_timer.h"
#include "hrm/app/hrm_app.h"
#include "hrm/ble/config/ble_att_handler.h"
#include "sl_sleeptimer.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
#define ATT_WRITE_NOT_PERMITTED                  0x03
#define SENSOR_CONTACT_NOT_SUPPORTED             1 << 1
#define SENSOR_CONTACT_SUPPORTED_NOT_DETECTED    2 << 1
#define SENSOR_CONTACT_SUPPORTED_DETECTED        3 << 1

/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/
typedef struct
{
  uint8_t heart_rate_measurement;
  uint8_t body_sensor_location;
  uint8_t heart_rate_control_point;
} heart_rate_t;

/*******************************************************************************
 *****************************   LOCAL DATA   **********************************
 ******************************************************************************/
static heart_rate_t service_data;
static bool notifications_enabled = false;
static sl_sleeptimer_timer_handle_t heart_rate_timer;
static void heart_rate_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                      void *data);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/*******************************************************************************
 * @brief
 *   Service Heart Rate initialization
 * @return
 *   None
 ******************************************************************************/
void heart_rate_init(void)
{
  // Initialize const strings values
  service_data.heart_rate_measurement = 0;
}

/*******************************************************************************
 * @brief
 *   Function to handle read data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void heart_rate_read_callback(sl_bt_msg_t *evt)
{
  uint16_t characteristic_size = 0;
  const uint8_t *characteristic_ptr = NULL;

  switch (evt->data.evt_gatt_server_user_read_request.characteristic)
  {
    // Body Sensor Location value read
    case gattdb_body_sensor_location:
    {
      characteristic_size = sizeof(service_data.body_sensor_location);
      characteristic_ptr = (const uint8_t *)&service_data.body_sensor_location;
    }
    break;

    // Do nothing
    default:
      break;
  }

  // Send response
  ble_att_send_data(evt->data.evt_gatt_server_user_read_request.connection,
                    evt->data.evt_gatt_server_user_read_request.characteristic,
                    characteristic_ptr, characteristic_size);
}

/*******************************************************************************
 * @brief
 *   Function to handle write data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void heart_rate_write_callback(sl_bt_msg_t *evt)
{
  uint8_t response_code = 0;

  switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
    // Heart Rate Control Point characteristic written
    case gattdb_heart_rate_control_point:
      memcpy((uint8_t *)&service_data.heart_rate_control_point,
             evt->data.evt_gatt_server_user_write_request.value.data,
             evt->data.evt_gatt_server_user_write_request.value.len);
      break;

    // Write operation not permitted by default
    default:
      response_code = ATT_WRITE_NOT_PERMITTED;
      break;
  }

  // Send response
  sl_bt_gatt_server_send_user_write_response(
    evt->data.evt_gatt_server_user_write_request.connection,
    evt->data.evt_gatt_server_user_write_request.characteristic,
    response_code);
}

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void heart_rate_disconnect_event(sl_bt_msg_t *evt)
{
  (void)evt;
  // stop timer for indicate and notify
  if (notifications_enabled == true) {
    notifications_enabled = false;
    sl_sleeptimer_stop_timer(&heart_rate_timer);
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
void heart_rate_characteristic_status(sl_bt_msg_t *evt)
{
  uint8_t send_data[3];

  if (hrm_get_status()) {
    send_data[0] = SENSOR_CONTACT_SUPPORTED_DETECTED; // flags - sensor contact
                                                      //   supported and is
                                                      //   detected
  } else {
    send_data[0] = SENSOR_CONTACT_SUPPORTED_NOT_DETECTED; // flags - sensor
                                                          //   contact supported
                                                          //   and is not
                                                          //   detected
  }
  service_data.heart_rate_measurement = (uint16_t)hrm_get_heart_rate();
  send_data[1] = service_data.heart_rate_measurement & 0xff;
  send_data[2] = (service_data.heart_rate_measurement >> 8) & 0xff;

  // Notification or Indication status changed for Heart Rate Measurement
  if ((evt->data.evt_gatt_server_characteristic_status.characteristic
       == gattdb_heart_rate_measurement)
      && (evt->data.evt_gatt_server_characteristic_status.status_flags
          == sl_bt_gatt_server_client_config)) {
    // Notification or Indication - enabled
    if (evt->data.evt_gatt_server_characteristic_status.client_config_flags) {
      // Start a software timer 500ms interval
      sl_sleeptimer_start_periodic_timer(&heart_rate_timer,
                                         16384,
                                         heart_rate_timer_callback,
                                         (void *)NULL, 0, 0);

      sl_bt_gatt_server_send_notification(
        evt->data.evt_gatt_server_characteristic_status.connection,
        evt->data.evt_gatt_server_characteristic_status.characteristic,
        3,
        send_data);
      notifications_enabled = true;
    }
    // Notification or Indication - disabled
    else {
      notifications_enabled = false;
      // Stop the software timer
      sl_sleeptimer_stop_timer(&heart_rate_timer);
    }
  }
}

/*******************************************************************************
 * @brief
 *   Function to update Heart Rate data
 * @return
 *   None
 ******************************************************************************/
void heart_rate_send_new_data(uint8_t connect)
{
  uint8_t send_data[3];
  // uint16_t len;

  if (hrm_get_status()) {
    send_data[0] = SENSOR_CONTACT_SUPPORTED_DETECTED; // flags - sensor contact
                                                      //   supported and is
                                                      //   detected
  } else {
    send_data[0] = SENSOR_CONTACT_SUPPORTED_NOT_DETECTED; // flags - sensor
                                                          //   contact supported
                                                          //   and is not
                                                          //   detected
  }
  service_data.heart_rate_measurement = (uint16_t)hrm_get_heart_rate();
  send_data[1] = service_data.heart_rate_measurement & 0xff;
  send_data[2] = (service_data.heart_rate_measurement >> 8) & 0xff;

  if (notifications_enabled == true) {
    sl_bt_gatt_server_send_notification(connect,
                                        gattdb_heart_rate_measurement,
                                        3,
                                        send_data);
  }
}

static void heart_rate_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                      void *data)
{
  (void)handle;
  (void)data;

  sl_bt_external_signal(HEART_RATE_TIMER);
}
