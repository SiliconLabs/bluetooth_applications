/***************************************************************************//**
 * @file bma400.c
 * @brief BMA400 driver
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
#include "bma400.h"
#include "bma400_spi.h"
#include "sl_sleeptimer.h"

#define size_of_array(array) (sizeof(array)/sizeof(array[0]))

static sl_status_t set_sensor_conf(uint8_t *data,
                                   const bma400_sensor_conf_t *conf);
static sl_status_t set_accel_conf(const bma400_acc_conf_t *accel_conf);
static sl_status_t set_tap_conf(const bma400_tap_conf_t *tap_set);
static sl_status_t set_activity_change_conf(const bma400_act_ch_conf_t *act_ch_set);
static sl_status_t set_gen1_int(const bma400_gen_int_conf_t *gen_int_set);
static sl_status_t set_gen2_int(const bma400_gen_int_conf_t *gen_int_set);
static sl_status_t set_orient_int(const bma400_orient_int_conf_t *orient_conf);
static sl_status_t set_autowakeup_timeout(const bma400_auto_wakeup_conf_t *wakeup_conf);
static sl_status_t set_auto_wakeup(uint8_t conf);
static sl_status_t set_autowakeup_interrupt(const bma400_wakeup_conf_t *wakeup_conf);
static sl_status_t set_auto_low_power(const bma400_auto_lp_conf_t *auto_lp_conf);
static sl_status_t set_int_pin_conf(bma400_int_pin_conf_t int_conf);
static sl_status_t set_fifo_conf(const bma400_fifo_conf_t *fifo_conf);

static sl_status_t get_autowakeup_timeout(bma400_auto_wakeup_conf_t *wakeup_conf);
static sl_status_t get_autowakeup_interrupt(bma400_wakeup_conf_t *wakeup_conf);
static sl_status_t get_auto_low_power(bma400_auto_lp_conf_t *auto_lp_conf);
static sl_status_t get_sensor_conf(const uint8_t *data, bma400_sensor_conf_t *conf);
static sl_status_t get_accel_conf(bma400_acc_conf_t *accel_conf);
static sl_status_t get_fifo_length(uint16_t *fifo_byte_cnt);
static sl_status_t get_tap_conf(bma400_tap_conf_t *tap_set);
static sl_status_t get_int_pin_conf(bma400_int_pin_conf_t *int_conf);
static sl_status_t get_activity_change_conf(bma400_act_ch_conf_t *act_ch_set);
static sl_status_t get_fifo_conf(bma400_fifo_conf_t *fifo_conf);
static sl_status_t get_gen1_int(bma400_gen_int_conf_t *gen_int_set);
static sl_status_t get_gen2_int(bma400_gen_int_conf_t *gen_int_set);
static sl_status_t get_orient_int(bma400_orient_int_conf_t *orient_conf);
static sl_status_t read_fifo(bma400_fifo_data_t *fifo);
static void check_mapped_interrupts(uint8_t int_1_map, uint8_t int_2_map,
                                    bma400_int_chan_t *int_map);
static void check_frame_available(const bma400_fifo_data_t *fifo,
                                  uint8_t *frame_available,
                                  uint8_t accel_width,
                                  uint8_t data_en,
                                  uint16_t *data_index);
static void unpack_accel(const bma400_fifo_data_t *fifo,
                         bma400_sensor_data_t *accel_data,
                         uint16_t *data_index,
                         uint8_t accel_width,
                         uint8_t frame_header);
static void unpack_sensortime_frame(bma400_fifo_data_t *fifo, uint16_t *data_index);
static void get_int_pin_map(const uint8_t *data_array,
                            uint8_t int_enable,
                            bma400_int_chan_t *int_map);
static void map_int_pin(uint8_t *data_array, uint8_t int_enable,
                        bma400_int_chan_t int_map);
static sl_status_t  enable_self_test(void);
static sl_status_t positive_excited_accel(bma400_sensor_data_t *accel_pos);
static sl_status_t negative_excited_accel(bma400_sensor_data_t *accel_neg);
static sl_status_t validate_accel_self_test(const bma400_sensor_data_t *accel_pos,
                                            const bma400_sensor_data_t *accel_neg);

/**************************************************************************//**
 *                              PUBLIC FUNCTIONS
 *****************************************************************************/

/**************************************************************************//**
 * @brief Initialize bma400
 *  This API reads the chip-id of the sensor which is the first step to verify
 *  the sensor and also it configures the read mechanism of SPI and I2C interface.
 *  As this API is the entry point, call this API before using other APIs.
 * 
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_init(void)
{
  uint8_t chip_id = 0;
  sl_status_t ret;

  /* Initial power-up time */
  bma400_delay_ms(5);

  ret = bma400_soft_reset();
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Dummy read of Chip-ID in SPI mode */
  ret = bma400_read_from_register(BMA400_REG_CHIP_ID, &chip_id);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Check for chip id validity */
  if (chip_id != BMA400_CHIP_ID) {
    return SL_STATUS_NOT_FOUND;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Soft reset bma400
 *  Soft-resets the sensor where all the registers are reset.
 *  to their default values except 0x4B.
 * 
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_soft_reset(void)
{
  sl_status_t ret;
  uint8_t data;

  /* Reset the device */
  ret = bma400_write_to_register(BMA400_REG_CMD, BMA400_SOFT_RESET_CMD);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  bma400_delay_ms(BMA400_DELAY_MS_SOFT_RESET);

  /* Dummy read of 0x7F register to enable SPI Interface if SPI is used */
  ret = bma400_read_from_register(0x7F, &data);

  return ret;
}

/**************************************************************************//**
 * @brief Set power mode to bma400
 * @note Possible value for power_mode :
 * @code
 *   BMA400_NORMAL_MODE
 *   BMA400_SLEEP_MODE
 *   BMA400_LOW_POWER_MODE
 * @endcode
 * 
 * @param[in] power_mode
 *  Select power mode of the sensor.
 * 
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_set_power_mode(bma400_power_mode_t power_mode)
{
  sl_status_t ret;
  uint8_t reg_data = 0;

  ret = bma400_read_from_register(BMA400_REG_ACC_CONFIG_0, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  reg_data = BMA400_SET_BITS_POS_0(reg_data, BMA400_POWER_MODE, power_mode);

  /* Set the power mode of sensor */
  ret = bma400_write_to_register(BMA400_REG_ACC_CONFIG_0, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  if (power_mode == BMA400_MODE_LOW_POWER) {
    /* A delay of 1/ODR is required to switch power modes
     * Low power mode has 25Hz frequency and hence it needs
     * 40ms delay to enter low power mode
     */
    bma400_delay_ms(40);
  }
  else {
    bma400_delay_ms(10);
  }

  return SL_STATUS_OK;
}


/**************************************************************************//**
 * @brief Get the power mode of the sensor.
 * @note Possible value for power_mode :
 * @code
 *   BMA400_NORMAL_MODE
 *   BMA400_SLEEP_MODE
 *   BMA400_LOW_POWER_MODE
 * @endcode
 *
 * @param[out] power_mode
 *  Power mode of the sensor.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_power_mode(bma400_power_mode_t *power_mode)
{
  sl_status_t ret;
  uint8_t reg_data;

  /* Proceed if null check is not fine */
  if (power_mode == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  ret = bma400_read_from_register(BMA400_REG_STATUS, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  *power_mode = BMA400_GET_BITS(reg_data, BMA400_POWER_MODE_STATUS);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the accelerometer data along with the sensor-time
 *
 * @param[in] data_sel
 *  Select the data to be read. Assignable values for data_sel:
 *   - BMA400_DATA_ONLY
 *   - BMA400_DATA_SENSOR_TIME
 * 
 * @param[out] accel
 *  Structure instance to store the accel data.
 * 
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_accel_data(data_selection_t data_sel, 
                                  bma400_sensor_data_t *accel)
{
  sl_status_t ret;
  uint8_t data_array[9] = { 0 };
  uint16_t lsb;
  uint8_t msb;
  uint8_t time_0;
  uint16_t time_1;
  uint32_t time_2;

  /* Proceed if null check is not fine */
  if (accel == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  if (data_sel == BMA400_DATA_ONLY) {
    /* Read the sensor data registers only */
    ret = bma400_block_read(BMA400_REG_ACC_X_LSB, 6, data_array);
  }
  else if (data_sel == BMA400_DATA_SENSOR_TIME) {
    /* Read the sensor data along with sensor time */
    ret = bma400_block_read(BMA400_REG_ACC_X_LSB, 9, data_array);
  }
  else {
    /* Invalid use of "data_sel" */
    ret = SL_STATUS_INVALID_CONFIGURATION;
  }
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  lsb = data_array[0];
  msb = data_array[1];

  /* accel X axis data */
  accel->x = (int16_t)(((uint16_t)msb * 256) + lsb);
  if (accel->x > 2047) {
      /* Computing accel data negative value */
      accel->x = accel->x - 4096;
  }

  lsb = data_array[2];
  msb = data_array[3];

  /* accel Y axis data */
  accel->y = (int16_t)(((uint16_t)msb * 256) | lsb);
  if (accel->y > 2047) {
      /* Computing accel data negative value */
      accel->y = accel->y - 4096;
  }

  lsb = data_array[4];
  msb = data_array[5];

  /* accel Z axis data */
  accel->z = (int16_t)(((uint16_t)msb * 256) | lsb);
  if (accel->z > 2047) {
      /* Computing accel data negative value */
      accel->z = accel->z - 4096;
  }

  if (data_sel == BMA400_DATA_ONLY) {
      /* Update sensortime as 0 */
      accel->sensortime = 0;
  }

  if (data_sel == BMA400_DATA_SENSOR_TIME) {
      /* Sensor-time data*/
      time_0 = data_array[6];
      time_1 = ((uint16_t)data_array[7] << 8);
      time_2 = ((uint32_t)data_array[8] << 16);
      accel->sensortime = (uint32_t)(time_2 + time_1 + time_0);
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Reads the FIFO data from the sensor.
 *  This action clears all data in the FIFO.
 *
 * @param[out] fifo
 *  Pointer to the FIFO structure.
 *
 * @note User has to allocate the FIFO buffer along with
 * corresponding FIFO read length from his side before calling this API
 *
 * @note User must specify the number of bytes to read from the FIFO in
 * fifo->length , It will be updated by the number of bytes actually
 * read from FIFO after calling this API
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_fifo_data(bma400_fifo_data_t *fifo)
{
  sl_status_t ret;
  uint8_t data;
  uint16_t fifo_byte_cnt = 0;
  uint16_t user_fifo_len = 0;

  /* Proceed if null check is not fine */
  if (fifo == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  /* Resetting the FIFO data byte index */
  fifo->accel_byte_start_idx = 0;

  /* Reading the FIFO length */
  ret = get_fifo_length(&fifo_byte_cnt);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Get the FIFO configurations from the sensor */
  ret = bma400_read_from_register(BMA400_REG_FIFO_CONFIG_0, &data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Get the data from FIFO_CONFIG0 register */
  fifo->fifo_8_bit_en = BMA400_GET_BITS(data, BMA400_FIFO_8_BIT_EN);
  fifo->fifo_data_enable = BMA400_GET_BITS(data, BMA400_FIFO_AXES_EN);
  fifo->fifo_time_enable = BMA400_GET_BITS(data, BMA400_FIFO_TIME_EN);
  fifo->fifo_sensor_time = 0;
  user_fifo_len = fifo->length;
  if (fifo->length > fifo_byte_cnt) {
    /* Handling case where user requests
     * more data than available in FIFO
     */
    fifo->length = fifo_byte_cnt;
  }

  /* Reading extra bytes as per the macro
   * "BMA400_FIFO_BYTES_OVERREAD"
   * when FIFO time is enabled
   */
  if ((fifo->fifo_time_enable == BMA400_ENABLE) &&
      (fifo_byte_cnt + BMA400_FIFO_BYTES_OVERREAD <= user_fifo_len)) {
    /* Handling sensor time availability*/
    fifo->length = fifo->length + BMA400_FIFO_BYTES_OVERREAD;
  }

  /* Read the FIFO data */
  ret = read_fifo(fifo);

  return ret;
}

/**************************************************************************//**
 * @brief This API parses and extracts the accelerometer frames, FIFO time
 * and control frames from FIFO data read by the "bma400_get_fifo_data" API
 * and stores it in the "accel_data" structure instance.
 *
 * @note The bma400_extract_accel API should be called only after
 * reading the FIFO data by calling the bma400_get_fifo_data() API
 *
 * @param[out] fifo
 *  Pointer to the FIFO structure.
 *
 * @param[out] accel_data
 *  Structure instance of bma400_sensor_data where the accelerometer data
 *  from FIFO is extracted and stored after calling this API.
 *
 * @param[out] frame_count
 * Number of valid accelerometer frames requested by user is given
 * as input and it is updated by the actual frames parsed from the FIFO.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_extract_accel(bma400_fifo_data_t *fifo,
                                 bma400_sensor_data_t *accel_data,
                                 uint16_t *frame_count)
{
  uint8_t frame_header = 0; /* Frame header information is stored */
  uint8_t accel_width; /* Accel data width is stored */
  uint16_t data_index; /* Data index of the parsed byte from FIFO */
  uint16_t accel_index = 0; /* Number of accel frames parsed */
  uint8_t frame_available = BMA400_ENABLE; /* Variable to check frame availability */

  /* Proceed if null check is fine */
  if ((fifo == NULL) || (accel_data == NULL) || (frame_count == NULL)) {
    return SL_STATUS_NULL_POINTER;
  }

  /* Check if this is the first iteration of data unpacking
   * if yes, then consider dummy byte on SPI
   */
  if (fifo->accel_byte_start_idx == 0) {
      /* Dummy byte included */
      //fifo->accel_byte_start_idx = 1;
  }

  for (data_index = fifo->accel_byte_start_idx; data_index < fifo->length;) {
    /*Header byte is stored in the variable frame_header*/
    frame_header = fifo->data[data_index];
    /* Store the Accel 8 bit or 12 bit mode */
    accel_width = BMA400_GET_BITS(frame_header, BMA400_FIFO_8_BIT_EN);
    /* Exclude the 8/12 bit mode data from frame header */
    frame_header = frame_header & BMA400_AWIDTH_MASK;
    /*Index is moved to next byte where the data is starting*/
    data_index++;
    switch (frame_header) {
      case BMA400_FIFO_XYZ_ENABLE:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_XYZ_ENABLE, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Extract and store accel xyz data */
          unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
          accel_index++;
        }
        break;

      case BMA400_FIFO_X_ENABLE:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_X_ENABLE, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Extract and store accel x data */
          unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
          accel_index++;
        }
        break;

      case BMA400_FIFO_Y_ENABLE:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_Y_ENABLE, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Extract and store accel y data */
          unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
          accel_index++;
        }
        break;

      case BMA400_FIFO_Z_ENABLE:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_Z_ENABLE, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Extract and store accel z data */
          unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
          accel_index++;
        }
        break;

      case BMA400_FIFO_XY_ENABLE:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_XY_ENABLE, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Extract and store accel xy data */
          unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
          accel_index++;
        }
        break;

      case BMA400_FIFO_YZ_ENABLE:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_YZ_ENABLE, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Extract and store accel yz data */
          unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
          accel_index++;
        }
        break;

      case BMA400_FIFO_XZ_ENABLE:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_YZ_ENABLE, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Extract and store accel xz data */
          unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
          accel_index++;
        }
        break;

      case BMA400_FIFO_SENSOR_TIME:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_SENSOR_TIME, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Unpack and store the sensor time data */
          unpack_sensortime_frame(fifo, &data_index);
        }
        break;

      case BMA400_FIFO_EMPTY_FRAME:
        /* Update the data index as complete */
        data_index = fifo->length;
        break;

      case BMA400_FIFO_CONTROL_FRAME:
        check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_CONTROL_FRAME, &data_index);
        if (frame_available != BMA400_DISABLE) {
          /* Store the configuration change data from FIFO */
          fifo->conf_change = fifo->data[data_index++];
        }
          break;

      default:
        /* Update the data index as complete */
        data_index = fifo->length;
        break;

    }

    if (*frame_count == accel_index) {
      /* Frames read completely*/
      break;
    }
  }

  /* Update the data index */
  fifo->accel_byte_start_idx = data_index;

  /* Update number of accel frame index */
  *frame_count = accel_index;

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Writes the fifo_flush command into command register.
 *  This action clears all data in the FIFO.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_set_fifo_flush(void)
{
  sl_status_t ret;
  uint8_t data = BMA400_FIFO_FLUSH_CMD;

  /* FIFO flush command is set */
  ret = bma400_write_to_register(BMA400_REG_CMD, data);

  return ret;
}

