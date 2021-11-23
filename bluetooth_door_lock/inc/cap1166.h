/***************************************************************************//**
* @file cap1166.h
* @brief define driver structures and APIs for the cap1166 Capacitive
*        Touch Sensor
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
#ifndef __SL_CAP1166_H__
#define __SL_CAP1166_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "spidrv.h"
#include "sl_status.h"

#include "em_gpio.h"

#include "cap1166_regs.h"
#include "cap1166_config.h"

#include "sl_sleeptimer.h"

/***************************************************************************//**
 * @addtogroup cap1166 Cap1166 - Capacitive Touch Sensor
 *
 * @brief Driver for the Silicon Labs Cap1166 Capacitive Touch Sensor

 @n @section cap1166_example cap1166 example code

   Basic example for touch sensor: @n @n
   @verbatim

 #include "cap1166.h"

 int main( void )
 {

   ...

   cap1166_init(...);
   cap1166_config(...);

   ...

   cap1166_detect_touch(..., &out);

   ...

 } @endverbatim
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   led_ouput_type_t enum
 ******************************************************************************/
typedef enum {
  CAP1166_OUTPUT_OPEN_DRAIN,
  CAP1166_OUTPUT_PUSH_PULL
} led_ouput_type_t;

/***************************************************************************//**
 * @brief
 *   link_to_sensor_t enum
 ******************************************************************************/
typedef enum {
  CAP1166_OUTPUT_DO_NOT_LINK_TO_SENSOR,
  CAP1166_OUTPUT_LINK_TO_SENSOR
} led_link_to_sensor_t;

/***************************************************************************//**
 * @brief
 *   led_behavior_t enum
 ******************************************************************************/
typedef enum {
  CAP1166_OUTPUT_DIRECT_MODE,
  CAP1166_OUTPUT_PULSE_1_MODE,
  CAP1166_OUTPUT_PULSE_2_MODE,
  CAP1166_OUTPUT_BREATH_MODE,
} led_behavior_t;

/***************************************************************************//**
 * @brief
 *   led_polarity_output_t enum
 ******************************************************************************/
typedef enum {
  CAP1166_OUTPUT_INVERTED,
  CAP1166_OUTPUT_NON_INVERTED
} led_polarity_output_t;

/***************************************************************************//**
 * @brief
 *   mirror_output_t enum
 ******************************************************************************/
typedef enum {
  CAP1166_OUTPUT_DO_NOT_MIRROR,
  CAP1166_OUTPUT_MIRROR
} led_mirror_output_t;

