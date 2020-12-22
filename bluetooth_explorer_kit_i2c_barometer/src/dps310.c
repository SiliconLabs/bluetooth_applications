/***************************************************************************//**
* @file dps310.c
* @brief DPS310 sensor IC driver source
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
#include "barometer.h"
#include "sl_sleeptimer.h"

#define DPS310_BUS_ADDR             0x77        /**< I2C address of the DPS310 chip                               */
#define DPS310_CHIP_ID              0x10        /**< Device ID of the DPS310 chip                                 */

// Register address definitions
#define DPS310_REG_RESET            0x0C        /**< Reset register                                               */
#define DPS310_REG_PRSCFG           0x06        /**< Pressure configuration register                              */
#define DPS310_REG_TMPCFG           0x07        /**< Temperature configuration register                           */
#define DPS310_REG_MEASCFG          0x08        /**< Sensor Operating Mode and Status register                    */
#define DPS310_REG_CFGREG           0x09        /**< Interrupt and FIFO configuration register                    */
#define DPS310_REG_TMP_BASE         0x03        /**< Temperature Data base register                               */
#define DPS310_REG_PRS_BASE         0x00        /**< Pressure Data base register                                  */
#define DPS310_REG_TMPSRC           0x28        /**< Coefficient Source register                                  */
#define DPS310_REG_ID               0x0D        /**< Product and Revision ID register                             */
#define DPS310_REG_COEFF_BASE       0x10        /**< Calibration Coefficients register                            */

#define DPS310_COEFFICIENT_COUNT    9           /**< Number of coefficients                                       */
#define DPS310_COEFFICIENT_SIZE     18          /**< Size of all of the coefficients in bytes except c00 and c010 */

#define DPS310_CMD_RESET            0x89        /**< Reset command                                                */
#define DPS310_CMD_T_AND_P_SHIFT    0x0C        /**< Temperature and pressure result shift command                */

/// DPS310 Over-sample rate
typedef enum {
  DPS310_OSR_1,
  DPS310_OSR_2,
  DPS310_OSR_4,
  DPS310_OSR_8,
  DPS310_OSR_16,
  DPS310_OSR_32,
  DPS310_OSR_64,
  DPS310_OSR_128,
  DPS310_OSR_invalid
} dps310_osr_t;

// Measurement time depends on the selected over-sampling value
const uint8_t dps310_conversion_time[8] = { 4,    // OSR = 1
                                            7,    // OSR = 2
                                            10,   // OSR = 4
                                            17,   // OSR = 8
                                            30,   // OSR = 16
                                            59,   // OSR = 32
                                            112,  // OSR = 64
                                            220   // OSR = 128
};

// The over-sample factor is used in temperature compensation
static int32_t oversample_factor[] = {524288, 1572864, 3670016, 7864320,
                                      253952, 516096,  1040384, 2088960};

// Configuration structure for the dps310 sensor
static barometer_init_t dps310;

// Factory calibrated coefficients are saved for compensation
static int16_t coeffs[DPS310_COEFFICIENT_COUNT-2];
static int32_t coeff_c00, coeff_c10;

// Helper variables for the non-blocking sensor read
static uint32_t raw_temperature, raw_pressure;
static float temperature, pressure;
static barometer_states_t barometer_state = BAROMETER_STATE_UNINITIALIZED;
static sl_sleeptimer_timer_handle_t timer;

// The user callback which returns the sensor value in case of non-blocking sensor read
static void (*callback_user)(float);

// Local prototypes
static sl_status_t dps310_sensor_read(uint8_t target_address, uint8_t *rx_buff, uint8_t num_bytes);
static sl_status_t dps310_sensor_write(uint8_t target_address, uint8_t cmd);
static sl_status_t dps310_read_coeffs();
static sl_status_t dps310_present();
static int32_t dps310_decimal_conversion(int32_t raw_coeff, uint8_t length);
// Local callback function for the non-blocking sensor read function
static void timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

/**************************************************************************//**
 *  Initializes the DPS310 sensor
 *****************************************************************************/
