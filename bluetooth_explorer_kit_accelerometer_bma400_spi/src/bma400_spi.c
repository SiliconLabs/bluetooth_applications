/***************************************************************************//**
 * @file ssd1306_spi.c
 * @brief SPI abstraction used by SSD1306
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
* This code has been minimally tested to ensure that it builds with
* the specified dependency versions and is suitable as a demonstration
* for evaluation purposes only.
* This code will be maintained at the sole discretion of Silicon Labs.
*
******************************************************************************/
#include <bma400_spi.h>
#include "spidrv.h"
#include "sl_spidrv_instances.h"

#define spi_handle    sl_spidrv_mikroe_handle

static sl_status_t bma400_spi_write_byte_data(uint8_t address, uint8_t data);
static sl_status_t bma400_spi_read_byte_data(uint8_t address, uint8_t *data);
static sl_status_t bma400_spi_read_block_data(uint8_t address, uint8_t length,
                                              uint8_t* data);

/***************************************************************************//**
 * @brief
 *   Initialize gpio used in the SPI interface.
 *
 * @detail
 *  The driver instances will be initialized automatically,
 *  during the sl_system_init() call in main.c.
 *****************************************************************************/
void mba400_spi_init(void)
{

}

/***************************************************************************//**
 * @brief
 *    Write a byte to bma400 over SPI.
 *
 * @param[in] address
 *    Address of Register to write.
 *
 * @param[in] data
 *    The data write to the bma400.
 *
 * @return
 *    @ref SL_STATUS_OK on success.
 *    @ref On failure, SL_STATUS_TRANSMIT is returned.
 ******************************************************************************/
static sl_status_t bma400_spi_write_byte_data(uint8_t address, uint8_t data)
{
  uint8_t txBuffer[2];
  Ecode_t ret_code;

  txBuffer[0] = address & ~0x80;    // RWb = 0 for writes.
  txBuffer[1] = data;

  ret_code = SPIDRV_MTransmitB(spi_handle, txBuffer, 2);
  if (ret_code != ECODE_EMDRV_SPIDRV_OK) {
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Read a byte from bma400 over SPI.
 *
 * @param[in] address
 *    Address of Register to read.
 *
 * @param[in] data
 *    The data read from the bma400.
 *
 * @return
 *    @ref SL_STATUS_OK on success.
 *    @ref On failure, SL_STATUS_TRANSMIT is returned.
 ******************************************************************************/
static sl_status_t bma400_spi_read_byte_data(uint8_t address, uint8_t *data)
{
  uint8_t txBuffer[3];
  uint8_t rxBuffer[3];
  Ecode_t ret_code;

  txBuffer[0] = address | 0x80;    // RWb = 1 for reads
  txBuffer[1] = 0xff;   // dummy
  txBuffer[2] = 0xff;   // dummy

  ret_code = SPIDRV_MTransferB(spi_handle, txBuffer, rxBuffer, 3);
  if (ret_code != ECODE_EMDRV_SPIDRV_OK) {
    *data = 0;
    return SL_STATUS_TRANSMIT;
  }

  *data = rxBuffer[2];  // return data

  /* Note that at this point all the data is loaded into the fifo, this does
   * not necessarily mean that the transfer is complete. */
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Read block of data from bma400 over SPI.
 *
 * @param[in] address
 *    Address of the first Register to read.
 *
 * @param[in] length
 *    Number of registers to read.
 *
 * @param[out] data
 *    The data read from the bma400.
 *
 * @return
 *    @ref SL_STATUS_OK on success.
 *    @ref On failure, SL_STATUS_TRANSMIT is returned.
 ******************************************************************************/
static sl_status_t bma400_spi_read_block_data(uint8_t address, uint8_t length,
                                              uint8_t* data)
{
  uint8_t i;
  const uint16_t comm_length = length + 2;
  uint8_t txBuffer[comm_length];
  uint8_t rxBuffer[comm_length];
  Ecode_t ret_code;

  txBuffer[0] = address | 0x80;    // RWb = 1 for reads
  txBuffer[1] = 0xff; // dummy
  txBuffer[2] = 0xff; // dummy

  for(i = 3; i < comm_length; i++)
  {
    txBuffer[i] = 0xff;
  }

  ret_code = SPIDRV_MTransferB(spi_handle, txBuffer, rxBuffer, comm_length);
  if (ret_code != ECODE_EMDRV_SPIDRV_OK) {
    return SL_STATUS_TRANSMIT;
  }
  // Copy the receive payload (without the dummy byte) to the output buffer data
  for(i = 0; i < length; i++) {
    data[i] = rxBuffer[i+2];
  }

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Write a byte to bma400.
 *
 * @param[in] address
 *    Address of Register to write.
 *
 * @param[in] data
 *    The data write to the bma400.
 *
 * @return
 *    @ref SL_STATUS_OK on success.
 *    @ref On failure, SL_STATUS_TRANSMIT is returned.
 ******************************************************************************/
sl_status_t bma400_write_to_register(uint8_t address, uint8_t data)
{
  return bma400_spi_write_byte_data(address, data);
}

/***************************************************************************//**
 * @brief
 *    Write block of data to bma400.
 *
 * @param[in] address
 *    Address of the first Register to write.
 *
 * @note
 *    There is no address auto-increment.
 *
 * @param[in] length
 *    Number of registers to write.
 *
 * @param[in] data
 *    The data write to the bma400.
 *
 * @return
 *    @ref SL_STATUS_OK on success.
 *    @ref On failure, SL_STATUS_TRANSMIT is returned.
 ******************************************************************************/
sl_status_t bma400_block_write(uint8_t address, uint8_t length, const uint8_t *data)
{
  sl_status_t ret = SL_STATUS_OK;

  for (int count = 0; ((count < length) && (ret == SL_STATUS_OK)); count++) {
    ret = bma400_spi_write_byte_data(address, data[count]);
    address++;
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *    Read a byte from bma400.
 *
 * @param[in] address
 *    Address of Register to read.
 *
 * @param[in] data
 *    The data read from the bma400.
 *
 * @return
 *    @ref SL_STATUS_OK on success.
 *    @ref On failure, SL_STATUS_TRANSMIT is returned.
 ******************************************************************************/
sl_status_t bma400_read_from_register(uint8_t address, uint8_t *data)
{
  return bma400_spi_read_byte_data(address, data);
}

/***************************************************************************//**
 * @brief
 *    Read block of data from bma400.
 *
 * @param[in] address
 *    Address of the first Register to read.
 *
 * @note
 *    Address are automatically incremented after each read.
 *
 * @param[in] length
 *    Number of registers to read.
 *
 * @param[out] data
 *    The data read from the bma400.
 *
 * @return
 *    @ref SL_STATUS_OK on success.
 *    @ref On failure, SL_STATUS_TRANSMIT is returned.
 ******************************************************************************/
sl_status_t bma400_block_read(uint8_t address, uint8_t length, uint8_t *data)
{
  return bma400_spi_read_block_data(address, length, data);
}

