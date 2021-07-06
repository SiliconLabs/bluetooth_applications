/***************************************************************************//**
 * @file  mlx90632.c
 * @brief IrThremo 3 Click driver.
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

#include <mlx90632.h>
#include <mlx90632_i2c.h>
#include <math.h>


/// MLX90632 calibration variables
static int16_t ambient_new_raw;
static int16_t ambient_old_raw;
static int16_t object_new_raw;
static int16_t object_old_raw;

static int32_t PR;
static int32_t PG;
static int32_t PT;
static int32_t PO;
static int32_t Ea;
static int32_t Eb;
static int32_t Fa;
static int32_t Fb;
static int32_t Ga;

static int16_t Gb;
static int16_t Ka;
static int16_t Ha;
static int16_t Hb;

/***************************************************************************//**
 *                            LOCAL PROTOTYPES
 ******************************************************************************/
// Implementation of reading all calibration parameters.
static int32_t mlx90632_read_eeprom(int32_t *PR, int32_t *PG, int32_t *PO,
                                    int32_t *PT, int32_t *Ea, int32_t *Eb,
                                    int32_t *Fa, int32_t *Fb, int32_t *Ga,
                                    int16_t *Gb, int16_t *Ka, int16_t *Ha,
                                    int16_t *Hb);

// Read ambient raw old and new values.
static int32_t mlx90632_read_temp_ambient_raw(int16_t *ambient_new_raw,
                                              int16_t *ambient_old_raw);

// Pre-calculations for ambient.
static double mlx90632_preprocess_temp_ambient(int16_t ambient_new_raw,
                                               int16_t ambient_old_raw,
                                               int16_t Gb);
// Ambient temperature calculations.
static double mlx90632_calc_temp_ambient(int16_t ambient_new_raw,
                                         int16_t ambient_old_raw, int32_t P_T,
                                         int32_t P_R, int32_t P_G, int32_t P_O,
                                         int16_t Gb);

// Clear REG_STATUS new_data bit.
static int32_t clear_data_available(void);

// Give back the measurement mode.
static uint8_t get_mode(void);

// Set the measurement mode to CONTINUOUS.
static int32_t set_mode_continuous(void);

// Set SOC bit
static int32_t set_soc_bit(void);

// Blocking delay.
static void sleep(int value);

// Assert REG_STATUS and give back cyclic state.
static int mlx90632_start_measurement(void);

// RAM select.
static int32_t mlx90632_channel_new_select(int32_t ret, uint8_t *channel_new,
                                           uint8_t *channel_old);

// Read object raw old and new values.
static int32_t mlx90632_read_temp_object_raw(int32_t start_measurement_ret,
                                             int16_t *object_new_raw,
                                             int16_t *object_old_raw);

// Read new and old  ambient and object values from sensor.
static int32_t mlx90632_read_temp_raw(int16_t *ambient_new_raw,
                                      int16_t *ambient_old_raw,
                                      int16_t *object_new_raw,
                                      int16_t *object_old_raw);

// Pre-calculations for object temperature.
static double mlx90632_preprocess_temp_object(int16_t object_new_raw,
                                              int16_t object_old_raw,
                                              int16_t ambient_new_raw,
                                              int16_t ambient_old_raw,
                                              int16_t Ka);

// Calculations for object temperature.
static double mlx90632_calc_temp_object_iteration(double prev_object_temp,
                                                  int32_t object, double TAdut,
                                                  int32_t Ga, int32_t Fa,
                                                  int32_t Fb, int16_t Ha,
                                                  int16_t Hb, double emissivity);

// Calculation of TAdut and iterate calculations.
static double mlx90632_calc_temp_object(int32_t object, int32_t ambient,
                                        int32_t Ea, int32_t Eb, int32_t Ga,
                                        int32_t Fa, int32_t Fb,
                                        int16_t Ha, int16_t Hb);