/**************************************************************************//**
 * @brief Get sensor configuration.
 *  Get the sensor settings like sensor configurations and interrupt 
 *  configurations and store them in the corresponding structure instance.
 * 
 * @param[out] conf
 *  Structure instance of the configuration structure.
 * 
 * @param[in] n_sett
 *  Number of settings to be set.
 * 
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_sensor_conf(bma400_sensor_conf_t *conf, uint16_t n_sett)
{
  sl_status_t ret = SL_STATUS_OK;
  uint16_t idx;
  uint8_t data_array[3] = { 0 };

  /* Proceed if null check is not fine */
  if (conf == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  /* Read the interrupt pin mapping configurations */
  ret = bma400_block_read(BMA400_REG_INT1_MAP, 3, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  for (idx = 0; ((idx < n_sett) && (ret == SL_STATUS_OK)); idx++) {
    ret = get_sensor_conf(data_array, conf + idx);
  }

  return ret;
}

/**************************************************************************//**
 * @brief Set sensor configuration.
 *  Set the sensor settings such as:
 *    - Accelerometer configuration (Like ODR,OSR,range...)
 *    - Tap configuration
 *    - Activity change configuration
 *    - Gen1/Gen2 configuration
 *    - Orientation change configuration
 *    - Step counter configuration
 * @note Before calling this API, fill in the value of 
 *   the required configurations in the conf structure.
 * 
 * @param[in] conf
 *  Structure instance of the configuration structure.
 * 
 * @param[in] n_sett
 *  Number of settings to be set.
 * 
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_set_sensor_conf(const bma400_sensor_conf_t *conf, 
                                   uint16_t n_sett)
{
  sl_status_t ret = SL_STATUS_OK;
  uint16_t idx = 0;
  uint8_t data_array[3] = { 0 };

  /* Proceed if null check is not fine */
  if (conf == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  /* Read the interrupt pin mapping configurations */
  ret = bma400_block_read(BMA400_REG_INT1_MAP, 3, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  for (idx = 0; ((idx < n_sett) && (ret == SL_STATUS_OK)); idx++) {
    ret = set_sensor_conf(data_array, conf + idx);
  }
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Set the interrupt pin mapping configurations */
  ret = bma400_block_write(BMA400_REG_INT1_MAP, 3, data_array);

  return ret;
}

/**************************************************************************//**
 * @brief Get the device specific settings and store them
 * in the corresponding structure instance.
 *
 * @param[out] conf
 *  Structure instance of the configuration structure.
 *
 * @param[out] n_sett
 *  Number of settings to be obtained.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_device_conf(bma400_device_conf_t *conf, uint8_t n_sett)
{
  sl_status_t ret;
  uint16_t idx = 0;
  uint8_t data_array[3] = { 0 };

  /* Proceed if null check is not fine */
  if (conf == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  /* Read the interrupt pin mapping configurations */
  ret = bma400_block_read(BMA400_REG_INT1_MAP, 3, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  for (idx = 0; (idx < n_sett) && (ret == SL_STATUS_OK); idx++) {
    switch (conf[idx].type) {
      case BMA400_AUTOWAKEUP_TIMEOUT:
        ret = get_autowakeup_timeout(&conf[idx].param.auto_wakeup);
        break;

      case BMA400_AUTOWAKEUP_INT:
        ret = get_autowakeup_interrupt(&conf[idx].param.wakeup);
        if (ret != SL_STATUS_OK) {
          return ret;
        }
        /* Get the INT pin mapping */
        get_int_pin_map(data_array,
                        BMA400_WAKEUP_INT_MAP,
                        &conf[idx].param.wakeup.int_chan);
        break;

      case BMA400_AUTO_LOW_POWER:
        ret = get_auto_low_power(&conf[idx].param.auto_lp);
        break;

      case BMA400_INT_PIN_CONF:
        ret = get_int_pin_conf(&conf[idx].param.int_conf);
        break;

      case BMA400_INT_OVERRUN_CONF:
        get_int_pin_map(data_array,
                        BMA400_INT_OVERRUN_MAP,
                        &conf[idx].param.overrun_int.int_chan);
        break;

      case BMA400_FIFO_CONF:
        ret = get_fifo_conf(&conf[idx].param.fifo_conf);
        if (ret != SL_STATUS_OK) {
          return ret;
        }
        get_int_pin_map(data_array,
                        BMA400_FIFO_FULL_INT_MAP,
                        &conf[idx].param.fifo_conf.fifo_full_channel);
        get_int_pin_map(data_array,
                        BMA400_FIFO_WM_INT_MAP,
                        &conf[idx].param.fifo_conf.fifo_wm_channel);
        break;
    }
  }

  return ret;
}

/**************************************************************************//**
 * @brief Set device configuration.
 * Set the device specific settings like:
 *  - BMA400_AUTOWAKEUP_TIMEOUT
 *  - BMA400_AUTOWAKEUP_INT
 *  - BMA400_AUTO_LOW_POWER
 *  - BMA400_INT_PIN_CONF
 *  - BMA400_INT_OVERRUN_CONF
 *  - BMA400_FIFO_CONF
 * @note Before calling this API, fill in the value of 
 *   the required configurations in the conf structure.
 * 
 * @param[in] conf
 *  Structure instance of the configuration structure.
 * 
 * @param[in] n_sett
 *  Number of settings to be set.
 * 
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_set_device_conf(const bma400_device_conf_t *conf, uint8_t n_sett)
{
  sl_status_t ret;
  uint16_t idx;
  uint8_t data_array[3] = { 0 };

  /* Proceed if null check is not fine */
  if (conf == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  /* Read the interrupt pin mapping configurations */
  ret = bma400_block_read(BMA400_REG_INT1_MAP, 3, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  for (idx = 0; (idx < n_sett) && (ret == SL_STATUS_OK); idx++) {
    switch (conf[idx].type) {
      case BMA400_AUTOWAKEUP_TIMEOUT:
        ret = set_autowakeup_timeout(&conf[idx].param.auto_wakeup);
        break;

      case BMA400_AUTOWAKEUP_INT:
        ret = set_autowakeup_interrupt(&conf[idx].param.wakeup);
        if (ret != SL_STATUS_OK) {
          return ret;
        }
        /* Interrupt pin mapping */
        map_int_pin(data_array,
                    BMA400_WAKEUP_INT_MAP,
                    conf[idx].param.wakeup.int_chan);
        break;
      case BMA400_AUTO_LOW_POWER:
        ret = set_auto_low_power(&conf[idx].param.auto_lp);
        break;

      case BMA400_INT_PIN_CONF:
        ret = set_int_pin_conf(conf[idx].param.int_conf);
        break;

      case BMA400_INT_OVERRUN_CONF:
        /* Interrupt pin mapping */
        map_int_pin(data_array,
                    BMA400_INT_OVERRUN_MAP,
                    conf[idx].param.overrun_int.int_chan);
        break;

      case BMA400_FIFO_CONF:
        ret = set_fifo_conf(&conf[idx].param.fifo_conf);
        if (ret != SL_STATUS_OK) {
          return ret;
        }
        /* Interrupt pin mapping */
        map_int_pin(data_array,
                    BMA400_FIFO_WM_INT_MAP,
                    conf[idx].param.fifo_conf.fifo_wm_channel);
        map_int_pin(data_array,
                    BMA400_FIFO_FULL_INT_MAP,
                    conf[idx].param.fifo_conf.fifo_full_channel);
        break;
      default:
        ret = SL_STATUS_INVALID_CONFIGURATION;
    }
  }
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Set the interrupt pin mapping configurations */
  ret = bma400_block_write(BMA400_REG_INT1_MAP, 3, data_array);

  return ret;
}

/**************************************************************************//**
 * @brief Enable the various interrupts.
 * the hardware interrupt pin of the sensor.
 * @note Multiple interrupt can be enabled/disabled by
 * @code
 *    struct interrupt_enable int_select[2];
 *
 *    int_select[0].int_sel = BMA400_FIFO_FULL_INT_EN;
 *    int_select[0].conf = BMA400_ENABLE;
 *
 *    int_select[1].int_sel = BMA400_FIFO_WM_INT_EN;
 *    int_select[1].conf = BMA400_ENABLE;
 *
 *    rslt = bma400_enable_interrupt(&int_select, 2, dev);
 * @endcode
 *
 * @param[in] int_select
 *  Structure to enable specific interrupts.
 * @param[in] n_sett
 *  Number of interrupt settings enabled / disabled.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_enable_interrupt(const bma400_int_enable_t *int_select,
                                    uint8_t n_sett)
{
  sl_status_t ret;
  uint8_t conf, idx = 0;
  uint8_t reg_data[2];

  /* Proceed if null check is not fine */
  if (int_select == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  ret = bma400_block_read(BMA400_REG_INT_CONFIG_0, 2, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  ret = SL_STATUS_OK;
  for (idx = 0; idx < n_sett; idx++) {
    conf = int_select[idx].conf;

    /* Enable the interrupt based on user selection */
    switch (int_select[idx].type) {
      case BMA400_DRDY_INT_EN:
        reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_DRDY, conf);
        break;

      case BMA400_FIFO_WM_INT_EN:
        reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_FIFO_WM, conf);
        break;

      case BMA400_FIFO_FULL_INT_EN:
        reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_FIFO_FULL, conf);
        break;

      case BMA400_GEN2_INT_EN:
        reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_GEN2, conf);
        break;

      case BMA400_GEN1_INT_EN:
        reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_GEN1, conf);
        break;

      case BMA400_ORIENT_CHANGE_INT_EN:
        reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_ORIENT_CH, conf);
        break;

      case BMA400_LATCH_INT_EN:
        reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_LATCH, conf);
        break;

      case BMA400_ACTIVITY_CHANGE_INT_EN:
        reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_ACTCH, conf);
        break;

      case BMA400_DOUBLE_TAP_INT_EN:
        reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_D_TAP, conf);
        break;

      case BMA400_SINGLE_TAP_INT_EN:
        reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_S_TAP, conf);
        break;

      case BMA400_STEP_COUNTER_INT_EN:
        reg_data[1] = BMA400_SET_BITS_POS_0(reg_data[1], BMA400_EN_STEP_INT, conf);
        break;

      case BMA400_AUTO_WAKEUP_EN:
        ret = set_auto_wakeup(conf);
        break;

      default:
        ret = SL_STATUS_INVALID_PARAMETER;
        break;
    }
  }
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Set the configurations in the sensor */
  ret = bma400_block_write(BMA400_REG_INT_CONFIG_0, 2, reg_data);

  return ret;
}

