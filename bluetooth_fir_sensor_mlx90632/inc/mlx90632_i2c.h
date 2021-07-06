/***************************************************************************//**
 * @file  mlx90632_i2c.h
 * @brief IrThermo 3 Click i2c communication.
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#ifndef MLX90632_I2C_H
#define MLX90632_I2C_H

#include <stdint.h>
#include <stdbool.h>

// Definition of I2C address of MLX90632
#define SLAVE_ADDRESS     0x3A << 1

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief Read the register_address 16 bit value from the mlx90632.
 *
 * @param[in]  register_address Address of the register to be read from.
 * @param[out] *value pointer to where read data can be written.
 *
 * @retval  0 for Success.
 * @retval <0 for Failure.
 *
 ******************************************************************************/
int32_t mlx90632_i2c_read(int16_t register_address, uint16_t *value);

/***************************************************************************//**
 * @brief Read the register_address 32 bit value from the mlx90632.
 *
 * @param[in]  register_address Address of the register to be read from.
 * @param[out] *value pointer to where read data can be written.
 *
 * @retval  0 for Success.
 * @retval <0 for Failure.
 *
 ******************************************************************************/
int32_t mlx90632_i2c_read32(int16_t register_address, uint32_t *value);

/***************************************************************************//**
 * @brief Write value to register_address of the mlx90632.
 *
 * @param[in] register_address Address of the register to be read from.
 * @param[in] value value to be written to register address of mlx90632.
 *
 * @retval  0 for Success.
 * @retval <0 for Failure.
 ******************************************************************************/
int32_t mlx90632_i2c_write(int16_t register_address, uint16_t value);

#endif // MLX90632_I2C_H
