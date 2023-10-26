/**************************************************************************//**
* @file   device_information.c
* @brief  Device information service
* @version 1.1.0
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "device_information.h"
#include "gatt_db.h"
// #include "hrm_app.h"
#include <string.h>
#include <stdio.h>
#include "sl_bt_api.h"
#include "hrm/ble/config/ble_att_handler.h"

#define ATT_WRITE_NOT_PERMITTED 0x03

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
// TODO:: Fill const values
static const uint8_t *MANUFACTURER_NAME_STRING_STRING =
  (uint8_t *)"Silicon Labs";
static const uint8_t *MODEL_NUMBER_STRING_STRING =
  (uint8_t *)"max86161 HRM Demo";
static const uint8_t *SERIAL_NUMBER_STRING_STRING = (uint8_t *)"xxxx";
static const uint8_t *HARDWARE_REVISION_STRING_STRING = (uint8_t *)"xxxx";
static const uint8_t *FIRMWARE_REVISION_STRING_STRING = (uint8_t *)"1.0.0";
static const uint8_t *SOFTWARE_REVISION_STRING_STRING = (uint8_t *)"xxxx";

/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/
typedef struct
{
  const uint8_t *manufacturer_name_string;
  const uint8_t *model_number_string;
  const uint8_t *serial_number_string;
  const uint8_t *hardware_revision_string;
  const uint8_t *firmware_revision_string;
  const uint8_t *software_revision_string;
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

  // TODO:: Add suitable initialization for service
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

  switch (evt->data.evt_gatt_server_user_read_request.characteristic)
  {
    // Manufacturer Name String value read
    case gattdb_manufacturer_name_string:
    {
      characteristicSize = strlen(
        (const char *)service_data.manufacturer_name_string);
      characteristicPtr = service_data.manufacturer_name_string;
    }
    break;

    // Model Number String value read
    case gattdb_model_number_string:
    {
      characteristicSize =
        strlen((const char *)service_data.model_number_string);
      characteristicPtr = service_data.model_number_string;
    }
    break;

    // Serial Number String value read
    case gattdb_serial_number_string:
    {
      characteristicSize = strlen(
        (const char *)service_data.serial_number_string);
      characteristicPtr = service_data.serial_number_string;
    }
    break;

    // Hardware Revision String value read
    case gattdb_hardware_revision_string:
    {
      characteristicSize = strlen(
        (const char *)service_data.hardware_revision_string);
      characteristicPtr = service_data.hardware_revision_string;
    }
    break;

    // Firmware Revision String value read
    case gattdb_firmware_revision_string:
    {
      characteristicSize = strlen(
        (const char *)service_data.firmware_revision_string);
      characteristicPtr = service_data.firmware_revision_string;
    }
    break;

    // Software Revision String value read
    case gattdb_software_revision_string:
    {
      characteristicSize = strlen(
        (const char *)service_data.software_revision_string);
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
  // This service doesn't support write operation - return error response -
  //   write not permitted
  sl_bt_gatt_server_send_user_write_response(
    evt->data.evt_gatt_server_user_write_request.connection,
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
