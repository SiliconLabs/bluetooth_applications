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

#ifndef __HOMEKIT_ATT_HANDLER_H
#define __HOMEKIT_ATT_HANDLER_H

/*******************************************************************************
 *******************************   INCLUDES   **********************************
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/*******************************************************************************
 * @brief
 *   Function encrypt data and send it over BLE. It automatically handle sending
 *   long charactiristics.
 * @param[in] connection
 *   connection id
 * @param[in] attHandle
 *   characteristic handle
 * @param[in] data
 *   pointer to data to send
 * @param[in] len
 *   size of data to send
 * @return
 *   None
 ******************************************************************************/
void ble_att_send_data(uint8_t connection, uint16_t attHandle, const uint8_t *data, uint16_t len);

/*******************************************************************************
 * @brief
 *   Function handle sending long characterictic value
 * @param[in] attHandle
 *   characteristic handle
 * @param[in] offset
 *   offset for reading data
 * @return
 *   Return true if event was handled, in other case return false
 ******************************************************************************/
bool ble_att_send_data_handler(uint16_t attHandle, uint16_t offset);

/*******************************************************************************
 * @brief
 *   Function set supported MTU size for attribute handling data
 * @param[in] mtuSize
 *   MTU size
 * @return
 *   None
 ******************************************************************************/
void ble_set_mtu_size(uint16_t mtu_Size);

#endif //__HOMEKIT_ATT_HANDLER_H
