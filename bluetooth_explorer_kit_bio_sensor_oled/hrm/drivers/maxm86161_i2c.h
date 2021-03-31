/***************************************************************************//**
* @file maxm86161_i2c.h
* @brief Header file of maxm86161_i2c
* @version 1.0.0
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
* This code has been minimally tested to ensure that it builds with the specified dependency versions
* and is suitable as a demonstration for evaluation purposes only.
* This code will be maintained at the sole discretion of Silicon Labs.
*
******************************************************************************/
#ifndef MAXM86161_I2C_H_
#define MAXM86161_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_status.h"
#include "em_gpio.h"
/** I2C device address for MAX86161 */
#define MAXM86161_SLAVE_ADDRESS         0xC4

#define MAXM86161_EN_GPIO_PORT          gpioPortC
#define MAXM86161_EN_GPIO_PIN           3

/***************************************************************************//**
 * @brief
 *   Write a byte to the sensor via I2C.
 * @param[in] address
 *   The register address of the sensor.
 * @param[in] data
 *   Data in transfer.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_TRANSMIT on failure
 ******************************************************************************/
sl_status_t maxm86161_i2c_write_to_register( uint8_t address, uint8_t data);

/***************************************************************************//**
 * @brief
 *   Read a byte from the sensor via I2C.
 * @param[in] address
 *   The register address of the sensor.
 * @param[out] data
 *   Data in transfer.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_TRANSMIT on failure
 ******************************************************************************/
sl_status_t maxm86161_i2c_read_from_register(uint8_t address, uint8_t* data);

/***************************************************************************//**
 * @brief
 *   Write a block to the sensor over SPI interface.
 * @param[in] address
 *   The register address of the sensor.
 * @param[in] length
 *   Length of buffer in transfer.
 * @param[in] data
 *   Data buffer.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_TRANSMIT on failure
 ******************************************************************************/
sl_status_t maxm86161_i2c_block_write(uint8_t address, uint8_t length, uint8_t const *data);

/***************************************************************************//**
 * @brief
 *   Read a block from the sensor over SPI interface.
 * @param[in] address
 *   The register address of the sensor.
 * @param[in] length
 *   Length of buffer in transfer.
 * @param[out] data
 *   Data buffer.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_TRANSMIT on failure
 ******************************************************************************/
sl_status_t maxm86161_i2c_block_read(uint8_t address, uint16_t length, uint8_t* data);

#ifdef __cplusplus
}
#endif

#endif

