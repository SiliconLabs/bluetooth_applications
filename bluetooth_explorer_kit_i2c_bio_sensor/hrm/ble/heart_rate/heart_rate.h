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
 
#ifndef __HEART_RATE_H
#define __HEART_RATE_H

/*******************************************************************************
 *******************************   INCLUDES   **********************************
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "sl_bt_api.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

 
/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/
 
/*******************************************************************************
 * @brief
 *   Service Heart Rate initialization
 * @return
 *   None
 ******************************************************************************/
void heart_rate_init(void);

/*******************************************************************************
 * @brief
 *   Function to handle read data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
extern void heart_rate_read_callback(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle write data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
extern void heart_rate_write_callback(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
extern void heart_rate_disconnect_event(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle gecko_evt_gatt_server_characteristic_status_id event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void heart_rate_characteristic_status(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to update Heart Rate data
 * @return
 *   None
 ******************************************************************************/
void heart_rate_send_new_data(uint8_t connect);

#endif //__HEART_RATE_H