/***************************************************************************//**
 * @brief
 *   led_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /* Identify output type of the LEDs
   *  Assignable macros :
   *  - CAP1166_OUTPUT_OPEN_DRAIN
   *  - CAP1166_OUTPUT_PUSH_PULL
   */
  led_ouput_type_t led_output_type[6];

  /* Identify to whether a capacitive touch sensor input is linked to an
   * LED output
   *  Assignable macros :
   *  - CAP1166_OUTPUT_DO_NOT_LINK_TO_SENSOR
   *  - CAP1166_OUTPUT_LINK_TO_SENSOR
   */
  led_link_to_sensor_t led_link_to_sensor[6];

  /* Control the operation of LEDs
   *  Assignable macros :
   *  - CAP1166_OUTPUT_DIRECT_MODE,
   *  - CAP1166_OUTPUT_PULSE_1_MODE,
   *  - CAP1166_OUTPUT_PULSE_2_MODE,
   *  - CAP1166_OUTPUT_BREATH_MODE,
   */
  led_behavior_t led_behavior[6];

  /* Identify the logical polarity of the LED outputs
   *  Assignable macros :
   *  - CAP1166_OUTPUT_INVERTED
   *  - CAP1166_OUTPUT_NON_INVERTED
   */
  led_polarity_output_t led_output_polarity[6];

  /* Determine the meaning of duty cycle settings when polarity is non-inverted
   * for each LED channel
   *  Assignable macros :
   *  - CAP1166_OUTPUT_DO_NOT_MIRROR
   *  - CAP1166_OUTPUT_MIRROR
   */
  led_mirror_output_t led_output_mirror[6];

  /* Determines whether the device will assert the interrupt pin (ALERT# pin) or
   * not when LEDs actuated by the LED Output Control register bit have finished
   * their respective behaviors.
   *  Assignable macros :
   *  - CAP1166_ALERT_NOT_ASSERTED_ON_LED_ACT
   *  - CAP1166_ALERT_ASSERTED_ON_LED_ACT
   */
  uint8_t led_ramp_alert;

  /* Determines the start trigger for the LED Pulse behavior which on touch or
   * release
   *  Assignable macros :
   *  - CAP1166_LED_PULSE_ON_TOUCH
   *  - CAP1166_LED_PULSE_ON_RELEASE
   */
  uint8_t led_pulse_1_trigger_mode;

  /* Determines the overall period of a pulse 1 operation
   *  Assignable macros :
   *  - CAP1166_LED_PULSE_PERIOD_32MILISEC
   *  - CAP1166_LED_PULSE_PERIOD_4064MILISEC
   */
  uint8_t led_pulse_1_period;

  /* Determines the number of pulses used for the Pulse 1 behavior
   *  Assignable macros :
   *  - CAP1166_PULSE2_3_PULSES
   *  - ...
   */
  uint8_t led_pulse_1_pulse_count;

  /* Determines the maximum PWM duty cycle for the LED  Pulse 1 Duty Cycle
   *  - CAP1166_MAX_DUTY_100_PERCENTS
   *  - ...
   */
  uint8_t led_pulse_1_max_duty;

  /* Determines the minimum PWM duty cycle for the LED  Pulse 1 Duty Cycle
   *  Assignable macros :
   *  - CAP1166_MIN_DUTY_0_PERCENT
   *  - ...
   */
  uint8_t led_pulse_1_min_duty;

  /*
   * LED Pulse 2 Duty Cycle
   */
  uint8_t led_pulse_2_period;
  uint8_t led_pulse_2_pulse_count;
  uint8_t led_pulse_2_max_duty;
  uint8_t led_pulse_2_min_duty;

  /* Determines the overall period of a breathe operation
   *  Assignable macros :
   *  - CAP1166_LED_PULSE_PERIOD_32MILISEC
   *  - CAP1166_LED_PULSE_PERIOD_4064MILISEC
   */
  uint8_t led_breath_period;

  /* Determines the maximum PWM duty cycle for the LED Breathe Duty Cycle
   *  - CAP1166_MAX_DUTY_100_PERCENTS
   *  - ...
   */
  uint8_t led_breath_max_duty;

  /* Determines the minimum PWM duty cycle for the LED Breathe Duty Cycle
   *  Assignable macros :
   *  - CAP1166_MIN_DUTY_0_PERCENT
   *  - ...
   */
  uint8_t led_breath_min_duty;

  /* Determines the Breathe behavior mode off delay
   *  Assignable macros :
   *  - CAP1166_BR_OFF_DLY_0MS
   *  - ...
   */
  uint8_t led_breath_off_delay;

  /*  Determines the rising edge time
   *  Assignable macros :
   *  - CAP1166_RISE_RATE_0MS
   *  - ...
   */
  uint8_t led_direct_rise_rate;

  /* Determines the falling edge time
   *  Assignable macros :
   *  - CAP1166_FALL_RATE_0MS
   *  - ...
   */
  uint8_t led_direct_fall_rate;

  /* Determines the maximum PWM duty cycle for the Direct Duty Cycle
   *  - CAP1166_MAX_DUTY_100_PERCENTS
   *  - ...
   */
  uint8_t led_direct_max_duty;

  /* Determines the minimum PWM duty cycle for the Direct Duty Cycle
   *  Assignable macros :
   *  - CAP1166_MIN_DUTY_0_PERCENT
   *  - ...
   */
  uint8_t led_direct_min_duty;

  /*  Determines the turn-off delay
   *  Assignable macros :
   *  - CAP1166_DR_OFF_DLY_0MS
   *  - ...
   */
  uint8_t led_direct_off_delay;
} led_cfg_t;

/***************************************************************************//**
 * @brief
 *   power_state_t enum
 ******************************************************************************/
typedef enum {
  CAP1166_ACTIVE,
  CAP1166_STANDBY,
  CAP1166_DEEP_SLEEP
} power_state_t;

/***************************************************************************//**
 * @brief
 *   recalib_mode_t enum
 ******************************************************************************/
typedef enum {
  CAP1166_MANUAL_CALIB,
  CAP1166_AUTO_CALIB,
  CAP1166_NEG_DELTA_CALIB,
  CAP1166_DELAY_CALIB
} recalib_mode_t;