/**************************************************************************//**
 * @brief Check if the interrupts are asserted and return the status.
 *
 * @note Interrupt status of the sensor determines which all interrupts
 * are asserted at any instant of time.
 * @code
 *  BMA400_WAKEUP_INT_ASSERTED
 *  BMA400_ORIENT_CH_INT_ASSERTED
 *  BMA400_GEN1_INT_ASSERTED
 *  BMA400_GEN2_INT_ASSERTED
 *  BMA400_FIFO_FULL_INT_ASSERTED
 *  BMA400_FIFO_WM_INT_ASSERTED
 *  BMA400_DRDY_INT_ASSERTED
 *  BMA400_INT_OVERRUN_ASSERTED
 *  BMA400_STEP_INT_ASSERTED
 *  BMA400_S_TAP_INT_ASSERTED
 *  BMA400_D_TAP_INT_ASSERTED
 *  BMA400_ACT_CH_X_ASSERTED
 *  BMA400_ACT_CH_Y_ASSERTED
 *  BMA400_ACT_CH_Z_ASSERTED
 * @endcode
 * @code
 * if (int_status & BMA400_FIFO_FULL_INT_ASSERTED) {
 *     printf("\n FIFO FULL INT ASSERTED");
 * }
 * @endcode
 *
 * @param[in] int_status
 *  Interrupt status of sensor.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_interrupt_status(uint16_t *int_status)
{
  sl_status_t ret;
  uint8_t reg_data[3];

  /* Proceed if null check is not fine */
  if (int_status == NULL) {
    return SL_STATUS_NULL_POINTER;
  }
  /* Read the interrupt status registers */
  ret = bma400_block_read(BMA400_REG_INT_STATUS_0, 3, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_INT_STATUS, reg_data[2]);

  /* Concatenate the interrupt status to the output */
  *int_status = ((uint16_t)reg_data[1] << 8) | reg_data[0];

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the enable/disable status of the various interrupts.
 *
 * @param[out] int_select
 *  Structure to select specific interrupts.
 *
 * @param[out] n_sett
 *  Number of settings to be obtained.
 *
 * @note Select the needed interrupt type for which the status of it whether
 * it is enabled/disabled is to be known in the int_select->int_sel, and the
 * output is stored in int_select->conf either as BMA400_ENABLE/BMA400_DISABLE
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_interrupts_enabled(bma400_int_enable_t *int_select, uint8_t n_sett)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t idx = 0;
  uint8_t reg_data[2];
  uint8_t wkup_int;

  /* Proceed if null check is not fine */
  if (int_select == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  ret = bma400_block_read(BMA400_REG_INT_CONFIG_0, 2, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  for (idx = 0; (idx < n_sett) && (ret == SL_STATUS_OK); idx++) {
    /* Read the enable/disable of interrupts
     * based on user selection
     */
    switch (int_select[idx].type) {
      case BMA400_DRDY_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_DRDY);
        break;

      case BMA400_FIFO_WM_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_FIFO_WM);
        break;

      case BMA400_FIFO_FULL_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_FIFO_FULL);
        break;

      case BMA400_GEN2_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_GEN2);
        break;

      case BMA400_GEN1_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_GEN1);
        break;

      case BMA400_ORIENT_CHANGE_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_ORIENT_CH);
        break;

      case BMA400_LATCH_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_LATCH);
        break;

      case BMA400_ACTIVITY_CHANGE_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_ACTCH);
        break;

      case BMA400_DOUBLE_TAP_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_D_TAP);
        break;

      case BMA400_SINGLE_TAP_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_S_TAP);
        break;

      case BMA400_STEP_COUNTER_INT_EN:
        int_select[idx].conf = BMA400_GET_BITS_POS_0(reg_data[1], BMA400_EN_STEP_INT);
        break;

      case BMA400_AUTO_WAKEUP_EN:
        ret = bma400_read_from_register(BMA400_REG_AUTO_WAKEUP_1, &wkup_int);
        if (ret != SL_STATUS_OK) {
          return ret;
        }
        /* Auto-Wakeup int status */
        int_select[idx].conf = BMA400_GET_BITS(wkup_int, BMA400_WAKEUP_INTERRUPT);
        break;
      default:
        ret = SL_STATUS_INVALID_CONFIGURATION;
        break;
    }
  }

  return ret;
}

/**************************************************************************//**
 * @brief Set the step counter's configuration parameters from
 * the registers 0x59 to 0x70.
 *
 * @verbatim
 *----------------------------------------------------------------------
 * Register name              Address    wrist(default)   non-wrist
 *----------------------------------------------------------------------
 * STEP_COUNTER_CONFIG0        0x59             1             1
 * STEP_COUNTER_CONFIG1        0x5A            45            50
 * STEP_COUNTER_CONFIG2        0x5B           123           120
 * STEP_COUNTER_CONFIG3        0x5C           212           230
 * STEP_COUNTER_CONFIG4        0x5D            68           135
 * STEP_COUNTER_CONFIG5        0x5E             1             0
 * STEP_COUNTER_CONFIG6        0x5F            59           132
 * STEP_COUNTER_CONFIG7        0x60           122           108
 * STEP_COUNTER_CONFIG8        0x61           219           156
 * STEP_COUNTER_CONFIG9        0x62           123           117
 * STEP_COUNTER_CONFIG10       0x63            63           100
 * STEP_COUNTER_CONFIG11       0x64           108           126
 * STEP_COUNTER_CONFIG12       0x65           205           170
 * STEP_COUNTER_CONFIG13       0x66            39            12
 * STEP_COUNTER_CONFIG14       0x67            25            12
 * STEP_COUNTER_CONFIG15       0x68           150            74
 * STEP_COUNTER_CONFIG16       0x69           160           160
 * STEP_COUNTER_CONFIG17       0x6A           195             0
 * STEP_COUNTER_CONFIG18       0x6B            14             0
 * STEP_COUNTER_CONFIG19       0x6C            12            12
 * STEP_COUNTER_CONFIG20       0x6D            60            60
 * STEP_COUNTER_CONFIG21       0x6E           240           240
 * STEP_COUNTER_CONFIG22       0x6F             0             1
 * STEP_COUNTER_CONFIG23       0x70           247             0
 *------------------------------------------------------------------------
 *@endverbatim
 *
 * @param[out] sccr_conf
 *  sc config parameter.
 *
 * @param[out] n_sett
 *  Number of settings to be obtained.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_set_step_counter_param(const uint8_t *sccr_conf)
{
  sl_status_t ret;

  /* Proceed if null check is not fine */
  if (sccr_conf == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  /* Set the step counter parameters in the sensor */
  ret = bma400_block_write(BMA400_REG_STEP_CNT_CONFIG_0, 24, sccr_conf);

  return ret;
}

