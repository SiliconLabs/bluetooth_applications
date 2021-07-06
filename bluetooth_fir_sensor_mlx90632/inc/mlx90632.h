/***************************************************************************//**
 * @file  mlx90632.h
 * @brief IrThermo 3 Click driver.
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

#ifndef MLX90632_H
#define MLX90632_H

#include <stdint.h>
#include <stdbool.h>
#include <sl_status.h>

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef BIT
#define BIT(x)(1UL << (x))
#endif

// 32 bit calibration parameters memory address
#define MLX90632_EE_P_R            (0x240c)
#define MLX90632_EE_P_G            (0x240e)
#define MLX90632_EE_P_T            (0x2410)
#define MLX90632_EE_P_O            (0x2412)
#define MLX90632_EE_Aa             (0x2414)
#define MLX90632_EE_Ab             (0x2416)
#define MLX90632_EE_Ba             (0x2418)
#define MLX90632_EE_Bb             (0x241a)
#define MLX90632_EE_Ca             (0x241c)
#define MLX90632_EE_Cb             (0x241e)
#define MLX90632_EE_Da             (0x2420)
#define MLX90632_EE_Db             (0x2422)
#define MLX90632_EE_Ea             (0x2424)
#define MLX90632_EE_Eb             (0x2426)
#define MLX90632_EE_Fa             (0x2428)
#define MLX90632_EE_Fb             (0x242a)
#define MLX90632_EE_Ga             (0x242c)

// 16 bit calibration parameters memory address
#define MLX90632_EE_Gb             (0x242e)
#define MLX90632_EE_Ka             (0x242f)
#define MLX90632_EE_Ha             (0x2481)
#define MLX90632_EE_Hb             (0x2482)

// Memory sections addresses
#define MLX90632_ADDR_RAM          (0x4000)
#define MLX90632_ADDR_EEPROM       (0x2480)
#define MLX90632_REG_CONTROL       (0x3001)

// EEPROM addresses - used at startup
#define MLX90632_EE_CTRL           (0x24d4)

#define MLX90632_EE_I2C_ADDRESS    (0x24d5)
#define MLX90632_EE_VERSION        (0x240b)

// RAM measure address-es
#define MLX90632_REG_RAM_1         (0x4000)
#define MLX90632_REG_RAM_2         (0x4001)
#define MLX90632_REG_RAM_3         (0x4002)
#define MLX90632_REG_RAM_4         (0x4003)
#define MLX90632_REG_RAM_5         (0x4004)
#define MLX90632_REG_RAM_6         (0x4005)
#define MLX90632_REG_RAM_7         (0x4006)
#define MLX90632_REG_RAM_8         (0x4007)
#define MLX90632_REG_RAM_9         (0x4008)

// RAM_MEAS address-es for each channel
#define MLX90632_RAM_1(meas_num)   (MLX90632_ADDR_RAM + 3 * meas_num)
#define MLX90632_RAM_2(meas_num)   (MLX90632_ADDR_RAM + 3 * meas_num + 1)
#define MLX90632_RAM_3(meas_num)   (MLX90632_ADDR_RAM + 3 * meas_num + 2)

// Device status register
#define   MLX90632_REG_STATUS       (0x3fff)
#define   MLX90632_STAT_DATA_RDY    BIT(0)

// Constants
#define MLX90632_EEPROM_VERSION     (0x8405)
#define NOT_MATCHING_EEPROM_VERSION (-6)
#define MLX90632_REF_12             (12.0)
#define MLX90632_REF_3              (12.0)
#define MLX90632_REG_STATUS_BITMASK (0x007C)
#define POW10                       (10000000000LL)
#define MLX90632_RESET_CMD          (0x0006)
#define MLX90632_EEPROM_UNLOCK_KEY  (0x554C)

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief Set other mode.
 *
 * @param[in] 0x02 - Sleeping step mode
 *            0x04 - Step mode
 *            0x06 - Continuous mode
 *
 * @retval  0 Successfully set mode.
 * @retval  None 0, Something went wrong.
 ******************************************************************************/
sl_status_t mlx90632_set_mode(uint16_t mod);

/***************************************************************************//**
 * @brief Initialize MLX90632 driver.
 *
 * EEPROM version is important to match sensor EEPROM content and calculations.
 * This is why this function checks for correct EEPROM version before it does
 * checksum validation of the EEPROM content.
 *
 * @retval  0 Successfully initialized MLX90632 driver.
 * @retval  None 0, Something went wrong.
 ******************************************************************************/
sl_status_t mlx90632_init(void);

/***************************************************************************//**
 * @brief Function for device reset.
 *
 * @retval  0 Successfully.
 * @retval  None 0, Something went wrong.
 ******************************************************************************/
sl_status_t mlx90632_addressed_reset(void);

/***************************************************************************//**
 * @brief  Function for unlock EEPROM.
 *
 * @retval  0 Successfully unlock the EEPROM.
 * @retval  None 0, Something went wrong.
 ******************************************************************************/
sl_status_t mlx90632_unlock_eeprom(void);

/***************************************************************************//**
 * @brief  Function gives back both temperature values. Contains all necessary
 *         functions for the measurement.
 *
 * @param[out] *ambient Pointer
 *             to memory location where ambient temperature value is stored.
 * @param[out] *object Pointer
 *             to memory location where object temperature value is stored.
 *
 * @retval  0 Successfully get temperature values.
 * @retval  None 0, Something went wrong.
 ******************************************************************************/
sl_status_t mlx90632_measurment_cb(double *ambient, double *object);

#endif // MLX90632_H