/***************************************************************************//**
 * @brief
 *   cap1166_command_t enum
 ******************************************************************************/
typedef enum {
  RESET_CMD = 0x7A,
  SET_ADDRESS_POINTER_CMD = 0x7D,
  WRITE_CMD = 0x7E,
  READ_CMD = 0x7F
} cap1166_command_t;

/***************************************************************************//**
 * @brief
 *   press_and_hold_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /* Release detection
   *  Assignable macros :
   *  - CAP1166_DETECT_RELEASE_EN
   *  - CAP1166_DETECT_RELEASE_DIS
   */
  uint8_t release_detection_en;

  /* The time duration between interrupt assertions
   *  Assignable macros :
   *  - CAP1166_INTERR_REPEAT_RATE_35MILISEC
   *  - ...
   */
  uint8_t set_repeat_rate;

  /*
   * The minimum amount of time that flagged a "press and hold" event or
   *  a touch event is detected
   * Assignable macros :
   * - CAP1166_PRESS_AND_HOLD_EVENT_AFTER_35MILISEC
   * - ...
   */
  uint8_t hold_time;
} press_and_hold_cfg_t;

/***************************************************************************//**
 * @brief
 *   sensor_inputs_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /* Identify an expected capacitive touch sensor input which included in
   *  the sampling cycle.
   *  Assignable macros :
   *  - CAP1166_SENSOR_INPUT_ENABLE
   *  - CAP1166_SENSOR_INPUT_DISABLE
   */
  uint8_t sensor_en[6];

  /* Identify an expected capacitive touch sensor input which will be trigger
   * an interrupt.
   *  Assignable macros :
   *  - CAP1166_SENSOR_INTERUPT_ENABLE
   *  - CAP1166_SENSOR_INTERUPT_DISABLE
   */
  uint8_t sensor_interrupt_en[6];

  /* Sensor inputs threshold in active mode
   *  Assignable macros :
   *  - CAP1166_SENS_IN_THRESHOLD_1X
   *  - ...
   */
  uint8_t sensor_threshold_active[6];

  /* Sensor inputs threshold in standby mode
   *  Assignable macros :
   *  - CAP1166_STBY_THRESHOLD_1X
   *  - ...
   */
  uint8_t sensor_threshold_standy;

  /* Identify an expected capacitive touch sensor input which will be enable
   *  repeat rate.
   *  Assignable macros :
   *  - CAP1166_SENSOR_REPEAT_ENABLE
   *  - CAP1166_SENSOR_REPEAT_DISABLE
   */
  uint8_t sensor_repeat_rate_en[6];
} sensor_inputs_cfg_t;

/***************************************************************************//**
 * @brief
 *   sensor_noise_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /* Sensor threshold noise
   *  Assignable macros :
   *  - CAP1166_SENS_IN_THRESHOLD_25_PERCENT
   */
  uint8_t noise_threshold;

  /* Digital noise threshold
   *  Assignable macros :
   *  - CAP1166_DIG_NOISE_THRESHOLD_EN
   *  - CAP1166_DIG_NOISE_THRESHOLD_DIS
   */
  uint8_t digital_noise_threshold;

  /* Analog noise filter
   *  Assignable macros :
   *  - CAP1166_AN_NOISE_FILTER_EN
   *  - CAP1166_AN_NOISE_FILTER_DIS
   */
  uint8_t analog_noise_filter;

  /* RF noise filter
   *  Assignable macros :
   *  - CAP1166_RF_NOISE_FILTER_EN
   *  - CAP1166_RF_NOISE_FILTER_DIS
   */
  uint8_t RF_noise_filter;

  /* Show RF Noise
   *  Assignable macros :
   *  - CAP1166_SHOW_LOW_FREQ_NOISE
   *  - CAP1166_NOT_SHOW_LOW_FREQ_NOISE
   */
  uint8_t show_low_frequency_noise;
} sensor_noise_cfg_t;

/***************************************************************************//**
 * @brief
 *   sensitivity_control_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /* Setting sensitivity in active mode
   *  Assignable macros :
   *  - CAP1166_SENSITIVITY_MULTIPLIER_1X
   *  - ...
   */
  uint8_t sens_active;

  /* Setting the scaling and data presentation of the Base Count registers
   *  Assignable macros :
   *   - CAP1166_DATA_SCALING_FACTOR_1X
   *   - ...
   */
  uint8_t base_shift;

  /* Setting sensitivity in standby mode
   *  Assignable macros :
   *  - CAP1166_STBY_SENSE_MULTIPLIER_1X
   *  - ...
   */
  uint8_t sens_stby;
} sensitivity_control_cfg_t;