/**************************************************************************//**
 * @brief Get the raw temperature data output.
 *
 * @note Temperature data output must be divided by a factor of 10.
 *  Consider temperature_data = 195,
 *  Then the actual temperature is 19.5 degree Celsius.
 *
 * @param[out] temperature_data
 *  Temperature data.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_temperature_data(int16_t *temperature_data)
{
  sl_status_t ret;
  uint8_t reg_data;

  /* Proceed if null check is not fine */
  if (temperature_data == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  ret = bma400_read_from_register(BMA400_REG_TEMPERATURE, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Temperature data calculations */
  *temperature_data = (int16_t)(((int8_t)reg_data) * 5) + 230;

  return ret;
}

/**************************************************************************//**
 * @brief Get the step counter output in form of number of steps in
 * the step_count value.
 *
 * @param[out] step_count
 *  Number of step counts.
 * @param[out] activity_data
 *  Activity data WALK/STILL/RUN.
 *  activity_data   |  Status
 * -----------------|------------------
 *  0x00            | BMA400_STILL_ACT
 *  0x01            | BMA400_WALK_ACT
 *  0x02            | BMA400_RUN_ACT
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_get_steps_counted(uint32_t *step_count, uint8_t *activity_data)
{
  sl_status_t ret;
  uint8_t data_array[4];
  uint32_t step_count_0 = 0;
  uint32_t step_count_1 = 0;
  uint32_t step_count_2 = 0;

  /* Proceed if null check is not fine */
  if ((step_count == NULL) || (activity_data == NULL)) {
    return SL_STATUS_NULL_POINTER;
  }

  ret = bma400_block_read(BMA400_REG_STEP_CNT_0, 4, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  step_count_0 = (uint32_t)data_array[0];
  step_count_1 = (uint32_t)data_array[1] << 8;
  step_count_2 = (uint32_t)data_array[2] << 16;
  *step_count = step_count_0 | step_count_1 | step_count_2;

  *activity_data = data_array[3];

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Performs a self test of the accelerometer in BMA400.
 *
 * @note The return value of this API is the result of self test.
 * A self test does not soft reset of the sensor. Hence, the user can
 * define the required settings after performing the self test.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_perform_self_test(void)
{
  sl_status_t ret;
  bma400_sensor_data_t accel_pos, accel_neg;

  /* pre-requisites for self test*/
  ret = enable_self_test();
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  ret = positive_excited_accel(&accel_pos);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  ret = negative_excited_accel(&accel_neg);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Validate the self test result */
  ret = validate_accel_self_test(&accel_pos, &accel_neg);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Perform soft reset */
  ret = bma400_soft_reset();

  return ret;
}

/**************************************************************************//**
 * @brief Delay function.
 *
 * @param[in] time_ms
 *  Time delay in ms.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
void bma400_delay_ms(uint16_t time_ms)
{
  sl_sleeptimer_delay_millisecond(time_ms);
}

/**************************************************************************//**
 *                              PRIVATE FUNCTIONS
 *****************************************************************************/

/**************************************************************************//**
 * @brief Set sensor configurations.
 *
 * @param[in] data
 *  Data to be mapped with interrupt.
 *
 * @param[in] conf
 *  Sensor configurations to be set.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_sensor_conf(uint8_t *data,
                                   const bma400_sensor_conf_t *conf)
{
  sl_status_t ret ;
  uint8_t int_enable = 0;
  bma400_int_chan_t int_map = BMA400_UNMAP_INT_PIN;

  switch (conf->type) {
    case BMA400_ACCEL:
      /* Setting Accel configurations */
      ret = set_accel_conf(&conf->param.accel);
      int_enable = BMA400_DATA_READY_INT_MAP;
      int_map = conf->param.accel.int_chan;
      break;

    case BMA400_TAP_INT:
      /* Setting tap configurations */
      ret = set_tap_conf(&conf->param.tap);
      int_enable = BMA400_TAP_INT_MAP;
      int_map = conf->param.tap.int_chan;
      break;

    case BMA400_ACTIVITY_CHANGE_INT:
      /* Setting activity change configurations */
      ret = set_activity_change_conf(&conf->param.act_ch);
      int_enable = BMA400_ACT_CH_INT_MAP;
      int_map = conf->param.act_ch.int_chan;
      break;

    case BMA400_GEN1_INT:
      /* Setting generic int 1 configurations */
      ret = set_gen1_int(&conf->param.gen_int);
      int_enable = BMA400_GEN1_INT_MAP;
      int_map = conf->param.gen_int.int_chan;
      break;

    case BMA400_GEN2_INT:
      /* Setting generic int 2 configurations */
      ret = set_gen2_int(&conf->param.gen_int);
      int_enable = BMA400_GEN2_INT_MAP;
      int_map = conf->param.gen_int.int_chan;
      break;

    case BMA400_ORIENT_CHANGE_INT:
      /* Setting orient int configurations */
      ret = set_orient_int(&conf->param.orient);
      int_enable = BMA400_ORIENT_CH_INT_MAP;
      int_map = conf->param.orient.int_chan;
      break;

    case BMA400_STEP_COUNTER_INT:
      ret = SL_STATUS_OK;
      int_enable = BMA400_STEP_INT_MAP;
      int_map = conf->param.step_cnt.int_chan;
      break;
  }

  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Int pin mapping settings */
  map_int_pin(data, int_enable, int_map);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Set the accel configurations in sensor.
 *
 * @param[in] accel_conf
 *  Structure instance with accel configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_accel_conf(const bma400_acc_conf_t *accel_conf)
{
  sl_status_t ret;
  uint8_t data_array[3] = { 0, 0, 0xE0 };

  /* Update the accel configurations from the user structure
   * accel_conf
   */
  ret = bma400_block_read(BMA400_REG_ACC_CONFIG_0, 3, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_FILT_1_BW, accel_conf->filt1_bw);
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_OSR_LP, accel_conf->osr_lp);
  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_ACCEL_RANGE, accel_conf->range);
  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_OSR, accel_conf->osr);
  data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_ACCEL_ODR, accel_conf->odr);
  data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_DATA_FILTER, accel_conf->data_src);

  /* Set the accel configurations in the sensor */
  ret = bma400_block_write(BMA400_REG_ACC_CONFIG_0, 3, data_array);

  return ret;
}

/**************************************************************************//**
 * @brief Set the tap setting parameters.
 *
 * @param[in] tap_set
 *  Structure instance of tap configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_tap_conf(const bma400_tap_conf_t *tap_set)
{
  sl_status_t ret;
  uint8_t reg_data[2] = { 0, 0 };

  ret = bma400_block_read(BMA400_REG_TAP_CONFIG_0, 2, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Set the axis to sense for tap */
  reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_TAP_AXES_EN, tap_set->axes_sel);
  /* Set the threshold for tap sensing */
  reg_data[0] = BMA400_SET_BITS_POS_0(reg_data[0], BMA400_TAP_SENSITIVITY, tap_set->sensitivity);
  /* Set the Quiet_dt setting */
  reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_TAP_QUIET_DT, tap_set->quiet_dt);
  /* Set the Quiet setting */
  reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_TAP_QUIET, tap_set->quiet);
  /* Set the tics_th setting */
  reg_data[1] = BMA400_SET_BITS_POS_0(reg_data[1], BMA400_TAP_TICS_TH, tap_set->tics_th);

  /* Set the TAP configuration in the sensor*/
  ret = bma400_block_write(BMA400_REG_TAP_CONFIG_0, 2, reg_data);

  return ret;
}

/**************************************************************************//**
 * @brief Set the parameters for activity change detection.
 *
 * @param[in] act_ch_set
 *  Structure instance of activity change configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_activity_change_conf(const bma400_act_ch_conf_t *act_ch_set)
{
  sl_status_t ret;
  uint8_t data_array[2] = { 0 };

  /* Set the activity change threshold */
  data_array[0] = act_ch_set->act_ch_thres;

  /* Set the axis to sense for activity change */
  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_ACT_CH_AXES_EN, act_ch_set->axes_sel);

  /* Set the data source for activity change */
  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_ACT_CH_DATA_SRC, act_ch_set->data_source);

  /* Set the Number of sample points(NPTS)
   * for sensing activity change
   */
  data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_ACT_CH_NPTS, act_ch_set->act_ch_ntps);

  /* Set the Activity change configuration in the sensor*/
  ret = bma400_block_write(BMA400_REG_ACTCH_CONFIG_0, 2, data_array);

  return ret;
}

/**************************************************************************//**
 * @brief Set the parameters for generic interrupt1 configuration.
 *
 * @param[in] gen_int_set
 *  Structure instance of generic interrupt configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_gen1_int(const bma400_gen_int_conf_t *gen_int_set)
{
  sl_status_t ret;
  uint8_t data_array[11] = { 0 };

  /* Set the axes to sense for interrupt */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_AXES_EN, gen_int_set->axes_sel);
  /* Set the data source for interrupt */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_DATA_SRC, gen_int_set->data_src);
  /* Set the reference update mode */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_REFU, gen_int_set->ref_update);
  /* Set the hysteresis for interrupt calculation */
  data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_INT_HYST, gen_int_set->hysteresis);
  /* Set the criterion to generate interrupt on either ACTIVITY OR INACTIVITY */
  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_GEN_INT_CRITERION, gen_int_set->criterion_sel);
  /* Set the interrupt axes logic (AND/OR) for the
   * enabled axes to generate interrupt
   */
  data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB, gen_int_set->evaluate_axes);
  /* Set the interrupt threshold */
  data_array[2] = gen_int_set->gen_int_thres;
  /* Set the MSB of gen int dur */
  data_array[3] = BMA400_GET_MSB(gen_int_set->gen_int_dur);
  /* Set the LSB of gen int dur */
  data_array[4] = BMA400_GET_LSB(gen_int_set->gen_int_dur);
  /* Handling case of manual reference update */
  if (gen_int_set->ref_update == BMA400_UPDATE_MANUAL) {
    /* Set the LSB of reference x threshold */
    data_array[5] = BMA400_GET_LSB(gen_int_set->int_thres_ref_x);
    /* Set the MSB of reference x threshold */
    data_array[6] = BMA400_GET_MSB(gen_int_set->int_thres_ref_x);
    /* Set the LSB of reference y threshold */
    data_array[7] = BMA400_GET_LSB(gen_int_set->int_thres_ref_y);
    /* Set the MSB of reference y threshold */
    data_array[8] = BMA400_GET_MSB(gen_int_set->int_thres_ref_y);
    /* Set the LSB of reference z threshold */
    data_array[9] = BMA400_GET_LSB(gen_int_set->int_thres_ref_z);
    /* Set the MSB of reference z threshold */
    data_array[10] = BMA400_GET_MSB(gen_int_set->int_thres_ref_z);
    /* Set the GEN1 INT configuration in the sensor */
    ret = bma400_block_write(BMA400_REG_GEN1_INT_CONFIG_0, 11, data_array);
  }
  else {
    /* Set the GEN1 INT configuration in the sensor */
    ret = bma400_block_write(BMA400_REG_GEN1_INT_CONFIG_0, 5, data_array);
  }

  return ret;
}

/**************************************************************************//**
 * @brief Set the parameters for generic interrupt2 configuration.
 *
 * @param[in] gen_int_set
 *  Structure instance of generic interrupt configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_gen2_int(const bma400_gen_int_conf_t *gen_int_set)
{
  sl_status_t ret;
  uint8_t data_array[11] = { 0 };

  /* Set the axes to sense for interrupt */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_AXES_EN, gen_int_set->axes_sel);
  /* Set the data source for interrupt */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_DATA_SRC, gen_int_set->data_src);
  /* Set the reference update mode */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_REFU, gen_int_set->ref_update);
  /* Set the hysteresis for interrupt calculation */
  data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_INT_HYST, gen_int_set->hysteresis);
  /* Set the criterion to generate interrupt on either ACTIVITY OR INACTIVITY */
  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_GEN_INT_CRITERION, gen_int_set->criterion_sel);
  /* Set the interrupt axes logic (AND/OR) for the
   * enabled axes to generate interrupt
   */
  data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB, gen_int_set->evaluate_axes);
  /* Set the interrupt threshold */
  data_array[2] = gen_int_set->gen_int_thres;
  /* Set the MSB of gen int dur */
  data_array[3] = BMA400_GET_MSB(gen_int_set->gen_int_dur);
  /* Set the LSB of gen int dur */
  data_array[4] = BMA400_GET_LSB(gen_int_set->gen_int_dur);
  /* Handling case of manual reference update */
  if (gen_int_set->ref_update == BMA400_UPDATE_MANUAL) {
    /* Set the LSB of reference x threshold */
    data_array[5] = BMA400_GET_LSB(gen_int_set->int_thres_ref_x);
    /* Set the MSB of reference x threshold */
    data_array[6] = BMA400_GET_MSB(gen_int_set->int_thres_ref_x);
    /* Set the LSB of reference y threshold */
    data_array[7] = BMA400_GET_LSB(gen_int_set->int_thres_ref_y);
    /* Set the MSB of reference y threshold */
    data_array[8] = BMA400_GET_MSB(gen_int_set->int_thres_ref_y);
    /* Set the LSB of reference z threshold */
    data_array[9] = BMA400_GET_LSB(gen_int_set->int_thres_ref_z);
    /* Set the MSB of reference z threshold */
    data_array[10] = BMA400_GET_MSB(gen_int_set->int_thres_ref_z);
    /* Set the GEN2 INT configuration in the sensor */
    ret = bma400_block_write(BMA400_REG_GEN2_INT_CONFIG_0, 11, data_array);
  }
  else {
    /* Set the GEN2 INT configuration in the sensor */
    ret = bma400_block_write(BMA400_REG_GEN2_INT_CONFIG_0, 5, data_array);
  }

  return ret;
}

