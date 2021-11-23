/***************************************************************************//**
* @file cap1166.c
* @brief Driver for the cap1166 Capacitive Touch Sensor
*******************************************************************************
* # License
* <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "cap1166.h"

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup cap1166_spi_interface
 * @brief the SPI communication for cap1166 touch sensor
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Reset SPI communication between MCU and cap1166 touch sensor
 *
 * @param[in] spidrv_handle
 *   The SPI peripheral to use.
 * @param[in] cmd
 *   The cap1166_command_t instance to use.
 *
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT SPI transmit failure
 ******************************************************************************/
static sl_status_t cap1166_reset_spi_interface(SPIDRV_Handle_t spidrv_handle,
                                               cap1166_command_t cmd);

/***************************************************************************//**
 * @brief
 *   Sets address pointer for SPI communication between MCU and cap1166
 *   touch sensor.
 *
 * @param[in] spidrv_handle
 *   The SPI peripheral to use.
 * @param[in] reg
 *   The register address to set address pointer for SPI communication between
 *   MCU and cap1166 touch sensor.
 * @param[in] cmd
 *   The cap1166_command_t instance to use.
 *
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT SPI transmit failure
 ******************************************************************************/
static sl_status_t cap1166_set_address_pointer(SPIDRV_Handle_t spidrv_handle,
                                               uint8_t reg,
                                               cap1166_command_t cmd);

/***************************************************************************//**
 * @brief
 *   Read register from the cap1166 sensor.
 *
 * @param[in] spidrv_handle
 *   The SPI peripheral to use.
 * @param[in] reg
 *   The register address to read from in the sensor.
 * @param[out] data
 *   The data read from the sensor
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL;
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT SPI transmit failure
 ******************************************************************************/
static sl_status_t cap1166_read_register(SPIDRV_Handle_t spidrv_handle,
                                         uint8_t reg,
                                         uint8_t *data);

/***************************************************************************//**
 * @brief
 *   Write register in the cap1166 sensor.
 *
 * @param[in] spidrv_handle
 *   The SPI peripheral to use.
 * @param[in] reg
 *   The register address to write to in the sensor
 * @param[in] data
 *   The data to write to the sensor
 *
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT SPI transmit failure
 ******************************************************************************/
static sl_status_t cap1166_write_register(SPIDRV_Handle_t spidrv_handle,
                                          uint8_t reg,
                                          uint8_t data);

/***************************************************************************//**
 * @brief
 *   Write a block of data to the cap1166 sensor.
 *
 * @param[in] spidrv_handle
 *   The SPI peripheral to use.
 * @param[in] reg
 *   The first register to begin writing to
 * @param[in] length
 *   The number of bytes to write to the sensor
 * @param[in] data
 *   The data to write to the sensor
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL;
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT SPI transmit failure
 ******************************************************************************/
static sl_status_t cap1166_write_register_block(SPIDRV_Handle_t spidrv_handle,
                                                uint8_t reg,
                                                uint8_t length,
                                                const uint8_t *data);

/***************************************************************************//**
 * @brief
 *   Set the given bit(s) in a register in the Touch sensor device.
 *
 * @param[in] spidrv_handle
 *   The SPI instance to use.
 * @param[in] addr
 *   The address of the register
 * @param[in] mask
 *   The mask specifies which bits should be set. If a given bit of the mask is
 *   1, that register bit will be set to 1. All the other register bits will be
 *   untouched.
 *
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
static sl_status_t cap1166_set_register_bits(SPIDRV_Handle_t spidrv_handle,
                                             uint8_t addr,
                                             uint8_t mask);

/***************************************************************************//**
 * @brief
 *   Clear the given bit(s) in a register in the Touch sensor device.
 *
 * @param[in] spidrv_handle
 *   The SPI instance to use.
 * @param[in] addr
 *   The address of the register
 * @param[in] mask
 *   The mask specifies which bits should be clear. If a given bit of the mask
 *   is 1 that register bit will be cleared to 0. All the other register bits
 *   will be untouched.
 *
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
static sl_status_t cap1166_clear_register_bits(SPIDRV_Handle_t spidrv_handle,
                                               uint8_t addr,
                                               uint8_t mask);

/***************************************************************************//**
 * @brief
 *   Write the given bit(s) in a register in the Touch sensor device.
 *
 * @param[in] spidrv_handle
 *   The SPI instance to use.
 * @param[in] addr
 *   The address of the register
 * @param[in] mask
 *   The mask specifies which bits should be write. All the other register bits
 *   will be untouched.
 * @param[in] pos
 *   The bit position which will change value in register
 *
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
static sl_status_t cap1166_write_register_bits(SPIDRV_Handle_t spidrv_handle,
                                               uint8_t reg,
                                               uint8_t pos,
                                               uint8_t mask);
/** @} (end addtogroup cap1166_spi_interface) */

