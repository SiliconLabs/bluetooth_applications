/**
 * The Bluetooth Developer Studio auto-generated code
 * 
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com </b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 * 
 */
 
/*******************************************************************************
 *******************************   INCLUDES   **********************************
 ******************************************************************************/
#include "sl_bt_api.h"
#include "gatt_db.h"
#include <string.h>
#include <stdio.h>
#include "heart_rate.h"
#include "app_timer.h"
#include "hrm_app.h"
#include "ble_att_handler.h"


#define ATT_WRITE_NOT_PERMITTED 0x03
#define SENSOR_CONTACT_NOT_SUPPORTED             1<<1
#define SENSOR_CONTACT_SUPPORTED_NOT_DETECTED    2<<1 
#define SENSOR_CONTACT_SUPPORTED_DETECTED        3<<1

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
// TODO:: Fill const values

 
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

  //TODO:: Add suitable initialization for service
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
  
  // TODO:: Add your own code here.
  
  switch(evt->data.evt_gatt_server_user_read_request.characteristic)
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
  switch(evt->data.evt_gatt_server_user_write_request.characteristic)
  {
	// Heart Rate Control Point characteristic written
    case gattdb_heart_rate_control_point:
    {
      memcpy((uint8_t *)&service_data.heart_rate_control_point, evt->data.evt_gatt_server_user_write_request.value.data, evt->data.evt_gatt_server_user_write_request.value.len);
      // TODO:: Add your own code here.
    }
    break;
    
	// Write operation not permitted by default
    default:
    {
      response_code = ATT_WRITE_NOT_PERMITTED;
    }
    break;
  }
  
  // TODO:: Add your own code here.
  
  // Send response
  sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
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
  // TODO:: Add your own code here.
  // stop timer for indicate and notify
  if (notifications_enabled == true)
  {
    notifications_enabled = false;
    sl_bt_system_set_soft_timer(0,HEART_RATE_TIMER,0);
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


  if (hrm_get_status())
    send_data[0] = SENSOR_CONTACT_SUPPORTED_DETECTED; //flags - sensor contact supported and is detected
  else
    send_data[0] = SENSOR_CONTACT_SUPPORTED_NOT_DETECTED; //flags - sensor contact supported and is not detected
  
  service_data.heart_rate_measurement = (uint16_t)hrm_get_heart_rate();
  send_data[1] = service_data.heart_rate_measurement & 0xff;
  send_data[2] = (service_data.heart_rate_measurement >> 8) & 0xff;
  
  // Notification or Indication status changed for Heart Rate Measurement
  if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_heart_rate_measurement
      && evt->data.evt_gatt_server_characteristic_status.status_flags == gatt_server_client_config )
  {
    if (evt->data.evt_gatt_server_characteristic_status.client_config_flags) // Notification or Indication - enabled 
    {
      //Start a software timer 500ms interval
      sl_bt_system_set_soft_timer(16384,HEART_RATE_TIMER,0);

      // TODO:: Add your own code here.

      sl_bt_gatt_server_send_notification(evt->data.evt_gatt_server_characteristic_status.connection,
             evt->data.evt_gatt_server_characteristic_status.characteristic,
             3,
             send_data);
      notifications_enabled = true;
    }
    else // Notification or Indication - disabled
    {
      // TODO:: Add your own code here.
      notifications_enabled = false;
      //Stop the software timer 
      sl_bt_system_set_soft_timer(0,HEART_RATE_TIMER,0);
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
  //uint16_t len;

  if (hrm_get_status())
    send_data[0] = SENSOR_CONTACT_SUPPORTED_DETECTED; //flags - sensor contact supported and is detected
  else
    send_data[0] = SENSOR_CONTACT_SUPPORTED_NOT_DETECTED; //flags - sensor contact supported and is not detected

  service_data.heart_rate_measurement = (uint16_t)hrm_get_heart_rate();
  send_data[1] = service_data.heart_rate_measurement & 0xff;
  send_data[2] = (service_data.heart_rate_measurement >> 8) & 0xff;


  if (notifications_enabled == true)
  { //sl_bt_gatt_server_send_characteristic_notification(0xFF,
     sl_bt_gatt_server_send_notification(connect,
         gattdb_heart_rate_measurement,
          3,
          send_data);
  }
}

