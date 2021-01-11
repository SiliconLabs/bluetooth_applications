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