// Check the eeprom.
static uint16_t eeprom_busy(void);
/***************************************************************************//**
 * @brief Read the eeprom registers value from the mlx90632.
 *
 * @param[out]  P_R calibration constant
 * @param[out]  P_G calibration constant
 * @param[out]  P_O calibration constant
 * @param[out]  P_T calibration constant
 * @param[out]  Ea calibration constant
 * @param[out]  Eb calibration constant
 * @param[out]  Fa calibration constant
 * @param[out]  Fb calibration constant
 * @param[out]  Ga calibration constant
 * @param[out]  Gb calibration constant
 * @param[out]  Ka calibration constant
 * @param[out]  Ha Customer calibration constant
 * @param[out]  Hb Customer calibration constant
 *
 * @retval  0 Successfully read eeprom register.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t mlx90632_read_eeprom(int32_t *PR, int32_t *PG, int32_t *PO,
                                    int32_t *PT, int32_t *Ea, int32_t *Eb,
                                    int32_t *Fa, int32_t *Fb, int32_t *Ga,
                                    int16_t *Gb, int16_t *Ka, int16_t *Ha,
                                    int16_t *Hb)
{
  int32_t ret;
  uint8_t mode;
  uint32_t counter;

  counter = 0;
  while (eeprom_busy() >= 1)
  {
      sleep(32000);
      counter++;

      if (counter >=1000) {
        break;
      }
  }

  mlx90632_set_mode(0x02);

  ret = mlx90632_i2c_read32(MLX90632_EE_P_R, (uint32_t *) PR);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_P_G, (uint32_t *) PG);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_P_O, (uint32_t *) PO);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_P_T, (uint32_t *) PT);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_Ea, (uint32_t *) Ea);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_Eb, (uint32_t *) Eb);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_Fa, (uint32_t *) Fa);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_Fb, (uint32_t *) Fb);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read32(MLX90632_EE_Ga, (uint32_t *) Ga);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read(MLX90632_EE_Gb, (uint16_t *) Gb);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read(MLX90632_EE_Ha, (uint16_t *) Ha);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read(MLX90632_EE_Hb, (uint16_t *) Hb);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_read(MLX90632_EE_Ka, (uint16_t *) Ka);
  if (ret < 0) {
    return ret;
  }

  // Get measurement mode
  mode = get_mode();

  // Set measuring mode to continuous
  if (mode != 3) {
    set_mode_continuous();
  }

  return ret;
}

/***************************************************************************//**
 * @brief Read ambient raw old and new values.
 *
 * @param[out] *ambient_new_raw Pointer
 *             to memory location where new ambient value from sensor is stored.
 * @param[out] *ambient_old_raw Pointer
 *             to memory location where old ambient value from sensor is stored.
 *
 * @retval  0 Successfully read both values.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t mlx90632_read_temp_ambient_raw(int16_t *ambient_new_raw,
                                       int16_t *ambient_old_raw)
{
  int32_t ret;
  uint16_t read_tmp;

  ret = mlx90632_i2c_read(MLX90632_REG_RAM_6, &read_tmp);
  if (ret < 0) {
    return ret;
  }
  *ambient_new_raw = (int16_t)read_tmp;

  ret = mlx90632_i2c_read(MLX90632_REG_RAM_9, &read_tmp);
  if (ret < 0) {
    return ret;
  }
  *ambient_old_raw = (int16_t)read_tmp;

  return ret;
}

/***************************************************************************//**
 * @brief Pre-calculations for ambient temperature.
 *
 * @param[in] *ambient_new_raw Pointer to
 *             memory location where new ambient value from sensor is stored.
 * @param[in] *ambient_old_raw  Pointer
 *             to memory location where old ambient value from sensor is stored.
 * @param[in] Gb calibration constant.
 *
 * @retval Calculated AMB value.
 ******************************************************************************/
static double mlx90632_preprocess_temp_ambient(int16_t ambient_new_raw,
                                        int16_t ambient_old_raw, int16_t Gb)
{
  double VR_Ta, kGb;

  kGb = (double)Gb * 0.0009765625;
  VR_Ta = ambient_old_raw + kGb * (ambient_new_raw / 12.0);

  return ((ambient_new_raw / (12.0) ) / VR_Ta) * 524288.0;
}

/***************************************************************************//**
 * @brief Ambient temperature calculations.
 *
 * @param[in] ambient_new_raw value.
 * @param[in] ambient_old_raw value.
 * @param[in] P_T calibration constant.
 * @param[in] P_R calibration constant.
 * @param[in] P_G calibration constant.
 * @param[in] P_O calibration constant.
 * @param[in] Gb calibration constant.
 *
 * @retval Calculated ambient temperature value in C.
 ******************************************************************************/