/***************************************************************************//**
 * @brief
 *   sensing_cycle_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /* The number of samples that are taken for all active channels during
   * the sensor cycle in active mode or standby mode
   *  Assignable macros :
   *  - CAP1166_STBY_AVERAGE_8
   *  - ...
   */
  uint8_t number_samples_per_cycle;

  /* The time to take a single sample in active mode or standby mode
   *  Assignable macros :
   *  - AP1166_STBY_SAMPLE_TIME_320US
   *  - ...
   */
  uint8_t sample_time;

  /* The overall cycle time for all measured channels during normal operation
   * in active mode or standby mode
   *  Assignable macros :
   *  - CAP1166_STBY_CYCLE_TIME_70MS
   *  - ...
   */
  uint8_t overall_cycle_time;
} sensing_cycle_cfg_t;

/***************************************************************************//**
 * @brief
 *  recalib_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /* Re-calibrate mode selection
   *  Assignable macros :
   *   - CAP1166_MANUAL_CALIB,
   *   - CAP1166_AUTO_CALIB,
   *   - CAP1166_NEG_DELTA_CALIB,
   *   - CAP1166_DELAY_CALIB
   */
  recalib_mode_t recalib_mode;

  /* Manual Re-calibration
   *  Assignable macros :
   *  - CAP1166_SENSOR_RECALIB_ENABLE
   *  - CAP1166_SENSOR_RECALIB_DISABLE
   */
  uint8_t sensor_recalib[6];

  /* Delayed Re-calibration
   *  Assignable macros :
   *  - CAP1166_MAX_DUR_RECALIB_EN
   *  - CAP1166_MAX_DUR_RECALIB_DIS
   */
  uint8_t max_during_en;

  /* The maximum time that a sensor pad is allowed to be touched until the
   * capacitive touch sensor input is re-calibrated
   *  Assignable macros :
   *  - CAP1166_560MILISEC_BEFORE_RECALIB
   *  - ...
   */
  uint8_t delay_recalib;

  /* Negative Delta Count Re-calibration
   *  Assignable macros :
   *  - CAP1166_CONS_NEG_DELTA_CNT_8
   *  - ...
   */
  uint8_t neg_del_cnt;

  /* Automatic Re-calibration
   *  Assignable macros :
   *  - CAP1166_REC_SAMPLES_16_UPDT_TIME_16
   *  - ...
   */
  uint8_t rate_recalib;
} recalib_cfg_t;

/***************************************************************************//**
 * @brief
 *   proximity_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /*
   * The sensing_cycle_t instance
   */
  sensing_cycle_cfg_t standby_sensing_cycle;

  /* Proximity detection mode
   *  Assignable macros :
   *   - CAP1166_AVERAGE_BASED
   *   - CAP1166_SUMMATION_BASED
   */
  uint8_t sum_avr_mode;
} proximity_cfg_t;

/**************************************************************************/ /**
 * @brief
 *   pattern_cfg_t object definition.
 ******************************************************************************/
typedef struct
{
  /* Sensor threshold in MTP detection
   *  Assignable Macros :
   *  - CAP1166_MULTIPLE_THRESHOLD_12_PERCENTS
   *  - ...
   */
  uint8_t threshold_percent;

  /* Multiple touch pattern mode
   *  Assignable Macros :
   *   - CAP1166_MODE_PATTERN_RECOGNITION
   *   - CAP1166_MODE_MLTP_TOUCH_PATTERN
   */
  uint8_t MTP_mode;

  /* Determines whether the device will assert the interrupt pin (ALERT# pin)
   * or not If an MTP event occurs
   *  Assignable Macros :
   *  - CAP1166_MLTP_EVENT_ALERT_EN
   *  - CAP1166_MLTP_EVENT_ALERT_DIS
   */
  uint8_t MTP_alert;

  /* Identify an expected sensor input profile for diagnostics or
   * other significant events
   *   Assignable Macros :
   *   - CAP1166_SENSOR_PATTERN_ENABLE
   *   - CAP1166_SENSOR_PATTERN_DISABLE
   */
  uint8_t MTP_sensor_inputs[6];
} pattern_cfg_t;

/**************************************************************************/ /**
 * @brief
 *   multi_touch_cfg_t object definition.
 ******************************************************************************/
