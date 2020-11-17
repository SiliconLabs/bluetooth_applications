/***************************************************************************//**
* @file barometer.h
* @brief Barometer driver header
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#ifndef BAROMETER_H_
#define BAROMETER_H_

#include "barometer_config.h"
#include "sl_status.h"
#include "sl_i2cspm.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup barometer Barometer Driver
 *
 * @brief Barometer Driver
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief Structure to configure the barometer sensor
 ******************************************************************************/
typedef struct {
  I2CSPM_Init_TypeDef   I2C_sensor;
  uint8_t               I2C_address;    /**< I2C address of the sensor */
  uint8_t               oversample_rate; /**< Over-sample index */
}barometer_init_t;

/// Default initialization structure for barometer driver
#define BAROMETER_INIT_DEFAULT            \
  {                                       \
   {                                      \
    BAROMETER_DEFAULT_I2C_INSTANCE,       \
    BAROMETER_DEFAULT_SCL_PORT,           \
    BAROMETER_DEFAULT_SCL_PIN,            \
    BAROMETER_DEFAULT_SDA_PORT,           \
    BAROMETER_DEFAULT_SDA_PIN,            \
    0,                                    \
    I2C_FREQ_STANDARD_MAX,                \
    i2cClockHLRStandard,                  \
   },                                     \
    BAROMETER_DEFAULT_I2C_ADDR,           \
    BAROMETER_DEFAULT_OVERSAMP_INDEX      \
  }

/// Inner state-machine states
typedef enum {
  BAROMETER_STATE_UNINITIALIZED,
  BAROMETER_STATE_STANDBY,
  BAROMETER_STATE_TEMP_CONVERSION,
  BAROMETER_STATE_TEMP_RDY,
  BAROMETER_STATE_PRESS_CONVERSION,
  BAROMETER_STATE_PRESS_RDY,
  BAROMETER_STATE_ERROR
}barometer_states_t;

/// Determines the type of the measurement conversion
typedef enum {
  BAROMETER_PRESSURE,
  BAROMETER_TEMPERATURE
}barometer_measurement_t;

/***************************************************************************//**
 * @brief
 *   Initializes I2C peripheral of the MCU. Also reset the actual sensor and
 *   configure it. Factory-programmed calibration values are read here.
 *
 * @param[in] init
 *   Pointer to the initialization structure
 *
 * @return
 *   Returns zero on OK, non-zero otherwise
 ******************************************************************************/
sl_status_t barometer_init(barometer_init_t* init);

/***************************************************************************//**
 * @brief
 *   Resets the sensor.
 *
 * @return
 *   Returns zero on OK, non-zero otherwise
 ******************************************************************************/
sl_status_t barometer_reset();

/***************************************************************************//**
 * @brief
 *   Configures the sensor with the given oversample rate.
 *
 * @param[in] osr
 *   Oversample rate
 *
 * @return  none
 ******************************************************************************/
void barometer_config(uint8_t osr);

/***************************************************************************//**
 * @brief
 *   Start the measurement process and waits for the results actively. This
 *   will block the device.
 *
 * @return
 *   Returns the compensated pressure data
 ******************************************************************************/
float barometer_get_pressure_blocking();

/***************************************************************************//**
 * @brief
 *   Start the measurement process. This function will not block the device,
 *   if the measurement is ready it will call the user function passed to the
 *   function as parameter with the result. This function uses the sleeptimer
 *   module.
 *
 * @param[in] user_cb
 *   User callback function
 *
 * @return
 *   Returns zero on OK, non-zero otherwise
 ******************************************************************************/
sl_status_t barometer_get_pressure_non_blocking(void (*user_cb)(float));

/***************************************************************************//**
 * @brief
 *   Starts a temperature or pressure conversion process. The time that a conversion needs
 *   depends on the oversample rate.
 *
 * @param[in] measurement_type
 *   Determines the type of the measurement (temperature or pressure)
 *
 * @return
 *   Returns zero on OK, non-zero otherwise
 ******************************************************************************/
sl_status_t barometer_start_conversion(barometer_measurement_t measurement_type);

/***************************************************************************//**
 * @brief
 *   Reads out the raw sensor data (temperature or pressure),
 *   if the conversion is ready.
 *
 * @return
 *   Returns the raw sensor measurement data
 ******************************************************************************/
uint32_t barometer_read_raw_conversion();

/***************************************************************************//**
 * @brief
 *   Calculates the conversion time that a measurement takes.
 *
 * @return
 *   Returns with the conversion time
 ******************************************************************************/
uint8_t barometer_get_conversion_time_in_millis();

/***************************************************************************//**
 * @brief
 *   This function is responsible for temperature compensation then converts
 *   raw sensor data to float values.
 *
 * @param[in] adc_temp
 *   Raw temperature data
 * @param[in] adc_press
 *   Raw pressure data
 * @param[out] temperature
 *   Temperature value
 * @param[out] pressure
 *   Compensated pressure value
 *
 * @return  none
 ******************************************************************************/
void barometer_calculate(uint32_t raw_temp, uint32_t raw_press, float *temperature, float *pressure);

/** @} (end addtogroup barometer) */

#ifdef __cplusplus
}
#endif

#endif /* BAROMETER_H_ */