static double mlx90632_calc_temp_ambient(int16_t ambient_new_raw,
                                  int16_t ambient_old_raw, int32_t P_T,
                                  int32_t P_R, int32_t P_G,
                                  int32_t P_O, int16_t Gb)
{
  double AMB, ambientTemp;

  AMB = mlx90632_preprocess_temp_ambient(ambient_new_raw,
                                           ambient_old_raw, Gb);

  P_T = P_T * 5.6843418860808015E-14;
  P_R = P_R * 0.00390625;
  P_G = P_G * 0.000000953;
  P_O = P_O * 0.00390625;

  ambientTemp = P_O + (AMB - P_R) / P_G + P_T * (AMB - P_R)*(AMB - P_R);

  return ambientTemp;
}

/***************************************************************************//**
 * @brief Clear REG_STATUS new_data bit.
 *
 * @retval  0 Successfully read and write values.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t clear_data_available(void)
{
  int32_t ret;
  uint16_t reg_status;

  ret = mlx90632_i2c_read(MLX90632_REG_STATUS, &reg_status);
  if (ret < 0) {
    return ret;
  }

  ret = mlx90632_i2c_write(MLX90632_REG_STATUS,
                           reg_status & (~MLX90632_STAT_DATA_RDY));

  return ret;
}

/***************************************************************************//**
 * @brief Get measurement modes.
 *
 * Sleeping step mode: In this mode the device will be by default in sleep.
 * On request (soc bit), the device will power-on, the state machine
 * will do one measurement, will go into sleep and will wait for next command.
 *
 * Step mode: In this mode the state machine will do one measurement upon
 * request (soc bit) and will wait for next command.
 * The device remains powered all time in this mode.
 *
 * Continuous mode: Measurements are executed continuously. The device
 * remains powered all time in this mode.
 *
 * @retval  1: Enables the sleeping step mode.
 * @retval  2: Enables the step mode.
 * @retval  3: Device is in continuous mode.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static uint8_t get_mode(void)
{
  int32_t ret;
  uint16_t mode;

  ret = mlx90632_i2c_read(MLX90632_REG_CONTROL, &mode);
  if (ret < 0) {
    return ret;
  }

  mode = (mode >> 1) & 0x0003;
  return (uint8_t)mode;
}

/***************************************************************************//**
 * @brief Set CONTINUOUS measurement mode.
 *
 * Continuous mode: Measurements are executed continuously. The device
 * remains powered all time in this mode.
 *
 * @retval  0 Successfully read and write values.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t set_mode_continuous(void)
{
  int32_t ret;
  uint16_t mode;

  ret = mlx90632_i2c_read(MLX90632_REG_CONTROL, &mode);
  if (ret < 0) {
    return ret;
  }

  mode |= 0x06;

  ret = mlx90632_i2c_write(MLX90632_REG_CONTROL, mode);

  return ret;
}

/***************************************************************************//**
 * @brief Starts a measurement when being in (sleeping) step mode
 *
 * In step mode the state machine will execute
 * only one measurement which is initiated by soc bit.
 *
 * @retval  0 Successfully read and write values.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t set_soc_bit()
{
  int32_t ret;
  uint16_t soc;

  ret = mlx90632_i2c_read(MLX90632_REG_CONTROL, &soc);
  soc |= (1 << 3);
  ret = mlx90632_i2c_write(MLX90632_REG_CONTROL, soc);

  return ret;
}

/***************************************************************************//**
 * @brief Delay, blocking.
 *
 * @param[in] value (0-32,767).
 ******************************************************************************/
static void sleep(int value)
{
  while(--value);
}