/**************************************************************************//**
 * @brief Set the parameters for orientation interrupt.
 *
 * @param[in] orient_conf
 *  Structure instance of orient interrupt configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_orient_int(const bma400_orient_int_conf_t *orient_conf)
{
  sl_status_t ret;
  uint8_t data_array[10] = { 0 };

  /* Set the axes to sense for interrupt */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_AXES_EN, orient_conf->axes_sel);
  /* Set the data source for interrupt */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_DATA_SRC, orient_conf->data_src);
  /* Set the reference update mode */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_REFU, orient_conf->ref_update);
  /* Set the threshold for interrupt calculation */
  data_array[1] = orient_conf->orient_thres;
  /* Set the stability threshold */
  data_array[2] = orient_conf->stability_thres;
  /* Set the interrupt duration */
  data_array[3] = orient_conf->orient_int_dur;
  /* Handling case of manual reference update */
  if (orient_conf->ref_update == BMA400_UPDATE_MANUAL) {
    /* Set the LSB of reference x threshold */
    data_array[4] = BMA400_GET_LSB(orient_conf->orient_ref_x);
    /* Set the MSB of reference x threshold */
    data_array[5] = BMA400_GET_MSB(orient_conf->orient_ref_x);
    /* Set the MSB of reference x threshold */
    data_array[6] = BMA400_GET_LSB(orient_conf->orient_ref_y);
    /* Set the LSB of reference y threshold */
    data_array[7] = BMA400_GET_MSB(orient_conf->orient_ref_y);
    /* Set the MSB of reference y threshold */
    data_array[8] = BMA400_GET_LSB(orient_conf->orient_ref_z);
    /* Set the LSB of reference z threshold */
    data_array[9] = BMA400_GET_MSB(orient_conf->orient_ref_z);
    /* Set the orient configurations in the sensor */
    ret = bma400_block_write(BMA400_REG_ORIENTCH_CONFIG_0, 10, data_array);
  }
  else {
    /* Set the orient configurations in the sensor excluding
     * reference values of x,y,z
     */
    ret = bma400_block_write(BMA400_REG_ORIENTCH_CONFIG_0, 4, data_array);
  }

  return ret;
}

/**************************************************************************//**
 * @brief Set the auto-wakeup feature of the sensor using a timeout value.
 *
 * @param[in] wakeup_conf
 *  Structure instance of wakeup configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_autowakeup_timeout(const bma400_auto_wakeup_conf_t *wakeup_conf)
{
  sl_status_t ret;
  uint8_t data_array[2];
  uint8_t lsb;
  uint8_t msb;

  ret = bma400_read_from_register(BMA400_REG_AUTO_WAKEUP_1, &data_array[1]);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT, wakeup_conf->wakeup_timeout);
  /* LSB of timeout threshold */
  lsb = BMA400_GET_BITS_POS_0(wakeup_conf->timeout_thres, BMA400_WAKEUP_THRES_LSB);
  /* MSB of timeout threshold */
  msb = BMA400_GET_BITS(wakeup_conf->timeout_thres, BMA400_WAKEUP_THRES_MSB);
  /* Set the value in the data_array */
  data_array[0] = msb;
  data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT_THRES, lsb);
  ret = bma400_block_write(BMA400_REG_AUTO_WAKEUP_0, 2, data_array);

  return ret;
}

/**************************************************************************//**
 * @brief Set the auto-wakeup feature of the sensor.
 *
 * @param[in] conf
 *  Structure instance of wake-up configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_auto_wakeup(uint8_t conf)
{
  sl_status_t ret;
  uint8_t reg_data;

  ret = bma400_read_from_register(BMA400_REG_AUTO_WAKEUP_1, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  reg_data = BMA400_SET_BITS(reg_data, BMA400_WAKEUP_INTERRUPT, conf);
  /* Enabling the Auto wakeup interrupt */
  ret = bma400_write_to_register(BMA400_REG_AUTO_WAKEUP_1, reg_data);

  return ret;
}

/**************************************************************************//**
 * @brief Set the parameters for auto-wakeup feature of the sensor.
 *
 * @param[in] wakeup_conf
 *  Structure instance of wakeup configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_autowakeup_interrupt(const bma400_wakeup_conf_t *wakeup_conf)
{
  sl_status_t ret;
  uint8_t data_array[5] = { 0 };

  /* Set the wakeup reference update */
  data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_WKUP_REF_UPDATE, wakeup_conf->wakeup_ref_update);

  /* Set the number of samples for interrupt condition evaluation */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_SAMPLE_COUNT, wakeup_conf->sample_count);

  /* Enable low power wake-up interrupt for X,Y,Z axes*/
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_WAKEUP_EN_AXES, wakeup_conf->wakeup_axes_en);

  /* Set interrupt threshold configuration  */
  data_array[1] = wakeup_conf->int_wkup_threshold;

  /* Set the reference acceleration x-axis for the wake-up interrupt */
  data_array[2] = wakeup_conf->int_wkup_ref_x;

  /* Set the reference acceleration y-axis for the wake-up interrupt */
  data_array[3] = wakeup_conf->int_wkup_ref_y;

  /* Set the reference acceleration z-axis for the wake-up interrupt */
  data_array[4] = wakeup_conf->int_wkup_ref_z;

  /* Set the wakeup interrupt configurations in the sensor */
  ret = bma400_block_write(BMA400_REG_WAKEUP_CONFIG_0, 5, data_array);

  return ret;
}

/**************************************************************************//**
 * @brief Set the sensor to enter low power mode.
 *
 * @param[in] auto_lp_conf
 *  Structure instance of auto-low power settings.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_auto_low_power(const bma400_auto_lp_conf_t *auto_lp_conf)
{
  sl_status_t ret;
  uint8_t reg_data;
  uint8_t timeout_msb;
  uint8_t timeout_lsb;

  ret = bma400_read_from_register(BMA400_REG_AUTO_LOW_POW_1, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  reg_data = BMA400_SET_BITS_POS_0(reg_data,
                                   BMA400_AUTO_LOW_POW,
                                   auto_lp_conf->auto_low_power_trigger);

  /* If auto Low power timeout threshold is enabled */
  if (auto_lp_conf->auto_low_power_trigger & 0x0C) {
    ret = bma400_read_from_register(BMA400_REG_AUTO_LOW_POW_0, &timeout_msb);
    if (ret != SL_STATUS_OK) {
      return ret;
    }
    /* Compute the timeout threshold MSB value */
    timeout_msb = BMA400_GET_BITS(auto_lp_conf->auto_lp_timeout_threshold, BMA400_AUTO_LP_THRES);

    /* Compute the timeout threshold LSB value */
    timeout_lsb = BMA400_GET_BITS_POS_0(auto_lp_conf->auto_lp_timeout_threshold, BMA400_AUTO_LP_THRES_LSB);
    reg_data = BMA400_SET_BITS(reg_data, BMA400_AUTO_LP_TIMEOUT_LSB, timeout_lsb);

    /* Set the timeout threshold MSB value */
    ret = bma400_write_to_register(BMA400_REG_AUTO_LOW_POW_0, timeout_msb);
    if (ret != SL_STATUS_OK) {
      return ret;
    }
  }

  /* Set the Auto low power configurations */
  ret = bma400_write_to_register(BMA400_REG_AUTO_LOW_POW_1, reg_data);

  return ret;
}

/**************************************************************************//**
 * @brief Set the interrupt pin configurations.
 *
 * @param[in] auto_lp_conf
 *   Interrupt pin configuration.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_int_pin_conf(bma400_int_pin_conf_t int_conf)
{
  sl_status_t ret;
  uint8_t reg_data;

  ret = bma400_read_from_register(BMA400_REG_INT12_IO_CTRL, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  if (int_conf.int_chan == BMA400_INT_CHANNEL_1) {
      /* Setting interrupt pin configurations */
      reg_data = BMA400_SET_BITS(reg_data, BMA400_INT_PIN1_CONF, int_conf.pin_conf);
  }
  else if (int_conf.int_chan == BMA400_INT_CHANNEL_2) {
      /* Setting interrupt pin configurations */
      reg_data = BMA400_SET_BITS(reg_data, BMA400_INT_PIN2_CONF, int_conf.pin_conf);
  }

  /* Set the configurations in the sensor */
  ret = bma400_write_to_register(BMA400_REG_INT12_IO_CTRL, reg_data);

  return ret;
}

/**************************************************************************//**
 * @brief Set the FIFO configurations.
 *
 * @param[in] fifo_conf
 *   Structure instance containing the FIFO configuration set in the sensor.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t set_fifo_conf(const bma400_fifo_conf_t *fifo_conf)
{
  sl_status_t ret;
  uint8_t data_array[3];
  uint8_t sens_data[3];

  /* Get the FIFO configurations and water-mark values from the sensor */
  ret = bma400_block_read(BMA400_REG_FIFO_CONFIG_0, 3, sens_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* FIFO configurations */
  data_array[0] = fifo_conf->conf_regs;
  if (fifo_conf->conf_status == BMA400_DISABLE) {
    /* Disable the selected interrupt status */
    data_array[0] = sens_data[0] & (~data_array[0]);
  }

  /* FIFO water-mark values */
  data_array[1] = BMA400_GET_LSB(fifo_conf->fifo_watermark);
  data_array[2] = BMA400_GET_MSB(fifo_conf->fifo_watermark);
  data_array[2] = BMA400_GET_BITS_POS_0(data_array[2], BMA400_FIFO_BYTES_CNT);
  if ((data_array[1] == sens_data[1]) && (data_array[2] == sens_data[2])) {
    /* Set the FIFO configurations in the
     * sensor excluding the watermark value
     */
    ret = bma400_write_to_register(BMA400_REG_FIFO_CONFIG_0, data_array[0]);
  }
  else {
    /* Set the FIFO configurations in the sensor*/
    ret = bma400_block_write(BMA400_REG_FIFO_CONFIG_0, 3, data_array);
  }

    return ret;
}