sl_status_t barometer_init(barometer_init_t* init)
{
    if(barometer_state != BAROMETER_STATE_UNINITIALIZED){
      return SL_STATUS_INVALID_STATE;
    }

    uint8_t reg_value, shift_value=0;

    dps310.I2C_sensor = init->I2C_sensor;
    dps310.I2C_address = init->I2C_address;
    dps310.oversample_rate = init->oversample_rate;

    I2CSPM_Init(&init->I2C_sensor);

    barometer_reset();

    if(dps310_present() != SL_STATUS_OK) {
      return SL_STATUS_FAIL;
    }

    dps310_sensor_read(DPS310_REG_PRSCFG, &reg_value, 1);
    if(dps310_sensor_write(DPS310_REG_PRSCFG, reg_value | dps310.oversample_rate) != SL_STATUS_OK) {
      return SL_STATUS_FAIL;
    }

    dps310_sensor_read(DPS310_REG_TMPSRC, &reg_value, 1);
    if(reg_value & 0x80) {
      reg_value = 0x80;
    }

    if(dps310_sensor_write(DPS310_REG_TMPCFG, reg_value | dps310.oversample_rate) != SL_STATUS_OK) {
      return SL_STATUS_FAIL;
    }

    dps310_sensor_read(DPS310_REG_CFGREG, &reg_value, 1);

    if(dps310.oversample_rate > DPS310_OSR_8) {
      shift_value = DPS310_CMD_T_AND_P_SHIFT;
    }

    // T_SHIFT and P_SHIFT
    if(dps310_sensor_write(DPS310_REG_CFGREG, reg_value | shift_value) != SL_STATUS_OK) {
      return SL_STATUS_FAIL;
    }

    if(dps310_read_coeffs() != SL_STATUS_OK) {
      return SL_STATUS_FAIL;
    }

    sl_sleeptimer_init();
    barometer_state = BAROMETER_STATE_STANDBY;

    return SL_STATUS_OK;
}

/**************************************************************************//**
 *  Resets the DPS310 sensor.
 *****************************************************************************/
sl_status_t barometer_reset()
{
  return dps310_sensor_write(DPS310_REG_RESET, DPS310_CMD_RESET);
}

/**************************************************************************//**
 *  Configures the sensor with the given oversample rate.
 *****************************************************************************/
void barometer_config(dps310_osr_t measurement_rate)
{
  dps310.oversample_rate = measurement_rate;
}

/**************************************************************************//**
 *  Start the measurement process and waits for the results with sleeptimer.
 *****************************************************************************/
float barometer_get_pressure_blocking()
{
  uint32_t raw_temp, raw_press;
  float press, temp;

  barometer_start_conversion(BAROMETER_TEMPERATURE);
  sl_sleeptimer_delay_millisecond(barometer_get_conversion_time_in_millis());

  raw_temp = barometer_read_raw_conversion();
  barometer_start_conversion(BAROMETER_PRESSURE);
  sl_sleeptimer_delay_millisecond(barometer_get_conversion_time_in_millis());

  raw_press = barometer_read_raw_conversion();
  barometer_calculate(raw_temp, raw_press, &temp, &press);
  return press;
}

/**************************************************************************//**
 *  Start the measurement process. This function will not block the device.
 *****************************************************************************/
sl_status_t barometer_get_pressure_non_blocking(void (*user_cb)(float))
{
  uint32_t tick;

  if(barometer_state != BAROMETER_STATE_STANDBY){
    return SL_STATUS_INVALID_STATE;
  }

  callback_user = user_cb;
  barometer_start_conversion(BAROMETER_TEMPERATURE);

  tick = sl_sleeptimer_ms_to_tick(barometer_get_conversion_time_in_millis());

  return sl_sleeptimer_start_timer(&timer, tick, timer_callback, (void *)NULL, 0, 0);
}

/**************************************************************************//**
 *  Starts a temperature or pressure conversion process.
 *****************************************************************************/
sl_status_t barometer_start_conversion(barometer_measurement_t measurement_type)
{
  if(barometer_state != BAROMETER_STATE_STANDBY){
      return SL_STATUS_INVALID_STATE;
  }

  uint8_t reg_value;
  dps310_sensor_read(DPS310_REG_MEASCFG, &reg_value, 1);

  if(measurement_type == BAROMETER_TEMPERATURE){
      barometer_state = BAROMETER_STATE_TEMP_CONVERSION;
      return dps310_sensor_write(DPS310_REG_MEASCFG, reg_value | 0x02);
  }
  else if(measurement_type == BAROMETER_PRESSURE){
      barometer_state = BAROMETER_STATE_PRESS_CONVERSION;
      return dps310_sensor_write(DPS310_REG_MEASCFG, reg_value | 0x01);
  }

  return SL_STATUS_INVALID_PARAMETER;
}