typedef struct
{
  /* Number of simultaneous touches on all sensor pads before a Multiple Touch
   * Event is detected and sensor inputs are blocked
   *  Assignable Macros :
   *  - CAP1166_SIMUL_TOUCH_NUM_1
   *  - ...
   */
  uint8_t number_of_touch;

  /* Multiple touch detection circuitry is enable or not
   *  Assignable Macros :
   *  - CAP1166_MULTIPLE_BLOCK_EN
   *  - CAP1166_MULTIPLE_BLOCK_DIS
   */
  uint8_t multi_block_en;
} multi_touch_cfg_t;

/***************************************************************************//**
 * @brief
 *   cap1166_cfg_t object definition.
 ******************************************************************************/
typedef struct {
  /*
   * Sensor inputs configuration
   */
  sensor_inputs_cfg_t sensor_inputs;

  /*
   * Sensing cycle configuration
   */
  sensing_cycle_cfg_t sensing_cycle;

  /*
   * Sensitivity control configuration
   */
  sensitivity_control_cfg_t sensitivity_control;

  /*
   * Sensor noise configuration
   */
  sensor_noise_cfg_t sensor_noise;

  /*
   * Re-calibration configuration
   */
  recalib_cfg_t recalib;

  /*
   * Touch pattern detect configuration
   */
  pattern_cfg_t pattern_cfg;

  /*
   * Proximity detect configuration
   */
  proximity_cfg_t proximity_cfg;

  /*
   * Press and hold detect configuration
   */
  press_and_hold_cfg_t press_and_hold;

  /*
   * Multiple touch configuration
   */
  multi_touch_cfg_t multi_touch;

  /*
   * LED configuration
   */
  led_cfg_t led_cfg;

  /*
   * The interrupt pin (ALERT# pin) configuration
   */
  uint8_t alert_mode;

  /*
   * Analog gain configuration
   */
  uint8_t analog_gain;

  /*
   * Power state configuration
   */
  power_state_t power_state;
} cap1166_cfg_t;

/***************************************************************************//**
 * @brief
 *   cap1166_handle_t object definition.
 ******************************************************************************/
typedef struct {
  /*
   * The SPIDRV_Handle_t instance
   */
  SPIDRV_Handle_t spidrv_handle;

  /*
   * The GPIO port of reset pin
   */
  GPIO_Port_TypeDef sensor_rst_port;

  /*
   * Reset pin
   */
  uint8_t sensor_rst_pin;

  /*
   * Save the current status of LEDs
   */
  uint8_t output_direct_control[6];

  /*
   * Flag to check a release
   */
  uint8_t release_check;

  /*
   * Input check buffer
   */
  uint8_t input_check[6];

  /*
   * Byte data config
   */
  uint8_t cfg_byte_data;
} cap1166_handle_t;

/***************************************************************************//**
 * @addtogroup cap1166_details Cap1166 Details
 * @brief Register interface and implementation details
 * @{
 ******************************************************************************/

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Enables sensor inputs in active mode or standby mode.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] state
 *   The power_state_t instance to use.
 * @param[in] sensor_inputs_en
 *   Controls which sensor inputs will be active.
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_sensor_enable(cap1166_handle_t *cap1166_handle,
                                  power_state_t state,
                                  uint8_t *sensor_inputs_en);