/**************************************************************************//**
 * @brief Set sensor settings for auto-wakeup timeout feature.
 *
 * @param[in] wakeup_conf
 *  Structure instance of wake-up configurations
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_autowakeup_timeout(bma400_auto_wakeup_conf_t *wakeup_conf)
{
  sl_status_t ret;
  uint8_t data_array[2];
  uint8_t lsb;
  uint8_t msb;

  ret = bma400_block_read(BMA400_REG_AUTO_WAKEUP_0, 2, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  wakeup_conf->wakeup_timeout = BMA400_GET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT);
  msb = data_array[0];
  lsb = BMA400_GET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT_THRES);

  /* Store the timeout value in the wakeup structure */
  wakeup_conf->timeout_thres = msb << 4 | lsb;

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the set sensor settings for auto-wakeup interrupt feature.
 *
 * @param[in] wakeup_conf
 *  Structure instance of wakeup configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_autowakeup_interrupt(bma400_wakeup_conf_t *wakeup_conf)
{
  sl_status_t ret;
  uint8_t data_array[5];

  ret = bma400_block_read(BMA400_REG_WAKEUP_CONFIG_0, 5, data_array);
    if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* get the wakeup reference update */
  wakeup_conf->wakeup_ref_update = BMA400_GET_BITS_POS_0(data_array[0], BMA400_WKUP_REF_UPDATE);

  /* Get the number of samples for interrupt condition evaluation */
  wakeup_conf->sample_count = BMA400_GET_BITS(data_array[0], BMA400_SAMPLE_COUNT);

  /* Get the axes enabled */
  wakeup_conf->wakeup_axes_en = BMA400_GET_BITS(data_array[0], BMA400_WAKEUP_EN_AXES);

  /* Get interrupt threshold configuration  */
  wakeup_conf->int_wkup_threshold = data_array[1];

  /* Get the reference acceleration x-axis for the wake-up interrupt */
  wakeup_conf->int_wkup_ref_x = data_array[2];

  /* Get the reference acceleration y-axis for the wake-up interrupt */
  wakeup_conf->int_wkup_ref_y = data_array[3];

  /* Get the reference acceleration z-axis for the wake-up interrupt */
  wakeup_conf->int_wkup_ref_z = data_array[4];

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the sensor to get the auto-low power mode configuration settings
 *
 * @param[in] auto_lp_conf
 *  Structure instance of auto-low power settings.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_auto_low_power(bma400_auto_lp_conf_t *auto_lp_conf)
{
  sl_status_t ret;
  uint8_t data_array[2];
  uint8_t timeout_msb;
  uint8_t timeout_lsb;

  ret = bma400_block_read(BMA400_REG_AUTO_LOW_POW_0, 2, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Get the auto low power trigger */
  auto_lp_conf->auto_low_power_trigger = BMA400_GET_BITS_POS_0(data_array[1],
                                                               BMA400_AUTO_LOW_POW);
  timeout_msb = data_array[0];
  timeout_lsb = BMA400_GET_BITS(data_array[1], BMA400_AUTO_LP_TIMEOUT_LSB);

  /* Get the auto low power timeout threshold */
  auto_lp_conf->auto_lp_timeout_threshold = timeout_msb << 4 | timeout_lsb;

  return ret;
}

/**************************************************************************//**
 * @brief Get sensor configurations
 *
 * @param[out] data
 *  Data to be mapped with interrupt.
 *
 *  @param[out] conf
 *  Sensor configurations to be set.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_sensor_conf(const uint8_t *data, bma400_sensor_conf_t *conf)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t int_enable = 0;
  bma400_int_chan_t int_map = BMA400_UNMAP_INT_PIN;

  switch (conf->type) {
    case BMA400_ACCEL:
      /* Get Accel configurations */
      ret = get_accel_conf(&conf->param.accel);
      int_enable = BMA400_DATA_READY_INT_MAP;
      int_map = conf->param.accel.int_chan;
      break;

    case BMA400_TAP_INT:
      /* Get tap configurations */
      ret = get_tap_conf(&conf->param.tap);
      int_enable = BMA400_TAP_INT_MAP;
      int_map = conf->param.tap.int_chan;
      break;

    case BMA400_ACTIVITY_CHANGE_INT:
      /* Get activity change configurations */
      ret = get_activity_change_conf(&conf->param.act_ch);
      int_enable = BMA400_ACT_CH_INT_MAP;
      int_map = conf->param.act_ch.int_chan;
      break;

    case BMA400_GEN1_INT:
      /* Get generic int 1 configurations */
      ret = get_gen1_int(&conf->param.gen_int);
      int_enable = BMA400_GEN1_INT_MAP;
      int_map = conf->param.gen_int.int_chan;
      break;

    case BMA400_GEN2_INT:
      /* Get generic int 2 configurations */
      ret = get_gen2_int(&conf->param.gen_int);
      int_enable = BMA400_GEN2_INT_MAP;
      int_map = conf->param.gen_int.int_chan;
      break;

    case BMA400_ORIENT_CHANGE_INT:
      /* Get orient int configurations */
      ret = get_orient_int(&conf->param.orient);
      int_enable = BMA400_ORIENT_CH_INT_MAP;
      int_map = conf->param.orient.int_chan;
      break;

    case BMA400_STEP_COUNTER_INT:
      ret = SL_STATUS_OK;
      int_enable = BMA400_STEP_INT_MAP;
      break;
  }

  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* Int pin mapping settings */
  get_int_pin_map(data, int_enable, &int_map);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the accel configurations in sensor.
 *
 * @param[out] accel_conf
 *  Structure instance of basic accelerometer configuration.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_accel_conf(bma400_acc_conf_t *accel_conf)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t data_array[3];

  ret = bma400_block_read(BMA400_REG_ACC_CONFIG_0, 3, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  accel_conf->filt1_bw = BMA400_GET_BITS(data_array[0], BMA400_FILT_1_BW);
  accel_conf->osr_lp = BMA400_GET_BITS(data_array[0], BMA400_OSR_LP);
  accel_conf->range = BMA400_GET_BITS(data_array[1], BMA400_ACCEL_RANGE);
  accel_conf->osr = BMA400_GET_BITS(data_array[1], BMA400_OSR);
  accel_conf->odr = BMA400_GET_BITS_POS_0(data_array[1], BMA400_ACCEL_ODR);
  accel_conf->data_src = BMA400_GET_BITS(data_array[2], BMA400_DATA_FILTER);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the number of bytes filled in FIFO.
 *
 * @param[out] fifo_byte_cnt
 *  Number of bytes in the FIFO buffer actually filled by the sensor.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_fifo_length(uint16_t *fifo_byte_cnt)
{
  sl_status_t ret;
  uint8_t data_array[2] = { 0 };

  ret = bma400_block_read(BMA400_REG_FIFO_LENGTH_0, 2, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  data_array[1] = BMA400_GET_BITS_POS_0(data_array[1], BMA400_FIFO_BYTES_CNT);
  /* Available data in FIFO is stored in fifo_byte_cnt*/
  *fifo_byte_cnt = ((uint16_t)data_array[1] << 8) | ((uint16_t)data_array[0]);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Read the FIFO of BMA400.
 *
 * @param[out] fifo
 *  Pointer to the fifo structure.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t read_fifo(bma400_fifo_data_t *fifo)
{
  sl_status_t ret;
  uint8_t reg_data;
  uint8_t fifo_addr = BMA400_REG_FIFO_DATA;

  /* Read the FIFO enable bit */
  ret = bma400_read_from_register(BMA400_REG_FIFO_PWR_CONFIG, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  /* FIFO read disable bit */
  if (reg_data == 0) {
    /* Read FIFO Buffer since FIFO read is enabled */
    ret = bma400_block_read(fifo_addr, (uint32_t)fifo->length, fifo->data);
    if (ret != SL_STATUS_OK) {
      return ret;
    }
  }
  else {
    /* Enable FIFO reading */
    reg_data = 0;
    ret = bma400_write_to_register(BMA400_REG_FIFO_PWR_CONFIG, reg_data);
    if (ret != SL_STATUS_OK) {
      return ret;
    }
  }

  /* Delay to enable the FIFO */
  bma400_delay_ms(1);

  /* Read FIFO Buffer since FIFO read is enabled*/
  ret = bma400_block_read(fifo_addr, (uint32_t)fifo->length, fifo->data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Disable FIFO reading */
  reg_data = 1;
  ret = bma400_write_to_register(BMA400_REG_FIFO_PWR_CONFIG, reg_data);

  return ret;
}

/**************************************************************************//**
 * @brief Set the tap setting parameters.
 *
 * @param[in] tap_set
 *  Structure instance of tap configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_tap_conf(bma400_tap_conf_t *tap_set)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t reg_data[2];

  ret = bma400_block_read(BMA400_REG_TAP_CONFIG_0, 2, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Get the axis enabled for tap sensing */
  tap_set->axes_sel = BMA400_GET_BITS(reg_data[0], BMA400_TAP_AXES_EN);

  /* Get the threshold for tap sensing */
  tap_set->sensitivity = BMA400_GET_BITS_POS_0(reg_data[0], BMA400_TAP_SENSITIVITY);

  /* Get the Quiet_dt setting */
  tap_set->quiet_dt = BMA400_GET_BITS(reg_data[1], BMA400_TAP_QUIET_DT);

  /* Get the Quiet setting */
  tap_set->quiet = BMA400_GET_BITS(reg_data[1], BMA400_TAP_QUIET);

  /* Get the tics_th setting */
  tap_set->tics_th = BMA400_GET_BITS_POS_0(reg_data[1], BMA400_TAP_TICS_TH);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the interrupt pin configurations.
 *
 * @param[out] int_conf
 *  Interrupt pin configuration.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_int_pin_conf(bma400_int_pin_conf_t *int_conf)
{
  sl_status_t ret;
  uint8_t reg_data;

  ret = bma400_read_from_register(BMA400_REG_INT12_IO_CTRL, &reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  if (int_conf->int_chan == BMA400_INT_CHANNEL_1) {
    /* reading Interrupt pin configurations */
    int_conf->pin_conf = BMA400_GET_BITS(reg_data, BMA400_INT_PIN1_CONF);
  }
  else if (int_conf->int_chan == BMA400_INT_CHANNEL_2) {
    /* Setting interrupt pin configurations */
    int_conf->pin_conf = BMA400_GET_BITS(reg_data, BMA400_INT_PIN2_CONF);
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the parameters for activity change detection.
 *
 * @param[out] tap_set
 *  Structure instance of activity change configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_activity_change_conf(bma400_act_ch_conf_t *act_ch_set)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t data_array[2];

  ret = bma400_block_read(BMA400_REG_ACTCH_CONFIG_0, 2, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Get the activity change threshold */
  act_ch_set->act_ch_thres = data_array[0];

  /* Get the axis enabled for activity change detection */
  act_ch_set->axes_sel = BMA400_GET_BITS(data_array[1], BMA400_ACT_CH_AXES_EN);

  /* Get the data source for activity change */
  act_ch_set->data_source = BMA400_GET_BITS(data_array[1], BMA400_ACT_CH_DATA_SRC);

  /* Get the Number of sample points(NPTS)
   * for sensing activity change
   */
  act_ch_set->act_ch_ntps = BMA400_GET_BITS_POS_0(data_array[1], BMA400_ACT_CH_NPTS);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the FIFO configurations.
 *
 * @param[out] fifo_conf
 *  Structure instance containing the FIFO configuration set in the sensor.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_fifo_conf(bma400_fifo_conf_t *fifo_conf)
{
  sl_status_t ret;
  uint8_t data_array[3];

  /* Get the FIFO configurations and water-mark values from the sensor */
  ret = bma400_block_read(BMA400_REG_FIFO_CONFIG_0, 3, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Get the data of FIFO_CONFIG0 register */
  fifo_conf->conf_regs = data_array[0];

  /* Get the MSB of FIFO water-mark  */
  data_array[2] = BMA400_GET_BITS_POS_0(data_array[2], BMA400_FIFO_BYTES_CNT);

  /* FIFO water-mark value is stored */
  fifo_conf->fifo_watermark = ((uint16_t)data_array[2] << 8) | ((uint16_t)data_array[1]);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the generic interrupt1 configuration.
 *
 * @param[out] gen_int_set
 *  Structure instance of generic interrupt configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_gen1_int(bma400_gen_int_conf_t *gen_int_set)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t data_array[11];

  ret = bma400_block_read(BMA400_REG_GEN1_INT_CONFIG_0, 11, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Get the axes to sense for interrupt */
  gen_int_set->axes_sel = BMA400_GET_BITS(data_array[0], BMA400_INT_AXES_EN);

  /* Get the data source for interrupt */
  gen_int_set->data_src = BMA400_GET_BITS(data_array[0], BMA400_INT_DATA_SRC);

  /* Get the reference update mode */
  gen_int_set->ref_update = BMA400_GET_BITS(data_array[0], BMA400_INT_REFU);

  /* Get the hysteresis for interrupt calculation */
  gen_int_set->hysteresis = BMA400_GET_BITS_POS_0(data_array[0], BMA400_INT_HYST);

  /* Get the interrupt axes logic (AND/OR) to generate interrupt */
  gen_int_set->evaluate_axes = BMA400_GET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB);

  /* Get the criterion to generate interrupt ACTIVITY/INACTIVITY */
  gen_int_set->criterion_sel = BMA400_GET_BITS(data_array[1], BMA400_GEN_INT_CRITERION);

  /* Get the interrupt threshold */
  gen_int_set->gen_int_thres = data_array[2];

  /* Get the interrupt duration */
  gen_int_set->gen_int_dur = ((uint16_t)data_array[3] << 8) | data_array[4];

  /* Get the interrupt threshold */
  data_array[6] = data_array[6] & 0x0F;
  gen_int_set->int_thres_ref_x = ((uint16_t)data_array[6] << 8) | data_array[5];
  data_array[8] = data_array[8] & 0x0F;
  gen_int_set->int_thres_ref_y = ((uint16_t)data_array[8] << 8) | data_array[7];
  data_array[10] = data_array[10] & 0x0F;
  gen_int_set->int_thres_ref_z = ((uint16_t)data_array[10] << 8) | data_array[9];

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the generic interrupt2 configuration.
 *
 * @param[out] gen_int_set
 *  Structure instance of generic interrupt configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_gen2_int(bma400_gen_int_conf_t *gen_int_set)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t data_array[11];

  ret = bma400_block_read(BMA400_REG_GEN2_INT_CONFIG_0, 11, data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Get the axes to sense for interrupt */
  gen_int_set->axes_sel = BMA400_GET_BITS(data_array[0], BMA400_INT_AXES_EN);

  /* Get the data source for interrupt */
  gen_int_set->data_src = BMA400_GET_BITS(data_array[0], BMA400_INT_DATA_SRC);

  /* Get the reference update mode */
  gen_int_set->ref_update = BMA400_GET_BITS(data_array[0], BMA400_INT_REFU);

  /* Get the hysteresis for interrupt calculation */
  gen_int_set->hysteresis = BMA400_GET_BITS_POS_0(data_array[0], BMA400_INT_HYST);

  /* Get the interrupt axes logic (AND/OR) to generate interrupt */
  gen_int_set->evaluate_axes = BMA400_GET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB);

  /* Get the criterion to generate interrupt ACTIVITY/INACTIVITY */
  gen_int_set->criterion_sel = BMA400_GET_BITS(data_array[1], BMA400_GEN_INT_CRITERION);

  /* Get the interrupt threshold */
  gen_int_set->gen_int_thres = data_array[2];

  /* Get the interrupt duration */
  gen_int_set->gen_int_dur = ((uint16_t)data_array[3] << 8) | data_array[4];

  /* Get the interrupt threshold */
  data_array[6] = data_array[6] & 0x0F;
  gen_int_set->int_thres_ref_x = ((uint16_t)data_array[6] << 8) | data_array[5];
  data_array[8] = data_array[8] & 0x0F;
  gen_int_set->int_thres_ref_y = ((uint16_t)data_array[8] << 8) | data_array[7];
  data_array[10] = data_array[10] & 0x0F;
  gen_int_set->int_thres_ref_z = ((uint16_t)data_array[10] << 8) | data_array[9];

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get the parameters for orientation interrupt.
 *
 * @param[out] orient_conf
 *  Structure instance of orient interrupt configurations.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t get_orient_int(bma400_orient_int_conf_t *orient_conf)
{
  sl_status_t ret = SL_STATUS_OK;
  uint8_t data_array[10];

  ret = bma400_block_read(BMA400_REG_ORIENTCH_CONFIG_0, 11,  data_array);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Get the axes to sense for interrupt */
  orient_conf->axes_sel = BMA400_GET_BITS(data_array[0], BMA400_INT_AXES_EN);

  /* Get the data source for interrupt */
  orient_conf->data_src = BMA400_GET_BITS(data_array[0], BMA400_INT_DATA_SRC);

  /* Get the reference update mode */
  orient_conf->ref_update = BMA400_GET_BITS(data_array[0], BMA400_INT_REFU);

  /* Get the stability_mode for interrupt calculation */
  orient_conf->stability_mode = BMA400_GET_BITS_POS_0(data_array[0], BMA400_STABILITY_MODE);

  /* Get the threshold for interrupt calculation */
  orient_conf->orient_thres = data_array[1];

  /* Get the stability threshold */
  orient_conf->stability_thres = data_array[2];

  /* Get the interrupt duration */
  orient_conf->orient_int_dur = data_array[3];

  /* Get the interrupt reference values */
  data_array[5] = data_array[5] & 0x0F;
  orient_conf->orient_ref_x = ((uint16_t)data_array[5] << 8) | data_array[4];
  data_array[5] = data_array[7] & 0x0F;
  orient_conf->orient_ref_y = ((uint16_t)data_array[7] << 8) | data_array[6];
  data_array[5] = data_array[9] & 0x0F;
  orient_conf->orient_ref_z = ((uint16_t)data_array[9] << 8) | data_array[8];

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief Check for a frame availability in FIFO.
 *
 * @param[out] fifo
 *  Pointer to the fifo structure.
 * @param[out] frame_available
 *  Variable to denote availability of a frame
 * @param[in] accel_width
 *  Variable to denote 12/8 bit accel data
 * @param[in] data_en
 *  Data enabled in FIFO
 * @param[out] data_index
 *  Index of the currently parsed FIFO data
 *
 * @return
 *  Nothing
 *****************************************************************************/
static void check_frame_available(const bma400_fifo_data_t *fifo,
                                  uint8_t *frame_available,
                                  uint8_t accel_width,
                                  uint8_t data_en,
                                  uint16_t *data_index)
{
  switch (data_en) {
    case BMA400_FIFO_XYZ_ENABLE:
      /* Handling case of 12 bit/ 8 bit data available in FIFO */
      if (accel_width == BMA400_12_BIT_FIFO_DATA) {
        if ((*data_index + 6) > fifo->length) {
          /* Partial frame available */
          *data_index = fifo->length;
          *frame_available = BMA400_DISABLE;
        }
      }
      else if ((*data_index + 3) > fifo->length) {
        /* Partial frame available */
        *data_index = fifo->length;
        *frame_available = BMA400_DISABLE;
      }
      break;

    case BMA400_FIFO_X_ENABLE:
    case BMA400_FIFO_Y_ENABLE:
    case BMA400_FIFO_Z_ENABLE:
      /* Handling case of 12 bit/ 8 bit data available in FIFO */
      if (accel_width == BMA400_12_BIT_FIFO_DATA) {
        if ((*data_index + 2) > fifo->length) {
          /* Partial frame available */
          *data_index = fifo->length;
          *frame_available = BMA400_DISABLE;
        }
      }
      else if ((*data_index + 1) > fifo->length) {
        /* Partial frame available */
        *data_index = fifo->length;
        *frame_available = BMA400_DISABLE;
      }
      break;
    case BMA400_FIFO_XY_ENABLE:
    case BMA400_FIFO_YZ_ENABLE:
    case BMA400_FIFO_XZ_ENABLE:
      /* Handling case of 12 bit/ 8 bit data available in FIFO */
      if (accel_width == BMA400_12_BIT_FIFO_DATA) {
        if ((*data_index + 4) > fifo->length) {
          /* Partial frame available */
          *data_index = fifo->length;
          *frame_available = BMA400_DISABLE;
        }
      }
      else if ((*data_index + 2) > fifo->length) {
        /* Partial frame available */
        *data_index = fifo->length;
        *frame_available = BMA400_DISABLE;
      }
      break;

    case BMA400_FIFO_SENSOR_TIME:
      if ((*data_index + 3) > fifo->length) {
        /* Partial frame available */
        *data_index = fifo->length;
        *frame_available = BMA400_DISABLE;
      }
      break;

    case BMA400_FIFO_CONTROL_FRAME:
      if ((*data_index + 1) > fifo->length) {
        /* Partial frame available */
        *data_index = fifo->length;
        *frame_available = BMA400_DISABLE;
      }
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * @brief Unpack the accelerometer xyz data from the FIFO
 * and store it in the user defined buffer.
 *
 * @param[out] fifo
 *  Pointer to the fifo structure.
 * @param[out] accel_data
 *  Structure instance to store the accel data
 * @param[out] data_index
 *  Index of the currently parsed FIFO data
 * @param[in] accel_width
 *  Variable to denote 12/8 bit accel data
 * @param[in] frame_header
 *  Variable to get the data enabled
 *
 * @return
 *  Nothing
 *****************************************************************************/
static void unpack_accel(const bma400_fifo_data_t *fifo,
                         bma400_sensor_data_t *accel_data,
                         uint16_t *data_index,
                         uint8_t accel_width,
                         uint8_t frame_header)
{
  uint8_t data_lsb;
  uint8_t data_msb;

  /* Header information of enabled axes */
  frame_header = frame_header & BMA400_FIFO_DATA_EN_MASK;
  if (accel_width == BMA400_12_BIT_FIFO_DATA) {
    if (frame_header & BMA400_FIFO_X_ENABLE) {
      /* Accel x data */
      data_lsb = fifo->data[(*data_index)++];
      data_msb = fifo->data[(*data_index)++];
      accel_data->x = (int16_t)(((uint16_t)(data_msb << 4)) | data_lsb);
      if (accel_data->x > 2047) {
        /* Computing accel x data negative value */
        accel_data->x = accel_data->x - 4096;
      }
    }
    else {
      /* Accel x not available */
      accel_data->x = 0;
    }

    if (frame_header & BMA400_FIFO_Y_ENABLE) {
      /* Accel y data */
      data_lsb = fifo->data[(*data_index)++];
      data_msb = fifo->data[(*data_index)++];
      accel_data->y = (int16_t)(((uint16_t)(data_msb << 4)) | data_lsb);
      if (accel_data->y > 2047) {
        /* Computing accel y data negative value */
        accel_data->y = accel_data->y - 4096;
      }
    }
    else {
      /* Accel y not available */
      accel_data->y = 0;
    }

    if (frame_header & BMA400_FIFO_Z_ENABLE) {
      /* Accel z data */
      data_lsb = fifo->data[(*data_index)++];
      data_msb = fifo->data[(*data_index)++];
      accel_data->z = (int16_t)(((uint16_t)(data_msb << 4)) | data_lsb);
      if (accel_data->z > 2047) {
        /* Computing accel z data negative value */
        accel_data->z = accel_data->z - 4096;
      }
    }
    else {
      /* Accel z not available */
      accel_data->z = 0;
    }
  }
  else {
    if (frame_header & BMA400_FIFO_X_ENABLE) {
      /* Accel x data */
      data_msb = fifo->data[(*data_index)++];
      accel_data->x = (int16_t)((uint16_t)(data_msb << 4));
      if (accel_data->x > 2047) {
        /* Computing accel x data negative value */
        accel_data->x = accel_data->x - 4096;
      }
    }
    else {
      /* Accel x not available */
      accel_data->x = 0;
    }

    if (frame_header & BMA400_FIFO_Y_ENABLE)
    {
      /* Accel y data */
      data_msb = fifo->data[(*data_index)++];
      accel_data->y = (int16_t)((uint16_t)(data_msb << 4));
      if (accel_data->y > 2047) {
        /* Computing accel y data negative value */
        accel_data->y = accel_data->y - 4096;
      }
    }
    else {
      /* Accel y not available */
      accel_data->y = 0;
    }

    if (frame_header & BMA400_FIFO_Z_ENABLE) {
      /* Accel z data */
      data_msb = fifo->data[(*data_index)++];
      accel_data->z = (int16_t)((uint16_t)(data_msb << 4));
      if (accel_data->z > 2047) {
        /* Computing accel z data negative value */
        accel_data->z = accel_data->z - 4096;
      }
    }
    else {
      /* Accel z not available */
      accel_data->z = 0;
    }
  }
}

/**************************************************************************//**
 * @brief Parse and store the sensor time from the FIFO data in
 * the structure instance dev.
 *
 * @param[out] fifo
 *  Pointer to the fifo structure.
 * @param[out] data_index
 *  Index of the FIFO data which has sensor time
 *
 * @return
 *  Nothing
 *****************************************************************************/
static void unpack_sensortime_frame(bma400_fifo_data_t *fifo, uint16_t *data_index)
{
  uint32_t time_msb;
  uint16_t time_lsb;
  uint8_t time_xlsb;

  time_msb = fifo->data[(*data_index) + 2] << 16;
  time_lsb = fifo->data[(*data_index) + 1] << 8;
  time_xlsb = fifo->data[(*data_index)];

  /* Sensor time */
  fifo->fifo_sensor_time = (uint32_t)(time_msb | time_lsb | time_xlsb);
  *data_index = (*data_index) + 3;
}

/**************************************************************************//**
 * @brief Check whether the interrupt is mapped to the INT pin1
 * or INT pin2 of the sensor.
 *
 * @param[in] int_1_map
 *  Denote whether the interrupt is mapped to INT1 pin or not.
 * @param[in] int_2_map
 *  Denote whether the interrupt is mapped to INT2 pin or not.
 * @param[out] int_map
 *  Interrupt channel which is mapped INT1/INT2/NONE/BOTH.
 *
 * @return
 *  Nothing
 *****************************************************************************/
static void check_mapped_interrupts(uint8_t int_1_map, uint8_t int_2_map,
                                    bma400_int_chan_t *int_map)
{
  if ((int_1_map == BMA400_ENABLE) && (int_2_map == BMA400_DISABLE)) {
    /* INT 1 mapped INT 2 not mapped */
    *int_map = BMA400_INT_CHANNEL_1;
  }

  if ((int_1_map == BMA400_DISABLE) && (int_2_map == BMA400_ENABLE)) {
    /* INT 1 not mapped INT 2 mapped */
    *int_map = BMA400_INT_CHANNEL_2;
  }

  if ((int_1_map == BMA400_ENABLE) && (int_2_map == BMA400_ENABLE)) {
    /* INT 1 ,INT 2 both mapped */
    *int_map = BMA400_MAP_BOTH_INT_PINS;
  }

  if ((int_1_map == BMA400_DISABLE) && (int_2_map == BMA400_DISABLE)) {
    /* INT 1 ,INT 2 not mapped */
    *int_map = BMA400_UNMAP_INT_PIN;
  }
}

/**************************************************************************//**
 * @brief Get the selected interrupt and its mapping to
 * the hardware interrupt pin of the sensor.
 *
 * @param[out] data_array
 *  Data array of interrupt pin configurations.
 * @param[in] int_enable
 *  Interrupt selected for pin mapping.
 * @param[in] int_map
 *  Interrupt channel which is mapped.
 *
 * @return
 *  Nothing
 *****************************************************************************/
static void get_int_pin_map(const uint8_t *data_array,
                            uint8_t int_enable,
                            bma400_int_chan_t *int_map)
{
  uint8_t int_1_map;
  uint8_t int_2_map;

  switch (int_enable) {
    case BMA400_DATA_READY_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_DRDY);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_DRDY);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_FIFO_WM_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_FIFO_WM);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_FIFO_WM);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_FIFO_FULL_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_FIFO_FULL);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_FIFO_FULL);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_GEN2_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_GEN2);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_GEN2);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_GEN1_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_GEN1);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_GEN1);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_ORIENT_CH_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_ORIENT_CH);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_ORIENT_CH);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_WAKEUP_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS_POS_0(data_array[0], BMA400_EN_WAKEUP_INT);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS_POS_0(data_array[1], BMA400_EN_WAKEUP_INT);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_ACT_CH_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[2], BMA400_ACTCH_MAP_INT1);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[2], BMA400_ACTCH_MAP_INT2);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_TAP_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[2], BMA400_TAP_MAP_INT1);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[2], BMA400_TAP_MAP_INT2);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_STEP_INT_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS_POS_0(data_array[2], BMA400_EN_STEP_INT);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[2], BMA400_STEP_MAP_INT2);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    case BMA400_INT_OVERRUN_MAP:
      /* Interrupt 1 pin mapping status */
      int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_INT_OVERRUN);

      /* Interrupt 2 pin mapping status */
      int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_INT_OVERRUN);

      /* Check the mapped interrupt pins */
      check_mapped_interrupts(int_1_map, int_2_map, int_map);
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * @brief Set the selected interrupt and its mapping to
 * the hardware interrupt pin of the sensor.
 *
 * @param[out] data_array
 *  Data array of interrupt pin configurations.
 * @param[in] int_enable
 *  Interrupt selected for pin mapping.
 * @param[in] int_map
 *  Interrupt channel which is mapped.
 *
 * @return
 *  Nothing
 *****************************************************************************/