/**************************************************************************//**
 *  Reads out the raw sensor data (temperature or pressure),
 *  if the conversion is ready.
 *****************************************************************************/
uint32_t barometer_read_raw_conversion()
{
  uint8_t i, result_buff[3];
  uint32_t result=0;

  dps310_sensor_read(DPS310_REG_MEASCFG, &i, 1);

  if((i & 0x20) && (barometer_state == BAROMETER_STATE_TEMP_CONVERSION)){
    for(i=0; i<3; i++)
      {
        dps310_sensor_read(DPS310_REG_TMP_BASE+i, result_buff+i, 1);
      }
      result = ((uint32_t)result_buff[0]<<16) | ((uint32_t)result_buff[1]<<8) | result_buff[2];
      barometer_state = BAROMETER_STATE_STANDBY;
      return result;
  }
  else if((i & 0x10) && (barometer_state == BAROMETER_STATE_PRESS_CONVERSION)){
    for(i=0; i<3; i++)
      {
        dps310_sensor_read(DPS310_REG_PRS_BASE+i, result_buff+i, 1);
      }
      result = ((uint32_t)result_buff[0]<<16) | ((uint32_t)result_buff[1]<<8) | result_buff[2];
      barometer_state = BAROMETER_STATE_STANDBY;
      return result;
  }

  // Invalid state
  return 0;
}

/**************************************************************************//**
 *  Calculates the conversion time that a measurement takes.
 *****************************************************************************/
uint8_t barometer_get_conversion_time_in_millis()
{
  if(dps310.oversample_rate >= DPS310_OSR_invalid)
    return 0;
  return dps310_conversion_time[dps310.oversample_rate];
}

/**************************************************************************//**
 *  This function is responsible for temperature compensation.
 *****************************************************************************/
void barometer_calculate(uint32_t adc_temp, uint32_t adc_press, float *temperature, float *pressure)
{
  float temp_scaled, press_scaled;
  temp_scaled = (float) dps310_decimal_conversion(adc_temp, 24);
  temp_scaled = temp_scaled / oversample_factor[dps310.oversample_rate];

  press_scaled = (float) dps310_decimal_conversion(adc_press, 24);
  press_scaled = press_scaled / oversample_factor[dps310.oversample_rate];

  *temperature = coeffs[0] / 2.0 + coeffs[1] * temp_scaled;

  *pressure = coeff_c00 + press_scaled * (coeff_c10 + press_scaled * ((int32_t)coeffs[4] + press_scaled * (int32_t)coeffs[6])) +
      temp_scaled * (int32_t)coeffs[2] +  temp_scaled * press_scaled * ((int32_t)coeffs[3] + press_scaled * (int32_t)coeffs[5]);
  *pressure /= 100;
}

/**************************************************************************//**
 *  Reads sensor ID to check if a DPS310 is present.
 *****************************************************************************/
static sl_status_t dps310_present()
{
  uint8_t id=0;

  while(!(id & 0xC0))
  {
      dps310_sensor_read(DPS310_REG_MEASCFG, &id, 1);
  }

  if( dps310_sensor_read(DPS310_REG_ID, &id, 1) == SL_STATUS_OK)
  {
    if(id == DPS310_CHIP_ID)
      return SL_STATUS_OK;
  }
  return SL_STATUS_FAIL;
}

/**************************************************************************//**
 *  Implements I2C master read function.
 *****************************************************************************/
