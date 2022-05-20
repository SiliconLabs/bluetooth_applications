/***************************************************************************//**
 * @file sl_bma400_i2c.h
 * @brief I2C abstraction used by BMA400
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef SL_BMA400_I2C_H
#define SL_BMA400_I2C_H

#include "sl_status.h"
#include "sl_i2cspm.h"
#include "bma400.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup BMA400 - Accelerometer Sensor
 * @brief Driver for the Bosch Sensortec BMA400 Accelerometer sensor


   @n @section bma400_example BMA400 example

     Basic example for performing Accelerometer measurement.
     @verbatim

   #include "sl_i2cspm_instances.h"
   #include "sl_bma400_i2c.h"
   #include "bma400.h"

   static struct bma400_dev bma;

   int main( void )
   {

     ...

     struct bma400_sensor_conf conf[2] = { { 0 } };
     struct bma400_int_enable int_en[2];
     struct bma400_sensor_data data;
     uint16_t int_status = 0;

     bma400_i2c_init(sl_i2cspm_mikroe, BMA400_I2C_ADDRESS_SDO_HIGH, &bma);
     bma400_soft_reset(&bma);
     bma400_init(&bma);
     // Select the type of configuration to be modified
     conf.type = BMA400_ACCEL;

     // Get the accelerometer configurations which are set in the sensor
     bma400_get_sensor_conf(&conf, 1, &bma);

     // Modify the desired configurations as per macros
     // available in bma400_defs.h file
     conf.param.accel.odr = BMA400_ODR_100HZ;
     conf.param.accel.range = BMA400_RANGE_2G;
     conf.param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;

     // Set the desired configurations to the sensor
     bma400_set_sensor_conf(&conf, 1, &bma);

     bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);

     int_en.type = BMA400_DRDY_INT_EN;
     int_en.conf = BMA400_ENABLE;

     bma400_enable_interrupt(&int_en, 1, &bma);

     // Start read data from bma400
     bma400_get_interrupt_status(&int_status, &bma);
     if (int_status & BMA400_ASSERTED_DRDY_INT) {
       bma400_get_accel_data(BMA400_DATA_SENSOR_TIME, &data, &bma);
     }

     ...

   } @endverbatim

 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *  Initialize I2C Interface for the BMA400.
 *
 * @param[in] i2cspm
 *  The I2CSPM instance to use.
 * @param[in] bma400_i2c_addr
 *  The I2C address of the BMA400.
 * @param[out] bma400
 *  The BMA400 device structure.
 *
 * @return
 *  @ref BMA400_OK on success.
 *  @ref On failure, BMA400_E_NULL_PTR is returned.
 ******************************************************************************/
int8_t bma400_i2c_init(sl_i2cspm_t *i2cspm,
                       uint8_t bma400_i2c_addr,
                       struct bma400_dev *bma400);

/** @} (end addtogroup BMA400) */

#ifdef __cplusplus
}
#endif

#endif