static void map_int_pin(uint8_t *data_array, uint8_t int_enable,
                        bma400_int_chan_t int_map)
{
  switch (int_enable) {
    case BMA400_DATA_READY_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
        /* Mapping interrupt to INT pin 1*/
        data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_DRDY, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
        /* Mapping interrupt to INT pin 2*/
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_DRDY, BMA400_ENABLE);
      }
      else  if (int_map == BMA400_UNMAP_INT_PIN) {
        data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_DRDY);
        data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_DRDY);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
          data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_DRDY, BMA400_ENABLE);
          data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_DRDY, BMA400_ENABLE);
      }
      break;

    case BMA400_FIFO_WM_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
          /* Mapping interrupt to INT pin 1*/
          data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_WM, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
          /* Mapping interrupt to INT pin 2*/
          data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_WM, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
          data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_FIFO_WM);
          data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_FIFO_WM);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
          data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_WM, BMA400_ENABLE);
          data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_WM, BMA400_ENABLE);
      }
      break;

    case BMA400_FIFO_FULL_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
          /* Mapping interrupt to INT pin 1 */
          data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
          /* Mapping interrupt to INT pin 2 */
          data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
          data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_FIFO_FULL);
          data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_FIFO_FULL);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
          data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
          data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
      }
      break;

    case BMA400_INT_OVERRUN_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
          /* Mapping interrupt to INT pin 1 */
          data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
          /* Mapping interrupt to INT pin 2 */
          data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
          data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_INT_OVERRUN);
          data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_INT_OVERRUN);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
          data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
          data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
      }
      break;

    case BMA400_GEN2_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
        /* Mapping interrupt to INT pin 1 */
        data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN2, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
        /* Mapping interrupt to INT pin 2 */
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN2, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
        data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_GEN2);
        data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_GEN2);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
        data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN2, BMA400_ENABLE);
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN2, BMA400_ENABLE);
      }
      break;

    case BMA400_GEN1_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
        /* Mapping interrupt to INT pin 1 */
        data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN1, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
        /* Mapping interrupt to INT pin 2 */
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN1, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
        data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_GEN1);
        data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_GEN1);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
        data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN1, BMA400_ENABLE);
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN1, BMA400_ENABLE);
      }
      break;

    case BMA400_ORIENT_CH_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
        /* Mapping interrupt to INT pin 1 */
        data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
        /* Mapping interrupt to INT pin 2 */
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
        data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_ORIENT_CH);
        data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_ORIENT_CH);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
        data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
      }
      break;

    case BMA400_WAKEUP_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
        /* Mapping interrupt to INT pin 1 */
        data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
        /* Mapping interrupt to INT pin 2 */
        data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
        data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_WAKEUP_INT);
        data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_WAKEUP_INT);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
        data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
        data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
      }
      break;

    case BMA400_ACT_CH_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
          /* Mapping interrupt to INT pin 1 */
          data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT1, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
          /* Mapping interrupt to INT pin 2 */
          data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT2, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
          data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_ACTCH_MAP_INT1);
          data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_ACTCH_MAP_INT2);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
          data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT1, BMA400_ENABLE);
          data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT2, BMA400_ENABLE);
      }
      break;

    case BMA400_TAP_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
        /* Mapping interrupt to INT pin 1 */
        data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT1, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
        /* Mapping interrupt to INT pin 2 */
        data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT2, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
        data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_TAP_MAP_INT1);
        data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_TAP_MAP_INT2);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
        data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT1, BMA400_ENABLE);
        data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT2, BMA400_ENABLE);
      }
      break;

    case BMA400_STEP_INT_MAP:
      if (int_map == BMA400_INT_CHANNEL_1) {
        /* Mapping interrupt to INT pin 1 */
        data_array[2] = BMA400_SET_BITS_POS_0(data_array[2], BMA400_EN_STEP_INT, BMA400_ENABLE);
      }
      else if (int_map == BMA400_INT_CHANNEL_2) {
        /* Mapping interrupt to INT pin 2 */
        data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_STEP_MAP_INT2, BMA400_ENABLE);
      }
      else if (int_map == BMA400_UNMAP_INT_PIN) {
        data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_EN_STEP_INT);
        data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_STEP_MAP_INT2);
      }
      else if (int_map == BMA400_MAP_BOTH_INT_PINS) {
        data_array[2] = BMA400_SET_BITS_POS_0(data_array[2], BMA400_EN_STEP_INT, BMA400_ENABLE);
        data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_STEP_MAP_INT2, BMA400_ENABLE);
      }
      break;
    default:
      break;
  }
}