static sl_status_t dps310_sensor_read(uint8_t target_address, uint8_t *rx_buff, uint8_t num_bytes)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef result;

  // Initializing I2C transfer
  seq.addr          = dps310.I2C_address << 1;

  seq.flags         = I2C_FLAG_WRITE_READ; // must write target address before reading
  seq.buf[0].data   = &target_address;
  seq.buf[0].len    = 1;
  seq.buf[1].data   = rx_buff;
  seq.buf[1].len    = num_bytes;

  result = I2CSPM_Transfer(dps310.I2C_sensor.port, &seq);

  if(result != i2cTransferDone)
  {
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

/**************************************************************************//**
 *  Implements I2C master write function.
 *****************************************************************************/
static sl_status_t dps310_sensor_write(uint8_t target_address, uint8_t cmd)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef result;
  uint8_t txBuffer[2];

  txBuffer[0] = target_address;
  txBuffer[1] = cmd;

  // Initializing I2C transfer
  seq.addr          = dps310.I2C_address << 1;;
  seq.flags         = I2C_FLAG_WRITE;
  seq.buf[0].data   = txBuffer;
  seq.buf[0].len    = 2;

  result = I2CSPM_Transfer(dps310.I2C_sensor.port, &seq);

  if(result != i2cTransferDone)
   {
     return SL_STATUS_FAIL;
   }
   return SL_STATUS_OK;
}

/**************************************************************************//**
 *  Reads factory programmed coefficients from the sensor's PROM.
 *****************************************************************************/
static sl_status_t dps310_read_coeffs()
{
  uint8_t status=0, i;
  uint8_t coeff_buffer[DPS310_COEFFICIENT_SIZE];
  uint16_t coeff_temp_16;
  uint32_t coeff_temp_32;

  while (!(status & (1<<7))){
      dps310_sensor_read(DPS310_REG_MEASCFG, &status, 1);
  }

      for(i=0; i<DPS310_COEFFICIENT_SIZE; i++)
      {
          dps310_sensor_read(DPS310_REG_COEFF_BASE+i, coeff_buffer+i, 1);
      }

      //c0 - 12 bit
      coeff_temp_16 = ((uint16_t)coeff_buffer[0] << 4) | (((uint16_t)coeff_buffer[1] >> 4) & 0x0F);
      coeffs[0] = (int16_t)dps310_decimal_conversion(coeff_temp_16, 12);
      //c1 - 12 bit
      coeff_temp_16 = (((uint16_t)coeff_buffer[1] & 0x0F) << 8) | coeff_buffer[2];
      coeffs[1] = (int16_t)dps310_decimal_conversion(coeff_temp_16, 12);

      coeff_temp_32 = ((uint32_t)coeff_buffer[3] << 12) | ((uint32_t)coeff_buffer[4] << 4) |
              (((uint32_t)coeff_buffer[5] >> 4) & 0x0F);
      coeff_c00 = dps310_decimal_conversion(coeff_temp_32, 20);

      coeff_temp_32 = (((uint32_t)coeff_buffer[5] & 0x0F) << 16) | ((uint32_t)coeff_buffer[6] << 8) |
              (uint32_t)coeff_buffer[7];
      coeff_c10 = dps310_decimal_conversion(coeff_temp_32, 20);

      for(i=0; i<DPS310_COEFFICIENT_COUNT-4; i++)
      {
        coeff_temp_16 = ((uint16_t)coeff_buffer[8+i*2] << 8) | (uint16_t)coeff_buffer[9+i*2];
        coeffs[i+2] = (int16_t)dps310_decimal_conversion(coeff_temp_16, 16);
      }

    return SL_STATUS_OK;
}

/**************************************************************************//**
 *  Perform decimal conversion on the two-s complement number.
 *****************************************************************************/
static int32_t dps310_decimal_conversion(int32_t raw_coeff, uint8_t length)
{
  if (raw_coeff > (int32_t)(((uint32_t)1 << (length - 1)) - 1))
  {
    raw_coeff -= (uint32_t)1 << length;
  }

  return raw_coeff;
}

/**************************************************************************//**
 *  Callback function for the non-blocking sensor read function.
 *****************************************************************************/
static void timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;
  uint32_t tick;

  if (barometer_state == BAROMETER_STATE_TEMP_CONVERSION){
      raw_temperature = barometer_read_raw_conversion();
      barometer_start_conversion(BAROMETER_PRESSURE);
      tick = sl_sleeptimer_ms_to_tick(barometer_get_conversion_time_in_millis());
      sl_sleeptimer_start_timer(&timer, tick, timer_callback, (void *)NULL, 0, 0);
  }
  else if (barometer_state == BAROMETER_STATE_PRESS_CONVERSION){
      raw_pressure = barometer_read_raw_conversion();
      barometer_calculate(raw_temperature, raw_pressure, &temperature, &pressure);
      barometer_state = BAROMETER_STATE_STANDBY;
      callback_user(pressure);
  }
}
