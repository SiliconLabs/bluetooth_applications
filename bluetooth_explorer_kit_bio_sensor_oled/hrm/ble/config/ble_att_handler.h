/**************************************************************************//**
* @file   ble_att_handler.h
* @brief  Header file of ble_att_handler
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

#ifndef _BLE_ATT_HANDLER_H
#define _BLE_ATT_HANDLER_H

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
void ble_att_send_data(uint8_t connection,
                       uint16_t attHandle,
                       const uint8_t *data,
                       uint16_t len);

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

#endif //_BLE_ATT_HANDLER_H