/**************************************************************************//**
 * @brief Perform the pre-requisites needed to perform the self test.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t enable_self_test(void)
{
  sl_status_t  ret;

  /* Accelerometer setting structure */
  bma400_sensor_conf_t accel_setting;

  /* Select the type of configuration to be modified */
  accel_setting.type = BMA400_ACCEL;

  /* Get the accel configurations which are set in the sensor */
  ret = bma400_get_sensor_conf(&accel_setting, 1);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Modify to the desired configurations */
  accel_setting.param.accel.odr = BMA400_ODR_100HZ;

  accel_setting.param.accel.range = BMA400_RANGE_4G;
  accel_setting.param.accel.osr = BMA400_ACCEL_OSR_SETTING_3;
  accel_setting.param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;

  /* Set the desired configurations in the sensor */
  ret = bma400_set_sensor_conf(&accel_setting, 1);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* self test enabling delay */
  bma400_delay_ms(BMA400_DELAY_MS_SELF_TEST);

  ret = bma400_set_power_mode(BMA400_MODE_NORMAL);

  return ret;
}

/**************************************************************************//**
 * @brief Perform self test with positive excitation.
 *
 * @param[in] accel_pos
 *  Structure pointer to store accel data for positive excitation.
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t positive_excited_accel(bma400_sensor_data_t *accel_pos)
{
  sl_status_t ret;
  uint8_t reg_data = BMA400_ENABLE_POSITIVE_SELF_TEST;

  /* Enable positive excitation for all 3 axes */
  ret = bma400_write_to_register(BMA400_REG_SELF_TEST, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Read accel data after 50ms delay */
  bma400_delay_ms(BMA400_DELAY_MS_SELF_TEST_DATA_READ);
  ret = bma400_get_accel_data(BMA400_DATA_ONLY, accel_pos);

    return ret;
}

/**************************************************************************//**
 * @brief Perform self test with negative excitation.
 *
 * @param[in] accel_neg
 *  Structure pointer to store accel data for positive excitation.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t negative_excited_accel(bma400_sensor_data_t *accel_neg)
{
  sl_status_t ret;
  uint8_t reg_data = BMA400_ENABLE_NEGATIVE_SELF_TEST;

  /* Enable negative excitation for all 3 axes */
  ret = bma400_write_to_register(BMA400_REG_SELF_TEST, reg_data);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Read accel data after 50ms delay */
  bma400_delay_ms(BMA400_DELAY_MS_SELF_TEST_DATA_READ);
  ret = bma400_get_accel_data(BMA400_DATA_ONLY, accel_neg);
  if (ret != SL_STATUS_OK) {
    return ret;
  }
  /* Disable self test */
  reg_data = BMA400_DISABLE_SELF_TEST;
  ret = bma400_write_to_register(BMA400_REG_SELF_TEST, reg_data);

  return ret;
}

/**************************************************************************//**
 * @brief Validate the self test results.
 *
 * @param[in] accel_pos
 *  Structure pointer to store accel data for positive excitation.
 * @param[in] accel_neg
 *  Structure pointer to store accel data for negative excitation.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
static sl_status_t validate_accel_self_test(const bma400_sensor_data_t *accel_pos,
                                            const bma400_sensor_data_t *accel_neg)
{
  /* Structure for difference of accel values */
  bma400_selftest_delta_limit_t accel_data_diff = { 0, 0, 0 };

  /* accel x difference value */
  accel_data_diff.x = (accel_pos->x - accel_neg->x);

  /* accel y difference value */
  accel_data_diff.y = (accel_pos->y - accel_neg->y);

  /* accel z difference value */
  accel_data_diff.z = (accel_pos->z - accel_neg->z);

  /* Validate the results of self test */
  if (((accel_data_diff.x) > BMA400_ST_ACC_X_AXIS_SIGNAL_DIFF) &&
      ((accel_data_diff.y) > BMA400_ST_ACC_Y_AXIS_SIGNAL_DIFF) &&
      ((accel_data_diff.z) > BMA400_ST_ACC_Z_AXIS_SIGNAL_DIFF)) {
    /* Self test pass condition */
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}