/***************************************************************************//**
 * @brief Assert REG_STATUS and give back
 *        the position of the measurement (Cycle_pos).
 *
 * @return  1: Cycle_pos = 1.
 * @return  2: Cycle_pos = 2.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int mlx90632_start_measurement(void)
{
  int ret, tries = 7500;
  uint16_t reg_status;

  ret = mlx90632_i2c_read(MLX90632_REG_STATUS, &reg_status);
  if (ret < 0) {
    return ret;
  }

  ret = clear_data_available();
  if (ret < 0) {
    return ret;
  }

  while (tries-- > 0) {
    ret = mlx90632_i2c_read(MLX90632_REG_STATUS, &reg_status);
    if (ret < 0) {
      return ret;
    }

    if (reg_status & MLX90632_STAT_DATA_RDY) {
      break;
    }

    sleep(10000);
  }

  if (tries < 0) {
    return -1;
  }

  return (reg_status & MLX90632_REG_STATUS_BITMASK) >> 2;
}

/***************************************************************************//**
 * @brief Set helper number for the RAM_MEAS address-es for each channel.
 *
 * @param[in]  ret should get the Cyclic_pos which can be 1 or 2.
 * @param[out] *channel_new Pointer to memory location
 *             where channel_new value is stored.
 * @param[out] *channel_old Pointer to memory location
 *             where channel_old value is stored.
 *
 * @retval  0 Successfully set.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t mlx90632_channel_new_select(int32_t ret,
                                           uint8_t *channel_new,
                                           uint8_t *channel_old)
{
  switch (ret)
  {
    case 1:
      *channel_new = 1;
      *channel_old = 2;
      break;

    case 2:
      *channel_new = 2;
      *channel_old = 1;
      break;

    default:
      return -1;
  }
  return 0;
}

/***************************************************************************//**
 * @brief Read object raw old and new values based on
 *        mlx90632_start_measurement retval value.
 *
 * @param[in]  start_measurement_ret should get the Cyclic_pos which can be 1/2.
 * @param[out] *object_new_raw Pointer
 *             to memory location where new ambient value from sensor is stored.
 * @param[out] *object_old_raw Pointer
 *             to memory location where old ambient value from sensor is stored.
 *
 * @retval  0 Successfully read both values.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t mlx90632_read_temp_object_raw(int32_t start_measurement_ret,
                                             int16_t *object_new_raw,
                                             int16_t *object_old_raw)
{
  int32_t ret;
  uint16_t read_tmp;
  int16_t read;
  uint8_t channel, channel_old;

  ret = mlx90632_channel_new_select(start_measurement_ret,
                                    &channel, &channel_old);
  if (ret != 0) {
    return ret;
  }

  ret = mlx90632_i2c_read(MLX90632_RAM_2(channel), &read_tmp);
  if (ret < 0) {
    return ret;
  }

  read = (int16_t)read_tmp;

  ret = mlx90632_i2c_read(MLX90632_RAM_1(channel), &read_tmp);
  if (ret < 0) {
    return ret;
  }
  *object_new_raw = (read + (int16_t)read_tmp) / 2;

  ret = mlx90632_i2c_read(MLX90632_RAM_2(channel_old), &read_tmp);
  if (ret < 0) {
    return ret;
  }
  read = (int16_t)read_tmp;

  ret = mlx90632_i2c_read(MLX90632_RAM_1(channel_old), &read_tmp);
  if (ret < 0) {
    return ret;
  }
  *object_old_raw = (read + (int16_t)read_tmp) / 2;

  return ret;
}

/***************************************************************************//**
 * @brief Read new and old  ambient and object values from sensor.
 *
 * @param[out] *ambient_new_raw Pointer
 *             to memory location where new ambient value from sensor is stored.
 * @param[out] *ambient_old_raw Pointer
 *             to memory location where old ambient value from sensor is stored.
 * @param[out] *object_new_raw Pointer
 *             to memory location where new ambient value from sensor is stored.
 * @param[out] *object_old_raw Pointer
 *             to memory location where old ambient value from sensor is stored.
 *
 * @retval  0 Successfully read the values.
 * @retval <0 Something went wrong.
 ******************************************************************************/
static int32_t mlx90632_read_temp_raw(int16_t *ambient_new_raw,
                                      int16_t *ambient_old_raw,
                                      int16_t *object_new_raw,
                                      int16_t *object_old_raw)
{
  int32_t ret, start_measurement_ret;
  int8_t mode;

  mode = get_mode();
  if (mode != 0x06) {
      set_soc_bit();
  }

  // Read new and old **ambient** values from sensor
  ret = mlx90632_read_temp_ambient_raw(ambient_new_raw, ambient_old_raw);
  if (ret < 0) {
    return ret;
  }

  start_measurement_ret = mlx90632_start_measurement();
  if (start_measurement_ret < 0) {
    return start_measurement_ret;
  }

  // Read new and old **object** values from sensor
  ret = mlx90632_read_temp_object_raw(start_measurement_ret,
                                      object_new_raw, object_old_raw);

  return ret;
}

/***************************************************************************//**
 * @brief Pre-calculations for object temperature.
 *
 * @param[in] object_new_raw value.
 * @param[in] object_old_raw value.
 * @param[in] ambient_new_raw value.
 * @param[in] ambient_old_raw value.
 * @param[in] Ka calibration constant.
 *
 * @retval Calculated STO value.
 ******************************************************************************/
