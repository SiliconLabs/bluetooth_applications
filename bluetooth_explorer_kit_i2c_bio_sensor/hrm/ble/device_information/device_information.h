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
 
#ifndef __DEVICE_INFORMATION_H
#define __DEVICE_INFORMATION_H

/*******************************************************************************
 *******************************   INCLUDES   **********************************
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
//#include "native_gecko.h"
#include "sl_bt_types.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

 
/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/
 
/*******************************************************************************
 * @brief
 *   Service Device Information initialization
 * @return
 *   None
 ******************************************************************************/
void device_information_init(void);

/*******************************************************************************
 * @brief
 *   Function to handle read data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
extern void device_information_read_callback(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle write data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
extern void device_information_write_callback(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
extern void device_information_disconnect_event(sl_bt_msg_t *evt);

#endif //__DEVICE_INFORMATION_H
