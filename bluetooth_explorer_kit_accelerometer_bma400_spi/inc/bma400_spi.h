/***************************************************************************//**
 * @file ssd1306_spi.h
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
#ifndef SSD1306_SPI_H
#define SSD1306_SPI_H

#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif


#define  BMA400_INT1_GPIO_PORT gpioPortB
#define  BMA400_INT1_GPIO_PIN 3

#define  BMA400_INT2_GPIO_PORT gpioPortB
#define  BMA400_INT2_GPIO_PIN 4

/***************************************************************************//**
 * @brief
 *   Initialize gpio used in the SPI interface.
 *
 * @detail
 *  The driver instances will be initialized automatically,
 *  during the sl_system_init() call in main.c.
 *****************************************************************************/
void ssd1306_spi_init(void);

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
sl_status_t bma400_write_to_register(uint8_t address, uint8_t data);

/***************************************************************************//**
 * @brief
 *    Write block of data to bma400.
 *
 * @param[in] address
 *    Address of the first Register to write.
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
sl_status_t bma400_block_write(uint8_t address, uint8_t length, 
                               const uint8_t *values);

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
sl_status_t bma400_read_from_register(uint8_t address, uint8_t *data);

/***************************************************************************//**
 * @brief
 *    Read block of data from bma400.
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
sl_status_t bma400_block_read(uint8_t address, uint8_t length, uint8_t *values);

#ifdef __cplusplus
}
#endif

#endif
