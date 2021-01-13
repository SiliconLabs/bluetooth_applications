/**************************************************************************//**
 * @file   ble_att_handler.c
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
#include "ble_att_handler.h"
#include <string.h>
#include <stdio.h>
#include "sl_bt_api.h"



/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
/**
 *  buffer size = maximum ble characteristic length(64)
 *                       + tag size(16)
 */
#define ATT_BUFFER_SIZE    64
#define ATT_INVALID_OFFSET 0x07
/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/
typedef struct
{
  uint8_t connection;
  uint16_t att_handle;
  uint16_t bytes_to_send;
  uint16_t current_position;
  uint8_t buffer[ATT_BUFFER_SIZE];
  uint16_t max_payload;
} ble_att_handler_t;

/*******************************************************************************
 *****************************   LOCAL DATA   **********************************
 ******************************************************************************/
static ble_att_handler_t ble_att_handler = {.max_payload = 22};

/*******************************************************************************
 * @brief
 *   Send data over BLE. It automatically handle sending long charactiristics.
 * @param[in] connection
 *   connection id
 * @param[in] att_handle
 *   characteristic handle
 * @param[in] data
 *   pointer to data to send
 * @param[in] len
 *   size of data to send
 * @return
 *   None
 ******************************************************************************/
void ble_att_send_data(uint8_t connection, uint16_t att_handle, const uint8_t *data, uint16_t len)
{
  uint8_t bytes_to_send;
  uint16_t sent_len;

  if ( len < ble_att_handler.max_payload)
  {
    bytes_to_send = len;
  }
  else
  {
    bytes_to_send = ble_att_handler.max_payload;
    ble_att_handler.connection = connection;
    ble_att_handler.att_handle = att_handle;
    ble_att_handler.bytes_to_send = len;
    ble_att_handler.current_position = ble_att_handler.max_payload;
    memcpy(ble_att_handler.buffer, data, len);
  }

  // Send response
  sl_bt_gatt_server_send_user_read_response(connection,
                                                att_handle,
                                                (uint8_t) 0x00,  /* SUCCESS */
                                                bytes_to_send,
                                                data, &sent_len);
}

/*******************************************************************************
 * @brief
 *   Function handle sending long characterictic value
 * @param[in] att_handle
 *   characteristic handle
 * @param[in] offset
 *   offset for reading data
 * @return
 *   Return true if event was handled, in other case return false
 ******************************************************************************/
bool ble_att_send_data_handler(uint16_t att_handle, uint16_t offset)
{
  uint16_t sent_len;
  if (att_handle == ble_att_handler.att_handle)
  {
    uint8_t bytes_to_send;
    if( offset != ble_att_handler.current_position)
    {
      // Send response that offset is invalid
      sl_bt_gatt_server_send_user_read_response(ble_att_handler.connection,
                                                    att_handle,
                                                    ATT_INVALID_OFFSET,
                                                    0,
                                                    NULL, &sent_len);
      return true;

    }

    bytes_to_send = ble_att_handler.bytes_to_send  - ble_att_handler.current_position;

    if ( bytes_to_send < ble_att_handler.max_payload)
    {
      // Last packet - clear attribute handle
      ble_att_handler.att_handle = 0;
    }
    else
    {
      bytes_to_send = ble_att_handler.max_payload;
    }
    // Send response
    sl_bt_gatt_server_send_user_read_response(ble_att_handler.connection,
                                                  att_handle,
                                                  (uint8_t) 0x00,  /* SUCCESS */
                                                  bytes_to_send,
                                                  &ble_att_handler.buffer[ble_att_handler.current_position], &sent_len);
    // Set new position
    ble_att_handler.current_position += bytes_to_send;
    // Event handled
    return true;
  }
  else
  {
    // Event not handled
    return false;
  }
}

/*******************************************************************************
 * @brief
 *   Function set supported MTU size for attribute handling data
 * @param[in] mtuSize
 *   MTU size
 * @return
 *   None
 ******************************************************************************/
void ble_setmtu_size(uint16_t mtu_size)
{
  ble_att_handler.max_payload = mtu_size - 1;
}
