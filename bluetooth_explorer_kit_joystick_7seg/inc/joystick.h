/***************************************************************************//**
 * @file joystick.h
 * @brief Joystick driver
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
*
* EVALUATION QUALITY
* This code has been minimally tested to ensure that it builds with the specified
* dependency versions and is suitable as a demonstration for evaluation purposes only.
* This code will be maintained at the sole discretion of Silicon Labs.
*
******************************************************************************/

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "sl_status.h"
#include "stdbool.h"

#define JOYSTICK_I2C_ADDRESS 0x20

#define JOYSTICK_READ_ID           0x00  /* Read ID command*/
#define JOYSTICK_READ_FWREV_1      0x01  /* Firmware Number */
#define JOYSTICK_READ_FWREV_2      0x02  /* Firmware Number */
#define JOYSTICK_READ_HORIZONTAL_POSITION_MSB  0x03  /* Current Joystick Horizontal Position (MSB) */
#define JOYSTICK_READ_HORIZONTAL_POSITION_LSB  0x04  /* Current Joystick Horizontal Position (LSB) */
#define JOYSTICK_READ_VERTICAL_POSITION_MSB    0x05  /* Current Joystick Vertical Position (MSB) */
#define JOYSTICK_READ_VERTICAL_POSITION_LSB    0x06  /* Current Joystick Vertical Position (LSB) */
#define JOYSTICK_READ_BUTTON_STATE      0x07  /* Current Button State (clears Reg 0x08) */
#define JOYSTICK_READ_BUTTON_STATUS     0x08  /* Indicator for if button was pressed since last
                                               * read of button state (Reg 0x07). Clears after read */

/**************************************************************************//**
 * @brief
 *  Initialize the Joystick.
 * @return
 *  @retval SL_STATUS_OK An joystick device is present on the I2C bus
 *  @retval SL_STATUS_INITIALIZATION No Joystick device present
 *****************************************************************************/
sl_status_t joystick_init(void);

/**************************************************************************//**
 * @brief
 *  Check whether a Joystick is present on the I2C bus or not.
 * @param[out] device_id
 *  Should be 0x20 for joystick.
 * @return
 *  @retval true A joystick device is present on the I2C bus
 *  @retval false No joystick device present
 *****************************************************************************/
bool joystick_present(uint8_t *device_id);

/**************************************************************************//**
 * @brief
 *   Send a command and read the result over the I2C bus
 *
 * @param[out] data
 *   The data read from the joystick.
 * @param[in] command
 *   The command to send to device. See the joystick.h file for details.
 * @return
 *   @retval SL_STATUS_OK Success
 *   @retval SL_STATUS_TRANSMIT I2C transmission error
 *****************************************************************************/
sl_status_t joystick_send_command(uint8_t *data, uint8_t command);

#endif /* __JOYSTICK_H__ */