static double mlx90632_preprocess_temp_object(int16_t object_new_raw,
                                              int16_t object_old_raw,
                                              int16_t ambient_new_raw,
                                              int16_t ambient_old_raw,
                                              int16_t Ka)
{
  double VR_IR, kKa;

  kKa = ((double)Ka) / 1024.0;
  VR_IR = ambient_old_raw + kKa * (ambient_new_raw / (MLX90632_REF_3));

  // Return STo=(S/12)/VRTO*2^19
  return ((((object_new_raw + object_old_raw) / 2) /
         (MLX90632_REF_12)) / VR_IR) * 524288.0;
}

/***************************************************************************//**
 * @brief Calculations for object temperature.
 *
 * @param[in] prev_object_temp should be 25.0 C.
 * @param[in] object STO value.
 * @param[in] TAdut value.
 * @param[in] Ga calibration constant.
 * @param[in] Fa calibration constant.
 * @param[in] Fb calibration constant.
 * @param[in] Ha calibration constant.
 * @param[in] Hb calibration constant.
 * @param[in] emissivity, default 1.0.
 *
 * @retval Calculated object temperature value in Celsius.
 ******************************************************************************/
static double mlx90632_calc_temp_object_iteration(double prev_object_temp,
                                                  int32_t object, double TAdut,
                                                  int32_t Ga, int32_t Fa,
                                                  int32_t Fb, int16_t Ha,
                                                  int16_t Hb, double emissivity)
{
  double calcedGa, calcedGb, calcedFa, TAdut4, first_sqrt;
  // temp variables
  double KsTAtmp, Alpha_corr;
  double Ha_customer, Hb_customer;

  Ha_customer = Ha / ((double)16384.0);
  Hb_customer = Hb / ((double)16384.0);

  calcedGa = ((double)Ga * (prev_object_temp - 25)) / ((double)68719476736.0);
  KsTAtmp = (double)Fb * (TAdut - 25);
  calcedGb = KsTAtmp / ((double)68719476736.0);

  Alpha_corr = (((double)(Fa * POW10)) * Ha_customer *
               (double)(1 + calcedGa + calcedGb)) /
               ((double)70368744177664.0);

  calcedFa = object / (emissivity * (Alpha_corr / POW10));

  // TA^4
  TAdut4 = (TAdut + 273.15) * (TAdut + 273.15) *
           (TAdut + 273.15) * (TAdut + 273.15);

  first_sqrt = sqrt(calcedFa + TAdut4);

  return (sqrt(first_sqrt) - 273.15 - Hb_customer);
}

/***************************************************************************//**
 * @brief Calculation of TAdut and iterate calculations.
 *
 * @param[in] object STO value.
 * @param[in] ambient AMB value.
 * @param[in] Ea calibration constant.
 * @param[in] Eb calibration constant.
 * @param[in] Ga calibration constant.
 * @param[in] Fa calibration constant.
 * @param[in] Fb calibration constant.
 * @param[in] Ha calibration constant.
 * @param[in] Hb calibration constant.
 *
 * @retval Calculated object temperature value in Celsius.
 ******************************************************************************/
static double mlx90632_calc_temp_object(int32_t object, int32_t ambient,
                                        int32_t Ea, int32_t Eb, int32_t Ga,
                                        int32_t Fa, int32_t Fb,
                                        int16_t Ha, int16_t Hb)
{
  double kEa, kEb, TAdut;
  double temp = 25.0;
  double tmp_emi = 1.0;
  int8_t i;

  kEa = ((double)Ea) / ((double)65536.0);
  kEb = ((double)Eb) / ((double)256.0);
  TAdut = (((double)ambient) - kEb) / kEa + 25;

  //iterate through calculations (minimum 3)
  for (i = 0; i < 5; ++i) {
    temp = mlx90632_calc_temp_object_iteration(temp, object, TAdut, Ga,
                                               Fa, Fb, Ha, Hb, tmp_emi);
  }
  return temp;
}

/***************************************************************************//**
 * @brief Flag indicating that the eeprom is busy.
 *
 * Eeprom being busy is defined as follows:
 * - at start-up, the eeprom is busy and remains busy till initialization phase
 * (eeprom copy) has finished
 * - during eeprom write/erase, the eeprom is busy
 *
 * @retval  EEPROM state value.
 ******************************************************************************/
