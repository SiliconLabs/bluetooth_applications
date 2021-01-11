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
#include "device_information.h"
#include "gatt_db.h"
//#include "hrm_app.h"
#include <string.h>
#include <stdio.h>
#include "sl_bt_api.h"
#include "ble_att_handler.h"

#define ATT_WRITE_NOT_PERMITTED 0x03

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
// TODO:: Fill const values
static const uint8_t * MANUFACTURER_NAME_STRING_STRING = (uint8_t *)"Silicon Labs";
static const uint8_t * MODEL_NUMBER_STRING_STRING = (uint8_t *)"max86161 HRM Demo";
static const uint8_t * SERIAL_NUMBER_STRING_STRING = (uint8_t *)"xxxx";
static const uint8_t * HARDWARE_REVISION_STRING_STRING = (uint8_t *)"xxxx";
static const uint8_t * FIRMWARE_REVISION_STRING_STRING = (uint8_t *)"1.0.0";
static const uint8_t * SOFTWARE_REVISION_STRING_STRING = (uint8_t *)"xxxx";

 
/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/
typedef struct
{
  const uint8_t * manufacturer_name_string;
  const uint8_t * model_number_string;
  const uint8_t * serial_number_string;
  const uint8_t * hardware_revision_string;
  const uint8_t * firmware_revision_string;
  const uint8_t * software_revision_string;
} device_information_t;
 
/*******************************************************************************
 *****************************   LOCAL DATA   **********************************
 ******************************************************************************/
static device_information_t service_data;
 
/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/
 
/*******************************************************************************
 * @brief
 *   Service Device Information initialization
 * @return
 *   None
 ******************************************************************************/
void device_information_init(void)
{
  // Initialize const strings values
  service_data.manufacturer_name_string = MANUFACTURER_NAME_STRING_STRING;
  service_data.model_number_string = MODEL_NUMBER_STRING_STRING;
  service_data.serial_number_string = SERIAL_NUMBER_STRING_STRING;
  service_data.hardware_revision_string = HARDWARE_REVISION_STRING_STRING;
  service_data.firmware_revision_string = FIRMWARE_REVISION_STRING_STRING;
  service_data.software_revision_string = SOFTWARE_REVISION_STRING_STRING;

  //TODO:: Add suitable initialization for service
}

/*******************************************************************************
 * @brief
 *   Function to handle read data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void device_information_read_callback(sl_bt_msg_t *evt)
{
  uint16_t characteristicSize = 0;
  const uint8_t *characteristicPtr = NULL;
  
  // TODO:: Add your own code here.
  
  
  switch(evt->data.evt_gatt_server_user_read_request.characteristic)
  {
	// Manufacturer Name String value read
    case gattdb_manufacturer_name_string:
    {
      characteristicSize = strlen((const char *)service_data.manufacturer_name_string);
      characteristicPtr = service_data.manufacturer_name_string;
    }
    break;
	  
	// Model Number String value read
    case gattdb_model_number_string:
    {
      characteristicSize = strlen((const char *)service_data.model_number_string);
      characteristicPtr = service_data.model_number_string;
    }
    break;
	  
	// Serial Number String value read
    case gattdb_serial_number_string:
    {
      characteristicSize = strlen((const char *)service_data.serial_number_string);
      characteristicPtr = service_data.serial_number_string;
    }
    break;
	  
	// Hardware Revision String value read
    case gattdb_hardware_revision_string:
    {
      characteristicSize = strlen((const char *)service_data.hardware_revision_string);
      characteristicPtr = service_data.hardware_revision_string;
    }
    break;
	  
	// Firmware Revision String value read
    case gattdb_firmware_revision_string:
    {
      characteristicSize = strlen((const char *)service_data.firmware_revision_string);
      characteristicPtr = service_data.firmware_revision_string;
    }
    break;
	  
	// Software Revision String value read
    case gattdb_software_revision_string:
    {
      characteristicSize = strlen((const char *)service_data.software_revision_string);
      characteristicPtr = service_data.software_revision_string;
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
void device_information_write_callback(sl_bt_msg_t *evt)
{
  // This service doesn't support write operation - return error response - write not permitted
  sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
                                                 evt->data.evt_gatt_server_user_write_request.characteristic,
                                                 ATT_WRITE_NOT_PERMITTED);

}

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void device_information_disconnect_event(sl_bt_msg_t *evt)
{
  (void)evt;
  // TODO:: Add your own code here.
}