/***************************************************************************//**
 * @brief
 *   Enables sensor inputs interrupt in active mode or standby mode.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] sensor_int_en
 *   Controls which sensor inputs will trigger an interrupt.
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_sensor_int_enable(cap1166_handle_t *cap1166_handle,
                                      uint8_t *sensor_int_en);

/***************************************************************************//**
 * @brief
 *   Enables sensor inputs repeat rate in active mode or standby mode.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] sensor_repeat_rate_en
 *   Controls which sensor inputs will have repeat rate enabled.
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_sensor_repeat_rate_enable(cap1166_handle_t *cap1166_handle,
                                              uint8_t *sensor_repeat_rate_en);

/***************************************************************************//**
 * @brief
 *   Configures the threshold for sensor inputs (CS1 to CS6).
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] state
 *   The power_state_t instance to use.
 * @param[in] sensor_threshold
 *   Threshold value for each sensor input.
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_sensor_threshold_config(cap1166_handle_t *cap1166_handle,
                                            power_state_t state,
                                            uint8_t *sensor_threshold);
/***************************************************************************//**
 * @brief
 *   Configures the parameters for re-calibration.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] recalib_cfg
 *   The recalib_cfg_t instance to use.
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_sensor_recalib_config(cap1166_handle_t *cap1166_handle,
                                          recalib_cfg_t *recalib_cfg);

/***************************************************************************//**
 * @brief
 *   Configures the parameters for noise detection.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] sensor_noise_cfg
 *   The sensor_noise_cfg_t instance to use.
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
 sl_status_t cap1166_sensor_noise_config(cap1166_handle_t *cap1166_handle,
                                         sensor_noise_cfg_t *sensor_noise_cfg);

/***************************************************************************//**
 * @brief
 *   Configures the sensitivity parameters for touch sensor in active or
 *   standby mode.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] state
 *   The power_state_t instance to use.
 * @param[in] sensitivity_control_cfg
 *   The sensitivity_control_cfg_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_sensor_sensitivity_config(
    cap1166_handle_t *cap1166_handle,
    power_state_t state,
    sensitivity_control_cfg_t *sensitivity_control_cfg);

/***************************************************************************//**
 * @brief
 *   Configures the sensing cycle parameters for touch sensor in active or
 *   standby mode.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] state
 *   The power_state_t instance to use.
 * @param[in] sensing_cycle_cfg
 *   The sensing_cycle_cfg_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_sensor_sensing_cycle_config(
    cap1166_handle_t *cap1166_handle,
    power_state_t state,
    sensing_cycle_cfg_t *sensing_cycle_cfg);

/***************************************************************************//**
 * @brief
 *   Configures number of simultaneous touches detection.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] multi_touch_cfg
 *   The multi_touch_cfg_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_multiple_touch_config(cap1166_handle_t *cap1166_handle,
                                          multi_touch_cfg_t *multi_touch_cfg);

/***************************************************************************//**
 * @brief
 *   Configures the parameters for press and hold detection.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] press_and_hold_cfg
 *   The press_and_hold_cfg_t instance to use.
 * @param[in] sensor_repeat_rate_en
 *   Controls which sensor inputs will have repeat rate enabled.
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_press_and_hold_config(
    cap1166_handle_t *cap1166_handle,
    press_and_hold_cfg_t *press_and_hold_cfg,
    uint8_t *sensor_repeat_rate_en);

/***************************************************************************//**
 * @brief
 *   Function Configures state of interrupt pin (ALERT# pin)
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] state
 *   The state of interrupt pin (ALERT# pin) when an interrupt was triggered.
 *    - <b>CAP1166_ALERT_ACTIVE_HIGH (0x00):</b> high level
 *    - <b>CAP1166_ALERT_ACTIVE_LOW (0x40):</b> low level
 *
 * @retval SL_STATUS_NULL_POINTER  Pointer to NULL;
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_int_pin_config(cap1166_handle_t *cap1166_handle,
                                   uint8_t state);

/***************************************************************************//**
 * @brief
 *   Initializes the cap1166 touch sensor.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_NOT_FOUND  Not found device
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 * @return SL_STATUS_TIMEOUT  Timeout
 ******************************************************************************/
sl_status_t  cap1166_init(cap1166_handle_t *cap1166_handle);

/***************************************************************************//**
 * @brief
 *   Configures the cap1166 touch sensor.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] cap1166_cfg
 *   The cap1166_cfg_t instance to use.
 *
 * @note: Standby mode should be used when fewer sensor inputs are enabled,
 *   and when they are programmed to have more sensitivity.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t  cap1166_config(cap1166_handle_t *cap1166_handle,
                            cap1166_cfg_t *cap1166_cfg);

/***************************************************************************//**
 * @brief
 *   Enables the proximity detection for cap1166 touch sensor.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] promixity_cfg
 *   The proximity_cfg_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t  cap1166_proximity_detection_enable(
    cap1166_handle_t *cap1166_handle,
    proximity_cfg_t *promixity_cfg);

/***************************************************************************//**
 * @brief
 *   Enables the multiple touch pattern for cap1166 touch sensor.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] pattern_cfg
 *   The pattern_cfg_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t  cap1166_touch_pattern_enable(cap1166_handle_t *cap1166_handle,
                                          pattern_cfg_t *pattern_cfg);

/***************************************************************************//**
 * @brief
 *   Detects touch on sensor inputs to determine which one is detected or
 *   released.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param [out] in_sens
 *   Results of detecting touch on sensor inputs.
 *   When in_sens[index] = 2 - released detection
 *        in_sens[index] = 1 - touched detection
 *        in_sens[index] = 0 - no touch detection
 *  index which means the sensor inputs touch in range CS1 to CS6.
 *
 * @note
 * <pre>
 *     Function detects touch on sensor inputs and checks if touch is detected
 *     or if touch is released.
 *     If the noise flag was set corresponding to sensor input channel,
 *     the touch will not be detected.
 * </pre>
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t  cap1166_detect_touch(cap1166_handle_t *cap1166_handle,
                                  uint8_t *in_sens);

/***************************************************************************//**
 * @brief
 *   Read out cap1166 Revision and ID.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[out] partId
 *   Cap1166 part ID.
 * @param[out] partRev
 *   Cap1166 part Revision.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 ******************************************************************************/
