/***************************************************************************//**
 * @file joystick.c
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

#include <joystick.h>
#include "sl_i2cspm.h"
#include "sl_i2cspm_qwiic_config.h"
#include "em_gpio.h"
#include "sl_sleeptimer.h"

/**************************************************************************//**
 * @brief
 *  Initialize the Joystick.
 * @return
 *  @retval SL_STATUS_OK An joystick device is present on the I2C bus
 *  @retval SL_STATUS_INITIALIZATION No Joystick device present
 *****************************************************************************/
sl_status_t joystick_init(void)
{
  sl_status_t status = SL_STATUS_OK;

  if (!joystick_present(NULL)) {
    /* Wait for joystick to become ready */
    sl_sleeptimer_delay_millisecond(80);

    if (!joystick_present(NULL)) {
      status = SL_STATUS_INITIALIZATION;
    }
  }

  return status;
}

/**************************************************************************//**
 * @brief
 *  Check whether a Joystick is present on the I2C bus or not.
 * @param[out] device_id
 *  Should be 0x20 for joystick.
 * @return
 *  @retval true A joystick device is present on the I2C bus
 *  @retval false No joystick device present
 *****************************************************************************/
bool joystick_present(uint8_t *device_id)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t                    i2c_read_data[1];
  uint8_t                    i2c_write_data[1];

  seq.addr  = JOYSTICK_I2C_ADDRESS << 1;
  seq.flags = I2C_FLAG_WRITE_READ;
  /* Select command to issue */
  i2c_write_data[0] = JOYSTICK_READ_ID; // Read ID
  seq.buf[0].data   = i2c_write_data;
  seq.buf[0].len    = 1;
  /* Select location/length of data to be read */
  seq.buf[1].data = i2c_read_data;
  seq.buf[1].len  = 1;

  ret = I2CSPM_Transfer(SL_I2CSPM_QWIIC_PERIPHERAL, &seq);
  if (ret != i2cTransferDone) {
    return false;
  }

  if (NULL != device_id) {
    *device_id = i2c_read_data[0];
  }
  return true;
}

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
sl_status_t joystick_send_command(uint8_t *data, uint8_t command)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t                    i2c_read_data[1];
  uint8_t                    i2c_write_data[1];

  seq.addr  = JOYSTICK_I2C_ADDRESS << 1;
  seq.flags = I2C_FLAG_WRITE_READ;
  /* Select command to issue */
  i2c_write_data[0] = command;
  seq.buf[0].data   = i2c_write_data;
  seq.buf[0].len    = 1;
  /* Select location/length of data to be read */
  seq.buf[1].data = i2c_read_data;
  seq.buf[1].len  = 1;

  ret = I2CSPM_Transfer(SL_I2CSPM_QWIIC_PERIPHERAL, &seq);

  if (ret != i2cTransferDone) {
    *data = 0;
    return SL_STATUS_TRANSMIT;
  }
  *data = i2c_read_data[0];
  return SL_STATUS_OK;
}