/***************************************************************************//**
 * @addtogroup cap1166 Cap1166 - Capacitive touch sensor.
 * @brief  Capacitive touch sensor driver.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 *  Enables sensor inputs for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_sensor_enable(cap1166_handle_t *cap1166_handle,
                                  power_state_t state,
                                  uint8_t *sensor_inputs_en)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t input_enable_mask = 0x00;

  if((cap1166_handle == NULL) || (sensor_inputs_en == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  for(uint8_t index = 0; index < 6; index++){
      if(sensor_inputs_en[index] == CAP1166_SENSOR_INPUT_ENABLE){
          input_enable_mask |= 1 << index;
      }
  }

  if(state == CAP1166_ACTIVE){
      /* sensor inputs enable in active mode */
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_SENS_IN_EN_REG,
                                      input_enable_mask);
  }
  else if(state == CAP1166_STANDBY){
      /* sensor inputs enable in standby mode */
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_STANDBY_CHANN_REG,
                                      input_enable_mask);
  }

  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Enables sensor inputs interrupt for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_sensor_int_enable(cap1166_handle_t *cap1166_handle,
                                      uint8_t *sensor_int_en)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t interrupt_mask = 0x00;

  if((cap1166_handle == NULL) || (sensor_int_en == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  for(uint8_t index = 0; index < 6; index++){
      if(sensor_int_en[index] == CAP1166_SENSOR_INTERUPT_ENABLE){
          interrupt_mask |= 1 << index;
      }
  }

  /* sensor inputs interrupt enable */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_INTERR_EN_REG,
                                  interrupt_mask);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Enables sensor inputs Repeat rate for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_sensor_repeat_rate_enable(cap1166_handle_t *cap1166_handle,
                                              uint8_t *sensor_repeat_rate_en)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t repeat_rate_mask = 0x00;

  if((cap1166_handle == NULL) || (sensor_repeat_rate_en == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  for(uint8_t index = 0; index < 6; index++){
      if(sensor_repeat_rate_en[index] == CAP1166_SENSOR_REPEAT_ENABLE){
          repeat_rate_mask |= 1 << index;
      }
  }

  /* sensor inputs repeat rate enable */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_REPEAT_RATE_EN_REG,
                                  repeat_rate_mask);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures the threshold sensor inputs for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_sensor_threshold_config(cap1166_handle_t *cap1166_handle,
                                            power_state_t state,
                                            uint8_t *sensor_threshold)
{
  sl_status_t retval = SL_STATUS_OK;

  if((cap1166_handle == NULL) || (sensor_threshold == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  if(state == CAP1166_ACTIVE){
      /* sensor inputs threshold in active mode */
      retval = cap1166_write_register_block(cap1166_handle->spidrv_handle,
                                            CAP1166_SENS_IN1_THRESHOLD_REG,
                                            6,
                                            sensor_threshold);
  }
  else if(state == CAP1166_STANDBY){
      /* sensor inputs threshold in standby mode */
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_STANDBY_THRESHOLD_REG,
                                      *sensor_threshold);
  }

  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures the re-calibration sensor inputs for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_sensor_recalib_config(cap1166_handle_t *cap1166_handle,
                                          recalib_cfg_t *recalib_cfg)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t pos_mask = 0x00;
  uint8_t recalib_mask = 0x00;

  if((cap1166_handle == NULL) || (recalib_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  switch(recalib_cfg->recalib_mode){
    case CAP1166_NEG_DELTA_CALIB :
      /* configure negative delta calibration */
      pos_mask = CAP1166_NEG_DELTA_CNT_MASK;
      retval = cap1166_write_register_bits(cap1166_handle->spidrv_handle,
                                           CAP1166_RECALIB_CONFIG_REG,
                                           pos_mask,
                                           recalib_cfg->neg_del_cnt);
      if(retval != ECODE_EMDRV_SPIDRV_OK){
          return SL_STATUS_TRANSMIT;
      }
      break;

    case CAP1166_DELAY_CALIB:
      /* configure negative delay calibration */
      pos_mask = CAP1166_MAX_DUR_RECALIB_EN;
      retval = cap1166_write_register_bits(cap1166_handle->spidrv_handle,
                                           CAP1166_CONFIG_REG,
                                           pos_mask,
                                           recalib_cfg->max_during_en);
      if(retval != ECODE_EMDRV_SPIDRV_OK){
          return SL_STATUS_TRANSMIT;
      }

      pos_mask = CAP1166_MAX_DUR_CALIB_MASK;
      retval = cap1166_write_register_bits(cap1166_handle->spidrv_handle,
                                           CAP1166_SENS_IN_CONFIG_REG,
                                           pos_mask,
                                           recalib_cfg->delay_recalib);
      if(retval != ECODE_EMDRV_SPIDRV_OK){
          return SL_STATUS_TRANSMIT;
      }
      break;

    case CAP1166_MANUAL_CALIB:
      /* configure negative manual calibration */
      for(uint8_t index = 0; index < 6; index++){
          if(recalib_cfg->sensor_recalib[index] ==
              CAP1166_SENSOR_RECALIB_ENABLE){
              recalib_mask |= 1 << index;
          }
      }

      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_CALIB_ACTIVATE_REG,
                                      recalib_mask);
      if(retval != ECODE_EMDRV_SPIDRV_OK){
          return SL_STATUS_TRANSMIT;
      }
      break;

    case CAP1166_AUTO_CALIB:
      /* configure negative auto calibration */
      pos_mask = CAP1166_CAL_CFG_RE_CALIB_MASK;
      retval = cap1166_write_register_bits(cap1166_handle->spidrv_handle,
                                           CAP1166_RECALIB_CONFIG_REG,
                                           pos_mask,
                                           recalib_cfg->rate_recalib);
      if(retval != ECODE_EMDRV_SPIDRV_OK){
          return SL_STATUS_TRANSMIT;
      }
      break;
    default:
      break;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures the noise sensor inputs for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_sensor_noise_config(cap1166_handle_t *cap1166_handle,
                                        sensor_noise_cfg_t *sensor_noise_cfg)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t pos_mask;

  if((cap1166_handle == NULL) || (sensor_noise_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  if(sensor_noise_cfg->digital_noise_threshold ==
      CAP1166_DIG_NOISE_THRESHOLD_EN){
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_SENS_IN_NOISE_THRESHOLD_REG,
                                      sensor_noise_cfg->noise_threshold);
      if(retval != SL_STATUS_OK){
          return retval;
      }
  }

  /*
   * Input configuration register
   *   + The noise threshold is enable
   *   + Analog noise filter is enable
   */
  pos_mask = CAP1166_DIS_DIG_NOISE_MASK | CAP1166_DIS_ANA_NOISE_MASK ;
  retval = cap1166_write_register_bits(
      cap1166_handle->spidrv_handle,
      CAP1166_CONFIG_REG,
      pos_mask,
      sensor_noise_cfg->digital_noise_threshold |
      sensor_noise_cfg->analog_noise_filter);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  /*
   * Input configuration 2 register
   *   + show both RF noise and low frequency EMI noise
   *   + RF noise filter is enabled
   */

  pos_mask = CAP1166_RF_NOISE_FILTER_MASK | CAP1166_SHOW_RF_NOISE_MASK ;
  retval = cap1166_write_register_bits(
      cap1166_handle->spidrv_handle,
      CAP1166_CONFIG2_REG,
      pos_mask,
      sensor_noise_cfg->show_low_frequency_noise |
      sensor_noise_cfg->RF_noise_filter );
  if(retval != SL_STATUS_OK){
      return retval;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures the sensitivity for cap1166 touch sensor
 ******************************************************************************/
sl_status_t  cap1166_sensor_sensitivity_config(
    cap1166_handle_t *cap1166_handle,
    power_state_t state,
    sensitivity_control_cfg_t *sensitivity_control_cfg)
{
  sl_status_t retval = SL_STATUS_OK;

  if((cap1166_handle == NULL) || (sensitivity_control_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  if(state == CAP1166_ACTIVE){
      /* sensor sensitivity in active mode */
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_SENSITIVITY_CON_REG,
                                      sensitivity_control_cfg->sens_active  |
                                      sensitivity_control_cfg->base_shift);
  }
  else if(state == CAP1166_STANDBY){
      /* sensor sensitivity in standby mode */
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_STANDBY_SENSITIVITY_REG,
                                      sensitivity_control_cfg->sens_stby);
  }

  if(retval != ECODE_EMDRV_SPIDRV_OK){
    return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures the sensing cycle for cap1166 touch sensor
 ******************************************************************************/
sl_status_t  cap1166_sensor_sensing_cycle_config(
    cap1166_handle_t *cap1166_handle,
    power_state_t state,
    sensing_cycle_cfg_t *sensing_cycle_cfg)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t pos_mask;

  if((cap1166_handle == NULL) || (sensing_cycle_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  if(state == CAP1166_ACTIVE){
      /* sensor sensing cycle in active mode */
      retval = cap1166_write_register(
          cap1166_handle->spidrv_handle,
          CAP1166_AVRG_AND_SAMPL_CONFIG_REG,
          sensing_cycle_cfg->number_samples_per_cycle |
          sensing_cycle_cfg->sample_time |
          sensing_cycle_cfg->overall_cycle_time);
  }
  else if(state == CAP1166_STANDBY){
      /* sensor sensing cycle in standby mode */
      pos_mask = CAP1166_STBY_AVG_MASK |
          CAP1166_STBY_SAMP_TIM_MASK |
          CAP1166_STBY_CY_TIME_MASK;
      retval = cap1166_write_register_bits(
          cap1166_handle->spidrv_handle,
          CAP1166_STANDBY_CONFIG_REG,
          pos_mask,
          sensing_cycle_cfg->number_samples_per_cycle |
          sensing_cycle_cfg->sample_time |
          sensing_cycle_cfg->overall_cycle_time);
  }

  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures state of  interrupt pin (ALERT# pin) sensor inputs for cap1166
 *  touch sensor
 ******************************************************************************/
sl_status_t cap1166_int_pin_config(cap1166_handle_t *cap1166_handle,
                                   uint8_t state)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t pos_mask = 0x00;

  if(cap1166_handle == NULL){
      return SL_STATUS_NULL_POINTER;
  }

  /* configure INT pin */
  pos_mask = CAP1166_ALT_POL_MASK;
  retval = cap1166_write_register_bits(cap1166_handle->spidrv_handle,
                                       CAP1166_CONFIG2_REG,
                                       pos_mask,
                                       state);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}
/***************************************************************************//**
 *  Configures number of simultaneous touches detection for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_multiple_touch_config(cap1166_handle_t *cap1166_handle,
                                          multi_touch_cfg_t *multi_touch_cfg)
{
  sl_status_t retval = SL_STATUS_OK;

  if(cap1166_handle == NULL || multi_touch_cfg == NULL){
      return SL_STATUS_NULL_POINTER;
  }

  /* configure multiple touch */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_MULTIPLE_TOUCH_CONFIG_REG,
                                  multi_touch_cfg->number_of_touch |
                                  multi_touch_cfg->multi_block_en);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures the press and hold detection for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_press_and_hold_config(
    cap1166_handle_t *cap1166_handle,
    press_and_hold_cfg_t *press_and_hold_cfg,
    uint8_t *sensor_repeat_rate_en)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t pos_mask = 0x00;

  if((cap1166_handle == NULL) ||
      (press_and_hold_cfg == NULL) ||
      (sensor_repeat_rate_en == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  /*
   * INT_REL bit - an interrupt will be trigger when a press is detected
   * and a release do so
   */
  pos_mask = CAP1166_INT_REL_N_MASK;
  retval = cap1166_write_register_bits(
      cap1166_handle->spidrv_handle,
      CAP1166_CONFIG2_REG,
      pos_mask,
      press_and_hold_cfg->release_detection_en);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /*
   * The variable release_check will be used when detect touch.
   * Change it to fit the configuration
   */
  cap1166_handle->release_check = press_and_hold_cfg->release_detection_en;

  /*
   * RPT_RATE bit - Determines the time duration between interrupt assertions
   * when auto repeat is enabled
   */
  pos_mask = CAP1166_RPT_RATE_MASK;
  retval = cap1166_write_register_bits(cap1166_handle->spidrv_handle,
                                       CAP1166_SENS_IN_CONFIG2_REG,
                                       pos_mask,
                                       press_and_hold_cfg->set_repeat_rate);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* active sensor repeat rate */
  retval = cap1166_sensor_repeat_rate_enable(cap1166_handle,
                                             sensor_repeat_rate_en);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /*
   * M_PRESS bit: when repeat rate is enable,
   * MPRESS that determines whether a touch is flagged as a simple “touch” or
   * a “press and hold”
   */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_SENS_IN_CONFIG2_REG,
                                  press_and_hold_cfg->hold_time);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Initializes the cap1166 touch sensor
 ******************************************************************************/
sl_status_t  cap1166_init(cap1166_handle_t *cap1166_handle)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t partId = 0, partRev = 0;

  if(cap1166_handle == NULL){
      return SL_STATUS_NULL_POINTER;
  }

  /* reset sensor */
  retval = cap1166_reset(cap1166_handle);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  /* Dummy read of Chip-ID in SPI mode */
  retval = cap1166_identify(cap1166_handle,
                            &partId,
                            &partRev);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  /* Check for chip id validity */
  if(partId != CAP1166_CHIP_ID){
      return SL_STATUS_NOT_FOUND;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures the cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_config(cap1166_handle_t *cap1166_handle,
                           cap1166_cfg_t *cap1166_cfg)
{
  sl_status_t retval = SL_STATUS_OK;

  if((cap1166_handle == NULL) || (cap1166_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  /* configure the threshold level */
  if(cap1166_cfg->power_state == CAP1166_ACTIVE){
      retval = cap1166_sensor_threshold_config(
          cap1166_handle,
          cap1166_cfg->power_state,
          cap1166_cfg->sensor_inputs.sensor_threshold_active);
  }
  else{
      retval = cap1166_sensor_threshold_config(
          cap1166_handle,
          cap1166_cfg->power_state,
          &cap1166_cfg->sensor_inputs.sensor_threshold_standy);
  }

  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure the Sensitivity */
  retval = cap1166_sensor_sensitivity_config(cap1166_handle,
                                             cap1166_cfg->power_state,
                                             &cap1166_cfg->sensitivity_control);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure the sensing cycle */
  retval = cap1166_sensor_sensing_cycle_config(cap1166_handle,
                                               cap1166_cfg->power_state,
                                               &cap1166_cfg->sensing_cycle);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure for noise detection */
  retval = cap1166_sensor_noise_config(cap1166_handle,
                                       &cap1166_cfg->sensor_noise);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure for re-calibration */
  retval = cap1166_sensor_recalib_config(cap1166_handle,
                                         &cap1166_cfg->recalib);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure multiple touch detection */
  cap1166_multiple_touch_config(cap1166_handle,
                                &cap1166_cfg->multi_touch);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure for press and hold */
  retval = cap1166_press_and_hold_config(
      cap1166_handle,
      &cap1166_cfg->press_and_hold,
      cap1166_cfg->sensor_inputs.sensor_repeat_rate_en);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure for INT pin */
  retval = cap1166_int_pin_config(cap1166_handle,
                                  cap1166_cfg->alert_mode);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* configure for led */
  retval = cap1166_led_config(cap1166_handle,
                              &cap1166_cfg->led_cfg);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* enter in power mode */
  retval = cap1166_set_power_mode(cap1166_handle,
                                  cap1166_cfg->power_state,
                                  cap1166_cfg->analog_gain,
                                  &cap1166_cfg->sensor_inputs);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Enables the proximity detection for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_proximity_detection_enable(cap1166_handle_t *cap1166_handle,
                                               proximity_cfg_t *promixity_cfg)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t pos_mask;

  if((cap1166_handle == NULL) || (promixity_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  /* configure the sensing cycle */
  retval = cap1166_sensor_sensing_cycle_config(
      cap1166_handle,
      CAP1166_STANDBY,
      &promixity_cfg->standby_sensing_cycle);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* enable proximity detection by AVG_SUM bit */
  pos_mask = CAP1166_AVG_SUM_MASK;
  retval = cap1166_write_register_bits(cap1166_handle->spidrv_handle,
                                       CAP1166_STANDBY_CONFIG_REG,
                                       pos_mask,
                                       promixity_cfg->sum_avr_mode);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Enables the touch pattern for cap1166 touch sensor
 ******************************************************************************/
sl_status_t cap1166_touch_pattern_enable(cap1166_handle_t *cap1166_handle,
                                         pattern_cfg_t *pattern_cfg)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t sensor_inputs_mask = 0x00;

  if((cap1166_handle == NULL) || (pattern_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  /*
   * + multiple touch pattern is enabled
   * + Sets MTP threshold
   * + MTP mode which was pattern recognition mode or absolute number mode
   * + sets MTP alert
   */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_MULTIPLE_TOUCH_PATTERN_CONFIG_REG,
                                  CAP1166_MLTP_TOUCH_PATTERN_EN |
                                  pattern_cfg->threshold_percent |
                                  pattern_cfg->MTP_mode |
                                  pattern_cfg->MTP_alert);

  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  for(uint8_t index = 0; index < 6; index++){
      if(pattern_cfg->MTP_sensor_inputs[index] ==
          CAP1166_SENSOR_PATTERN_ENABLE){
          sensor_inputs_mask |= 1 << index;
      }
  }

  /* set sensor inputs which is part of MTP */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_MULTIPLE_TOUCH_PATTERN_REG,
                                  sensor_inputs_mask);

  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Detects touch on sensor inputs to determine which input touch is detected
 *  or released.
 ******************************************************************************/
sl_status_t cap1166_detect_touch(cap1166_handle_t *cap1166_handle,
                                 uint8_t *in_sens)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t int_check = 0x00;
  uint8_t touch_check = 0x00;
  uint8_t noise_check = 0x00;
  uint8_t index;
  uint8_t temp = 1;

  if((cap1166_handle == NULL) || (in_sens == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_MAIN_CONTROL_REG,
                                 &int_check);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* mask INT bit */
  cap1166_handle->cfg_byte_data = CAP1166_INT_MASK;

  /* clear INT bit => clear all bits on Sensor Input Status Register */
  retval = cap1166_clear_register_bits(cap1166_handle->spidrv_handle,
                                       CAP1166_MAIN_CONTROL_REG,
                                       cap1166_handle->cfg_byte_data);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* check whether which button is detected? */
  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_SENS_IN_STATUS_REG,
                                 &touch_check);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* check whether the receive data is valid? */
  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_REG_NOISE_FLAG_STATUS,
                                 &noise_check);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  for(index = 0; index < 6; index++){
      if((touch_check & temp) && (!( noise_check & temp))){
          /* which input touch is detected */
          if(!(cap1166_handle->release_check & 0x01)){
              in_sens[index] = CAP1166_BUTTON_PRESSED;
              cap1166_handle->input_check[index] = CAP1166_BUTTON_PRESSED;
          }
      }
      else if(cap1166_handle->input_check[index]){
          /* which input touch is released */
          in_sens[index] = CAP1166_BUTTON_RELEASED;
          cap1166_handle->input_check[index] = CAP1166_BUTTON_RELEASED;
      }
      else{
          /* touch is not detected */
          in_sens[index] = CAP1166_BUTTON_NOT_DETECTED;
      }
      temp <<= 1;
  }

  return retval;
}

/***************************************************************************//**
 *  Puts device in power mode and enables desired inputs in that power mode.
 ******************************************************************************/
sl_status_t cap1166_set_power_mode(cap1166_handle_t *cap1166_handle,
                                   power_state_t state,
                                   uint8_t analog_gain,
                                   sensor_inputs_cfg_t *sensor_inputs_cfg)
{
  sl_status_t retval = SL_STATUS_OK;

  if((cap1166_handle == NULL) || (sensor_inputs_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  if(!((analog_gain == CAP1166_GAIN_1) ||
      (analog_gain == CAP1166_GAIN_2) ||
      (analog_gain == CAP1166_GAIN_4) ||
      (analog_gain == CAP1166_GAIN_8))){
      return SL_STATUS_INVALID_PARAMETER;
  }

  /* sensor input enable */
  retval = cap1166_sensor_enable(cap1166_handle,
                                 state,
                                 sensor_inputs_cfg->sensor_en);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* sensor interrupt enable */
  retval = cap1166_sensor_int_enable(cap1166_handle,
                                     sensor_inputs_cfg->sensor_interrupt_en);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  switch(state){
    case CAP1166_ACTIVE:
      /* enter in active mode */
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_MAIN_CONTROL_REG,
                                      analog_gain);
      break;

    case CAP1166_STANDBY:
      /* enter in standby mode */
      retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                      CAP1166_MAIN_CONTROL_REG,
                                      analog_gain | CAP1166_SET_STANDBY_MODE );
      if(retval != ECODE_EMDRV_SPIDRV_OK){
        return SL_STATUS_TRANSMIT;
      }
      break;

    case CAP1166_DEEP_SLEEP:
      /* enter in deep sleep mode */
      retval = cap1166_set_register_bits(cap1166_handle->spidrv_handle,
                                         CAP1166_MAIN_CONTROL_REG,
                                         CAP1166_SET_SLEEP_MODE);
      if(retval != ECODE_EMDRV_SPIDRV_OK){
        return SL_STATUS_TRANSMIT;
      }
      break;

    default:
      return SL_STATUS_INVALID_PARAMETER;
      break;
  }

  return retval;
}

/***************************************************************************//**
 *  Function to check interrupt reason
 ******************************************************************************/
sl_status_t cap1166_check_interrupt_reason(cap1166_handle_t *cap1166_handle,
                                           uint8_t *interrupt_reason)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t int_check;

  if((cap1166_handle == NULL) || (interrupt_reason == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_MAIN_CONTROL_REG,
                                 &int_check);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  if((int_check & 0x01) != 0x01){
      /* there is no interrupt reason */
      return SL_STATUS_FAIL;
  }

  /* read status register */
  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_GEN_STATUS_REG,
                                 interrupt_reason);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* clear INT bit => clear all bits on Sensor Input Status Register
   * for active mode */
  retval = cap1166_clear_register_bits(cap1166_handle->spidrv_handle,
                                       CAP1166_MAIN_CONTROL_REG,
                                       CAP1166_INT_MASK);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Resets device function
 ******************************************************************************/
sl_status_t cap1166_reset(cap1166_handle_t *cap1166_handle)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t reset_state = 0;
  uint8_t timeout = 0;

  if(cap1166_handle == NULL){
      return SL_STATUS_NULL_POINTER;
  }

  GPIO_PinModeSet(cap1166_handle->sensor_rst_port,
                  cap1166_handle->sensor_rst_pin,
                  gpioModePushPull,
                  0);

  GPIO_PinOutSet(cap1166_handle->sensor_rst_port,
                 cap1166_handle->sensor_rst_pin);
  sl_sleeptimer_delay_millisecond(170);

  GPIO_PinOutClear(cap1166_handle->sensor_rst_port,
                   cap1166_handle->sensor_rst_pin);
  sl_sleeptimer_delay_millisecond(200);

  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_GEN_STATUS_REG,
                                 &reset_state);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  /* polling RST bit */
  while((reset_state & CAP1166_RESET_INT_MASK) == 0)
  {
      sl_sleeptimer_delay_millisecond(20);
      retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                     CAP1166_GEN_STATUS_REG,
                                     &reset_state);
      if(retval != SL_STATUS_OK){
          return retval;
      }

      timeout++;
      if(timeout == 10){
          return SL_STATUS_TIMEOUT;
      }
  }

  if(!(reset_state & CAP1166_RESET_INT_MASK)){
      retval = SL_STATUS_FAIL;
  }

  return retval;
}

/***************************************************************************//**
 *  Read out Cap1166 Revision and ID
 ******************************************************************************/
sl_status_t cap1166_identify(cap1166_handle_t *cap1166_handle,
                             uint8_t *partId,
                             uint8_t *partRev)
{
  sl_status_t retval = SL_STATUS_OK;

  if((cap1166_handle == NULL) || (partId == NULL) || (partRev == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  /* read partId */
  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_PRODUCT_ID_REG,
                                 partId);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  /* read partRev */
  retval = cap1166_read_register(cap1166_handle->spidrv_handle,
                                 CAP1166_REVISION_REG,
                                 partRev);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  return retval;
}

/***************************************************************************//**
 *  Configures LED output
 ******************************************************************************/
sl_status_t  cap1166_led_config(cap1166_handle_t *cap1166_handle,
                                led_cfg_t *cap1166_led_cfg)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t led_output_type_value = 0;
  uint8_t led_link_to_sensor_value = 0;
  uint8_t led_polarity_value = 0;
  uint8_t led_mirror_value = 0;
  uint16_t led_behavior_value = 0;

  if((cap1166_handle == NULL) || (cap1166_led_cfg == NULL)){
      return SL_STATUS_NULL_POINTER;
  }

  for(int index = 0; index < 6; index++){
      /* set output type */
      if(cap1166_led_cfg->led_output_type[index] ==
          CAP1166_OUTPUT_PUSH_PULL){
          led_output_type_value |= 1<<index;
      }
      else {
          led_output_type_value &= ~(1<<index);
      }

      /* set output link to sensor */
      if(cap1166_led_cfg->led_link_to_sensor[index] ==
          CAP1166_OUTPUT_LINK_TO_SENSOR){
          led_link_to_sensor_value |= 1<<index;
      }
      else {
          led_link_to_sensor_value &= ~(1<<index);
      }

      /* set output polarity */
      if(cap1166_led_cfg->led_output_polarity[index] ==
          CAP1166_OUTPUT_NON_INVERTED){
          led_polarity_value |= 1<<index;
      }
      else {
          led_polarity_value &= ~(1<<index);
      }

      /* set output mirror */
      if(cap1166_led_cfg->led_output_mirror[index] ==
          CAP1166_OUTPUT_MIRROR){
          led_mirror_value |= 1<<index;
      }
      else {
          led_mirror_value &= ~(1<<index);
      }

      /* set led behavior */
      if(cap1166_led_cfg->led_behavior[index] ==
          CAP1166_OUTPUT_PULSE_1_MODE){
          led_behavior_value |= 1<<(index*2);
      }
      else if(cap1166_led_cfg->led_behavior[index] ==
          CAP1166_OUTPUT_PULSE_2_MODE){
          led_behavior_value |= 2<<(index*2);
      }
      else if(cap1166_led_cfg->led_behavior[index] ==
          CAP1166_OUTPUT_BREATH_MODE){
          led_behavior_value |= 3<<(index*2);
      }
  }

  /* write CAP1166_LED_OUTPUT_TYPE_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_OUTPUT_TYPE_REG,
                                  led_output_type_value);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_SENS_IN_LED_LINK_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_SENS_IN_LED_LINK_REG,
                                  led_link_to_sensor_value);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_POLARITY_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_POLARITY_REG,
                                  led_polarity_value);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_MIRROR_CON_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_MIRROR_CON_REG,
                                  led_mirror_value);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_BEHAVIOR_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_BEHAVIOR1_REG,
                                  (uint8_t)(led_behavior_value & 0xff));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_BEHAVIOR2_REG,
                                  (uint8_t)(led_behavior_value >> 8));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_PULSE1_PERIOD_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_PULSE1_PERIOD_REG,
                                  (cap1166_led_cfg->led_pulse_1_trigger_mode |
                                  cap1166_led_cfg->led_pulse_1_period));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_PULSE2_PERIOD_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_PULSE2_PERIOD_REG,
                                  cap1166_led_cfg->led_pulse_2_period);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_BREATHE_PERIOD_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_BREATHE_PERIOD_REG,
                                  cap1166_led_cfg->led_breath_period);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write pulse count and led alert configuration */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_CONFIG_REG,
                                  (cap1166_led_cfg->led_ramp_alert |
                                  cap1166_led_cfg->led_pulse_1_pulse_count |
                                  cap1166_led_cfg->led_pulse_2_pulse_count));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_PULSE1_DUTY_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_PULSE1_DUTY_REG,
                                  (cap1166_led_cfg->led_pulse_1_max_duty |
                                  cap1166_led_cfg->led_pulse_1_min_duty));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_PULSE2_DUTY_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_PULSE2_DUTY_REG,
                                  (cap1166_led_cfg->led_pulse_2_max_duty |
                                  cap1166_led_cfg->led_pulse_2_min_duty));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_BREATHE_DUTY_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_BREATHE_DUTY_REG,
                                  (cap1166_led_cfg->led_breath_max_duty |
                                  cap1166_led_cfg->led_breath_min_duty));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_DIRECT_DUTY_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_DIRECT_DUTY_REG,
                                  (cap1166_led_cfg->led_direct_max_duty |
                                  cap1166_led_cfg->led_direct_min_duty));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_DIRECT_RAMP_RATES_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_DIRECT_RAMP_RATES_REG,
                                  (cap1166_led_cfg->led_direct_rise_rate |
                                  cap1166_led_cfg->led_direct_fall_rate));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write CAP1166_LED_OFF_DELAY_REG */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_OFF_DELAY_REG,
                                  (cap1166_led_cfg->led_direct_off_delay |
                                  cap1166_led_cfg->led_breath_off_delay));
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Sets LED output in direct mode
 ******************************************************************************/
sl_status_t  cap1166_led_direct_set(cap1166_handle_t *cap1166_handle,
                                    uint8_t led_index,
                                    uint8_t state)
{
  sl_status_t retval = SL_STATUS_OK;
  static uint8_t led_control = 0x00;

  if(cap1166_handle == NULL){
      return SL_STATUS_NULL_POINTER;
  }

  if(led_index > 5){
      return SL_STATUS_INVALID_PARAMETER;
  }

  cap1166_handle->output_direct_control[led_index] = state;

  if(state){
      led_control |= 1<<led_index;
  }
  else {
      led_control &= ~(1<<led_index);
  }

  /* write LED control reg */
  retval = cap1166_write_register(cap1166_handle->spidrv_handle,
                                  CAP1166_LED_OUT_CON_REG,
                                  led_control);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Resets SPI communication between MCU and cap1166 touch sensor.
 ******************************************************************************/
static sl_status_t cap1166_reset_spi_interface(SPIDRV_Handle_t spidrv_handle,
                                               cap1166_command_t cmd)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t spi_write_data[2];

  spi_write_data[0] = cmd;
  spi_write_data[1] = cmd;

  retval = SPIDRV_MTransmitB(spidrv_handle, spi_write_data, 2);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Sets address pointer SPI communication between MCU and cap1166 touch sensor.
 ******************************************************************************/
static sl_status_t cap1166_set_address_pointer(SPIDRV_Handle_t spidrv_handle,
                                                  uint8_t reg,
                                                  cap1166_command_t cmd)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t spi_write_data[2];

  spi_write_data[0] = cmd;
  spi_write_data[1] = reg;

  retval = SPIDRV_MTransmitB(spidrv_handle, spi_write_data, 2);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Reads register from the cap1166 touch sensor
 ******************************************************************************/
static sl_status_t cap1166_read_register(SPIDRV_Handle_t spidrv_handle,
                                         uint8_t reg,
                                         uint8_t *data)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t spi_write_data[1];

  if(data == NULL){
      return SL_STATUS_NULL_POINTER;
  }

  spi_write_data[0] = READ_CMD;

  /* reset SPI communication */
  retval = cap1166_reset_spi_interface(spidrv_handle, RESET_CMD);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      *data = 0;
      return SL_STATUS_TRANSMIT;
  }

  /* set address pointer */
  retval = cap1166_set_address_pointer(spidrv_handle,
                                       reg,
                                       SET_ADDRESS_POINTER_CMD);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      *data = 0;
      return SL_STATUS_TRANSMIT;
  }

  /*
   * remove the invalid data cased by The first read command after any
   * other command will return invalid data
   */
  retval = SPIDRV_MTransmitB(spidrv_handle, spi_write_data, 1);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      *data = 0;
      return SL_STATUS_TRANSMIT;
  }

  /* read data */
  retval = SPIDRV_MTransferB(spidrv_handle, spi_write_data, data, 1);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      *data = 0;
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Writes a register in the cap1166 touch sensor
 ******************************************************************************/
static sl_status_t cap1166_write_register(SPIDRV_Handle_t spidrv_handle,
                                  uint8_t reg,
                                  uint8_t data)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t spi_write_data[2];

  spi_write_data[0] = WRITE_CMD;
  spi_write_data[1] = data;

  /* reset SPI communication */
  retval = cap1166_reset_spi_interface(spidrv_handle, RESET_CMD);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* set address pointer */
  retval = cap1166_set_address_pointer(spidrv_handle,
                                       reg,
                                       SET_ADDRESS_POINTER_CMD);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write data */
  retval = SPIDRV_MTransmitB(spidrv_handle, spi_write_data, 2);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Writes a block of data to the cap1166 touch sensor.
 ******************************************************************************/
static sl_status_t cap1166_write_register_block(SPIDRV_Handle_t spidrv_handle,
                                           uint8_t reg,
                                           uint8_t length,
                                           const uint8_t *data)
{
  sl_status_t retval = SL_STATUS_OK;
  uint8_t spi_write_data[length + 1];
  uint8_t index;

  if(data == NULL){
      return SL_STATUS_NULL_POINTER;
  }

  spi_write_data[0] = WRITE_CMD;
  for (index = 1; index < length + 1; index++){
      spi_write_data[index] = data[index - 1];
  }

  /* reset SPI communication */
  retval = cap1166_reset_spi_interface(spidrv_handle, RESET_CMD);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* set address pointer */
  retval = cap1166_set_address_pointer(spidrv_handle,
                                       reg,
                                       SET_ADDRESS_POINTER_CMD);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  /* write block data */
  retval = SPIDRV_MTransmitB(spidrv_handle, spi_write_data, length + 1);
  if(retval != ECODE_EMDRV_SPIDRV_OK){
      return SL_STATUS_TRANSMIT;
  }

  return retval;
}

/***************************************************************************//**
 *  Sets the given bit(s) in a register in the cap1166 touch sensor
 ******************************************************************************/
static sl_status_t cap1166_set_register_bits(SPIDRV_Handle_t spidrv_handle,
                                             uint8_t reg,
                                             uint8_t mask)
{
  uint8_t value;
  sl_status_t retval = SL_STATUS_OK;

  retval = cap1166_read_register(spidrv_handle, reg, &value);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  value |= mask;

  retval = cap1166_write_register(spidrv_handle, reg, value);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  return retval;
}

/***************************************************************************//**
 *  Clears the given bit(s) in a register in the cap1166 touch sensor
 ******************************************************************************/
static sl_status_t cap1166_clear_register_bits(SPIDRV_Handle_t spidrv_handle,
                                               uint8_t reg,
                                               uint8_t mask)
{
  uint8_t value;
  sl_status_t retval = SL_STATUS_OK;

  retval = cap1166_read_register(spidrv_handle, reg, &value);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  value &= ~mask;

  retval = cap1166_write_register(spidrv_handle, reg, value);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  return retval;
}

/***************************************************************************//**
 *  Writes the given bit(s) in a register in the cap1166 touch sensor
 ******************************************************************************/
static sl_status_t cap1166_write_register_bits(SPIDRV_Handle_t spidrv_handle,
                                               uint8_t reg,
                                               uint8_t pos,
                                               uint8_t mask)
{
  sl_status_t retval = SL_STATUS_OK;

  /* clear the bit field */
  retval = cap1166_clear_register_bits(spidrv_handle, reg, pos);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  /* write to bit field */
  retval = cap1166_set_register_bits(spidrv_handle, reg, mask);
  if(retval != SL_STATUS_OK){
      return retval;
  }

  return retval;
}
/** @} (end group cap1166) */