sl_status_t cap1166_identify(cap1166_handle_t *cap1166_handle,
                             uint8_t *partId,
                             uint8_t *partRev);

/***************************************************************************//**
 * @brief
 *   Function sets power mode for cap1166.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] state
 *   The power_state_t instance to use.
 * @param[in] analog_gain
 *   Possible gain value is 1, 2, 4 or 8.
 *    - <b>CAP1166_GAIN_1 (0x00):</b> gain 1
 *    - <b>CAP1166_GAIN_2 (0x40):</b> gain 2
 *    - <b>CAP1166_GAIN_4 (0x80):</b> gain 4
 *    - <b>CAP1166_GAIN_8 (0xC0):</b> gain 8
 * @param[in] sensor_inputs_cfg
 *   The sensor_inputs_cfg_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @retval SL_STATUS_OK  Success
 * @retval SL_STATUS_TRANSMIT  SPI transmission error
 * @retval SL_STATUS_INVALID_PARAMETER  Parameter is invalid
 ******************************************************************************/
sl_status_t cap1166_set_power_mode(cap1166_handle_t *cap1166_handle,
                                   power_state_t state,
                                   uint8_t analog_gain,
                                   sensor_inputs_cfg_t *sensor_inputs_cfg);

/***************************************************************************//**
 * @brief
 *   Function checks interrupt reason.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[out] interrupt_reason
 *   The source which was trigger an interrupt.
 *
 * @return SL_STATUS_OK  Success
 * @return SL_STATUS_NULL_POINTER  Pointer NULL pointer
 ******************************************************************************/
sl_status_t cap1166_check_interrupt_reason(cap1166_handle_t *cap1166_handle,
                                           uint8_t *interrupt_reason);

/***************************************************************************//**
 * @brief
 *   Function performs device reset.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @return SL_STATUS_TIMEOUT  Timeout
 * @return SL_STATUS_FAIL  Fail
 * @return SL_STATUS_OK  Success
 ******************************************************************************/
sl_status_t cap1166_reset(cap1166_handle_t *cap1166_handle);

/***************************************************************************//**
 * @brief
 *   LED configure function for cap1166.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] cap1166_led_cfg
 *   The led_cfg_t instance to use.
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @return SL_STATUS_FAIL  Fail
 * @return SL_STATUS_OK  Success
 ******************************************************************************/
sl_status_t  cap1166_led_config(cap1166_handle_t *cap1166_handle,
                                led_cfg_t *cap1166_led_cfg);

/***************************************************************************//**
 * @brief
 *   This function sets LED output state in direct mode.
 *
 * @param[in] cap1166_handle
 *   The cap1166_handle_t instance to use.
 * @param[in] led_index
 *   In range [0-5].
 * @param[in] state
 *   State to set at led_index output.
 *    - <b>CAP1166_LED_TURN_ON (0x01):</b> LED on
 *    - <b>CAP1166_LED_TURN_OFF (0x00):</b> LED off
 *
 * @return SL_STATUS_NULL_POINTER  Pointer to NULL
 * @return SL_STATUS_INVALID_PARAMETER  Invalid led_index
 * @return SL_STATUS_FAIL  Fail
 * @return SL_STATUS_OK  Success
 ******************************************************************************/
sl_status_t  cap1166_led_direct_set(cap1166_handle_t *cap1166_handle,
                                    uint8_t led_index,
                                    uint8_t state);

/** @} (end addtogroup cap1166_details) */

#ifdef __cplusplus
}
#endif

#endif /* __SL_CAP1166_H__ */