static uint16_t eeprom_busy(void)
{
  int32_t ret;
  uint16_t reg_status;
  uint16_t value;

  ret = mlx90632_i2c_read(MLX90632_REG_STATUS, &reg_status);
  if (ret < 0) {
    return ret;
  }

  value = reg_status & (uint16_t)(0x0001 << 9);

  return value;
}

/***************************************************************************//**
 *                            PUBLIC FUNCTIONS
 ******************************************************************************/

// Set other mode
sl_status_t mlx90632_set_mode(uint16_t mod)
{
  sl_status_t sc = SL_STATUS_OK;
  int32_t ret;
  uint16_t mode;

  ret = mlx90632_i2c_read(MLX90632_REG_CONTROL, &mode);
  if (ret < 0) {
    sc = SL_STATUS_FAIL;
  }

  mode &= ~(0x0003 << 1); //Clear the mode bits
  mode |= (mod << 1);

  ret = mlx90632_i2c_write(MLX90632_REG_CONTROL, mode);
  if (ret < 0) {
    sc = SL_STATUS_FAIL;
  }

  return sc;
}

// Initialize MLX90632 driver, confirm EEPROM version
sl_status_t mlx90632_init(void)
{
  sl_status_t sc = SL_STATUS_OK;
  int32_t ret;
  uint16_t eeprom_version, reg_status;

  ret = mlx90632_i2c_read(MLX90632_EE_VERSION, &eeprom_version);
  if (ret < 0){
    sc = SL_STATUS_FAIL;
  }

  if (eeprom_version != MLX90632_EEPROM_VERSION) {
    return NOT_MATCHING_EEPROM_VERSION;
  }

  ret = mlx90632_i2c_read(MLX90632_REG_STATUS, &reg_status);
  if (ret < 0) {
    sc = SL_STATUS_FAIL;
  }

  // Prepare a clean start with setting NEW_DATA to 0
  ret = mlx90632_i2c_write(MLX90632_REG_STATUS,
                           reg_status & ~(MLX90632_STAT_DATA_RDY));
  if (ret < 0) {
    sc = SL_STATUS_FAIL;
  }

  // Read the eeprom registers value from the mlx90632
  ret = mlx90632_read_eeprom(&PR, &PG, &PO, &PT,
                             &Ea, &Eb, &Fa, &Fb,
                             &Ga, &Gb, &Ka, &Ha, &Hb);
  if (ret < 0) {
    sc = SL_STATUS_FAIL;
  }

  return sc;
}

// Function for device reset
sl_status_t mlx90632_addressed_reset(void)
{
  sl_status_t sc = SL_STATUS_OK;
  int32_t ret;

  ret = mlx90632_i2c_write(0x3005, MLX90632_RESET_CMD);

  if (ret < 0) {
    sc = SL_STATUS_FAIL;
  }

  return sc;
}

//  Function for unlock EEPROM
sl_status_t mlx90632_unlock_eeprom(void)
{
  sl_status_t sc = SL_STATUS_OK;
  int32_t ret;

  ret = mlx90632_i2c_write(0x3005, MLX90632_EEPROM_UNLOCK_KEY);

  if (ret < 0) {
    sc = SL_STATUS_FAIL;
  }

  return sc;
}

// Function gives back both temperature values.
sl_status_t mlx90632_measurment_cb(double *ambient,double *object)
{
  sl_status_t sc = SL_STATUS_OK;
  int32_t ret;
  double pre_ambient, pre_object;

  ret = mlx90632_read_temp_raw(&ambient_new_raw, &ambient_old_raw,
                               &object_new_raw, &object_old_raw);

  if (ret < 0) {
    return sc = SL_STATUS_FAIL;
  }

  pre_ambient = mlx90632_preprocess_temp_ambient(ambient_new_raw,
                                                 ambient_old_raw, Gb);

  *ambient = mlx90632_calc_temp_ambient(ambient_new_raw, ambient_old_raw, PT,
                                        PR, PG, PO, Gb);

  pre_object = mlx90632_preprocess_temp_object(object_new_raw, object_old_raw,
                                               ambient_new_raw, ambient_old_raw,
                                               Ka);

  *object = mlx90632_calc_temp_object(pre_object, pre_ambient, Ea,
                                      Eb, Ga, Fa, Fb, Ha, Hb);

  return sc;
}
