/**************************************************************************//**
 * @file   device_information.h
 * @brief  Header file of device information service
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
