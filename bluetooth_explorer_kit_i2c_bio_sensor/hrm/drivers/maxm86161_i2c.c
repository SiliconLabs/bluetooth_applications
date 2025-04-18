/***************************************************************************//**
 * @file maxm86161_i2c.c
 * @brief I2C device setup for maxm86161 driver.
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
 * This code has been minimally tested to ensure that it builds with
 * the specified dependency versions and is suitable as a demonstration
 * for evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

#include "sl_i2cspm.h"
#include "sl_i2cspm_mikroe_config.h"
#include "stdio.h"
#include "string.h"
#include "hrm/config/maxm86161_hrm_config.h"
#include "maxm86161_i2c.h"

/**************************************************************************//**
 * @brief Write a byte to Maxim register.
 *****************************************************************************/
sl_status_t maxm86161_i2c_write_to_register(uint8_t address, uint8_t data)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_write_data[2];
  uint8_t i2c_read_data[1];

  seq.addr = MAXM86161_SLAVE_ADDRESS;
  seq.flags = I2C_FLAG_WRITE;

  /* Select register and data to write */
  i2c_write_data[0] = address;
  i2c_write_data[1] = data;
  seq.buf[0].data = i2c_write_data;
  seq.buf[0].len = 2;

  /* Select length of data to be read */
  seq.buf[1].data = i2c_read_data;
  seq.buf[1].len = 0;

  ret = I2CSPM_Transfer(SL_I2CSPM_MIKROE_PERIPHERAL, &seq);
  if (ret != i2cTransferDone) {
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Read a byte from Maxim register.
 *****************************************************************************/
sl_status_t maxm86161_i2c_read_from_register(uint8_t address, uint8_t *data)
{
  I2C_TransferSeq_TypeDef    seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_write_data[1];

  seq.addr = MAXM86161_SLAVE_ADDRESS;
  seq.flags = I2C_FLAG_WRITE_READ;
  i2c_write_data[0] = address;
  seq.buf[0].data = i2c_write_data;
  seq.buf[0].len = 1;

  /* Select length of data to be read */
  seq.buf[1].data = data;
  seq.buf[1].len = 1;

  ret = I2CSPM_Transfer(SL_I2CSPM_MIKROE_PERIPHERAL, &seq);
  if (ret != i2cTransferDone) {
    *data = 0x00;
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Write a Block to Maxim.
 *****************************************************************************/
sl_status_t maxm86161_i2c_block_write(uint8_t address,
                                      uint8_t length,
                                      uint8_t const *data)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_write_data[length + 1];
  uint8_t i2c_read_data[1];

  seq.addr = MAXM86161_SLAVE_ADDRESS;
  seq.flags = I2C_FLAG_WRITE;

  /* Select register and data to write */
  i2c_write_data[0] = address;
  for (int i = 0; i < length; i++) {
    i2c_write_data[i + 1] = data[i];
  }
  seq.buf[0].data = i2c_write_data;
  seq.buf[0].len = length + 1;

  /* Select length of data to be read */
  seq.buf[1].data = i2c_read_data;
  seq.buf[1].len = 0;
  ret = I2CSPM_Transfer(SL_I2CSPM_MIKROE_PERIPHERAL, &seq);
  if (ret != i2cTransferDone) {
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Read a Block from Maxim.
 *****************************************************************************/
sl_status_t maxm86161_i2c_block_read(uint8_t address,
                                     uint16_t length,
                                     uint8_t *data)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t i2c_write_data[1];

  seq.addr = MAXM86161_SLAVE_ADDRESS;
  seq.flags = I2C_FLAG_WRITE_READ;

  i2c_write_data[0] = address;
  seq.buf[0].data = i2c_write_data;
  seq.buf[0].len = 1;

  /* Select length of data to be read */
  seq.buf[1].data = data;
  seq.buf[1].len = length;
  ret = I2CSPM_Transfer(SL_I2CSPM_MIKROE_PERIPHERAL, &seq);
  if (ret != i2cTransferDone) {
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}
