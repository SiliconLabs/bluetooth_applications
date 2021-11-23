/***************************************************************************//**
* @file cap1166_regs.h
* @brief register define header
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
#ifndef __SL_CAP1166_REGS_H__
#define __SL_CAP1166_REGS_H__

/***************************************************************************//**
 * @name Registers
 * @brief Describe details about registers in touch sensor
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*
 * Registers for Cap1166
 */
#define CAP1166_MAIN_CONTROL_REG                    0x00 /**< Main Control                                        */
#define CAP1166_GEN_STATUS_REG                      0x02 /**< General Status                                      */
#define CAP1166_SENS_IN_STATUS_REG                  0x03 /**< Sensor Input Status                                 */
#define CAP1166_NOISE_FLAG_REG                      0x04 /**< LED Status                                          */
#define CAP1166_REG_NOISE_FLAG_STATUS               0x0A /**< LED Status                                          */
#define CAP1166_SENS_IN1_DELTA_CNT_REG              0x10 /**< Sensor Input 1 Delta Count                          */
#define CAP1166_SENS_IN2_DELTA_CNT_REG              0x11 /**< Sensor Input 2 Delta Count                          */
#define CAP1166_SENS_IN3_DELTA_CNT_REG              0x12 /**< Sensor Input 3 Delta Count                          */
#define CAP1166_SENS_IN4_DELTA_CNT_REG              0x13 /**< Sensor Input 4 Delta Count                          */
#define CAP1166_SENS_IN5_DELTA_CNT_REG              0x14 /**< Sensor Input 5 Delta Count                          */
#define CAP1166_SENS_IN6_DELTA_CNT_REG              0x15 /**< Sensor Input 6 Delta Count                          */
#define CAP1166_SENSITIVITY_CON_REG                 0x1F /**< Sensitivity Control                                 */
#define CAP1166_CONFIG_REG                          0x20 /**< Configuration                                       */
#define CAP1166_SENS_IN_EN_REG                      0x21 /**< Sensor Input Enable                                 */
#define CAP1166_SENS_IN_CONFIG_REG                  0x22 /**< Sensor Input Configuration                          */
#define CAP1166_SENS_IN_CONFIG2_REG                 0x23 /**< Sensor Input Configuration 2                        */
#define CAP1166_AVRG_AND_SAMPL_CONFIG_REG           0x24 /**< Averaging and Sampling Configuration                */
#define CAP1166_CALIB_ACTIVATE_REG                  0x26 /**< Calibration Activate                                */
#define CAP1166_INTERR_EN_REG                       0x27 /**< Interrupt Enable                                    */
#define CAP1166_REPEAT_RATE_EN_REG                  0x28 /**< Repeat Rate Enable                                  */
#define CAP1166_MULTIPLE_TOUCH_CONFIG_REG           0x2A /**< Multiple Touch Configuration                        */
#define CAP1166_MULTIPLE_TOUCH_PATTERN_CONFIG_REG   0x2B /**< Multiple Touch Pattern Configuration                */
#define CAP1166_MULTIPLE_TOUCH_PATTERN_REG          0x2D /**< Multiple Touch Pattern                              */
#define CAP1166_RECALIB_CONFIG_REG                  0x2F /**< Re-calibration Configuration                        */
#define CAP1166_SENS_IN1_THRESHOLD_REG              0x30 /**< Sensor Input 1 Threshold                            */
#define CAP1166_SENS_IN2_THRESHOLD_REG              0x31 /**< Sensor Input 2 Threshold                            */
#define CAP1166_SENS_IN3_THRESHOLD_REG              0x32 /**< Sensor Input 3 Threshold                            */
#define CAP1166_SENS_IN4_THRESHOLD_REG              0x33 /**< Sensor Input 4 Threshold                            */
#define CAP1166_SENS_IN5_THRESHOLD_REG              0x34 /**< Sensor Input 5 Threshold                            */
#define CAP1166_SENS_IN6_THRESHOLD_REG              0x35 /**< Sensor Input 6 Threshold                            */
#define CAP1166_SENS_IN_NOISE_THRESHOLD_REG         0x38 /**< Sensor Input Noise Threshold                        */

/*
 * Standby Configuration Registers
 */
#define CAP1166_STANDBY_CHANN_REG                   0x40 /**< Standby Channel                                     */
#define CAP1166_STANDBY_CONFIG_REG                  0x41 /**< Standby Configuration                               */
#define CAP1166_STANDBY_SENSITIVITY_REG             0x42 /**< Standby Sensitivity                                 */
#define CAP1166_STANDBY_THRESHOLD_REG               0x43 /**< Standby Threshold                                   */
#define CAP1166_CONFIG2_REG                         0x44 /**< Configuration 2                                     */

/*
 * Base Count Register
 */
#define CAP1166_SENS_IN1_BASE_CNT_REG               0x50 /**< Sensor Input 1 Base Count                           */
#define CAP1166_SENS_IN2_BASE_CNT_REG               0x51 /**< Sensor Input 2 Base Count                           */
#define CAP1166_SENS_IN3_BASE_CNT_REG               0x52 /**< Sensor Input 3 Base Count                           */
#define CAP1166_SENS_IN4_BASE_CNT_REG               0x53 /**< Sensor Input 4 Base Count                           */
#define CAP1166_SENS_IN5_BASE_CNT_REG               0x54 /**< Sensor Input 5 Base Count                           */
#define CAP1166_SENS_IN6_BASE_CNT_REG               0x55 /**< Sensor Input 6 Base Count                           */

/*
 * LED Controls
 */
#define CAP1166_LED_OUTPUT_TYPE_REG                 0x71 /**< LED Output Type                                     */
#define CAP1166_SENS_IN_LED_LINK_REG                0x72 /**< Sensor Input LED Linking                            */
#define CAP1166_LED_POLARITY_REG                    0x73 /**< LED Polarity                                        */
#define CAP1166_LED_OUT_CON_REG                     0x74 /**< LED Output Control                                  */
#define CAP1166_LED_LINK_TRANS_CON_REG              0x77 /**< LED Linked Transition Control                       */
#define CAP1166_LED_MIRROR_CON_REG                  0x79 /**< LED Mirror Control                                  */
#define CAP1166_LED_BEHAVIOR1_REG                   0x81 /**< LED Behavior 1                                      */
#define CAP1166_LED_BEHAVIOR2_REG                   0x82 /**< LED Behavior 2                                      */
 
#define CAP1166_LED_PULSE1_PERIOD_REG               0x84 /**< LED Pulse 1 Period                                  */
#define CAP1166_LED_PULSE2_PERIOD_REG               0x85 /**< LED Pulse 2 Period                                  */
#define CAP1166_LED_BREATHE_PERIOD_REG              0x86 /**< LED Breathe Period                                  */
#define CAP1166_LED_CONFIG_REG                      0x88 /**< LED Configuration                                   */
#define CAP1166_LED_PULSE1_DUTY_REG                 0x90 /**< LED Pulse 1 Duty Cycle                              */
#define CAP1166_LED_PULSE2_DUTY_REG                 0x91 /**< LED Pulse 2 Duty Cycle                              */
#define CAP1166_LED_BREATHE_DUTY_REG                0x92 /**< LED Breathe Duty Cycle                              */
#define CAP1166_LED_DIRECT_DUTY_REG                 0x93 /**< LED Direct Duty Cycle                               */
#define CAP1166_LED_DIRECT_RAMP_RATES_REG           0x94 /**< LED Direct Ramp Rates                               */
#define CAP1166_LED_OFF_DELAY_REG                   0x95 /**< LED Off Delay                                       */
#define CAP1166_SENS_IN1_CALIB_REG                  0xB1 /**< Sensor Input 1 Calibration                          */
#define CAP1166_SENS_IN2_CALIB_REG                  0xB2 /**< Sensor Input 2 Calibration                          */
#define CAP1166_SENS_IN3_CALIB_REG                  0xB3 /**< Sensor Input 3 Calibration                          */
#define CAP1166_SENS_IN4_CALIB_REG                  0xB4 /**< Sensor Input 4 Calibration                          */
#define CAP1166_SENS_IN5_CALIB_REG                  0xB5 /**< Sensor Input 5 Calibration                          */
#define CAP1166_SENS_IN6_CALIB_REG                  0xB6 /**< Sensor Input 6 Calibration                          */

#define CAP1166_SENS_IN_CALIB_LSB1_REG              0xB9 /**< Sensor Input Calibration LSB 1                      */
#define CAP1166_SENS_IN_CALIB_LSB2_REG              0xBA /**< Sensor Input Calibration LSB 2                      */
#define CAP1166_PRODUCT_ID_REG                      0xFD /**< Product ID                                          */
#define CAP1166_MANUFACT_ID_REG                     0xFE /**< Manufacturer ID                                     */
#define CAP1166_REVISION_REG                        0xFF /**< Revision                                            */

/*
 * Registers
 */
#define CAP1166_CHIP_ID                 0x51  /**< CHIP ID VALUE */

/*
 * Status sensor inputs
 */
#define CAP1166_SENSOR_INPUT_ENABLE           0x01
#define CAP1166_SENSOR_INPUT_DISABLE          0x00

#define CAP1166_SENSOR_INTERUPT_ENABLE        0x01
#define CAP1166_SENSOR_INTERUPT_DISABLE       0x00

#define CAP1166_SENSOR_REPEAT_ENABLE          0x01
#define CAP1166_SENSOR_REPEAT_DISABLE         0x00

#define CAP1166_SENSOR_RECALIB_ENABLE         0x01
#define CAP1166_SENSOR_RECALIB_DISABLE        0x00

#define CAP1166_SENSOR_PATTERN_ENABLE         0x01
#define CAP1166_SENSOR_PATTERN_DISABLE        0x00

/*
 * Main Control
 */
#define CAP1166_SET_SLEEP_MODE        0x10
#define CAP1166_SET_STANDBY_MODE      0x20
#define CAP1166_INT_MASK              0x01

/*
 * Sensitivity Control
 */
#define CAP1166_SENSITIVITY_MULTIPLIER_128X 0x00
#define CAP1166_SENSITIVITY_MULTIPLIER_64X  0x10
#define CAP1166_SENSITIVITY_MULTIPLIER_32X  0x20
#define CAP1166_SENSITIVITY_MULTIPLIER_16X  0x30
#define CAP1166_SENSITIVITY_MULTIPLIER_8X   0x40
#define CAP1166_SENSITIVITY_MULTIPLIER_4X   0x50
#define CAP1166_SENSITIVITY_MULTIPLIER_2X   0x60
#define CAP1166_SENSITIVITY_MULTIPLIER_1X   0x70
#define CAP1166_DATA_SCALING_FACTOR_1X      0x00
#define CAP1166_DATA_SCALING_FACTOR_2X      0x01
#define CAP1166_DATA_SCALING_FACTOR_4X      0x02
#define CAP1166_DATA_SCALING_FACTOR_8X      0x03
#define CAP1166_DATA_SCALING_FACTOR_16X     0x04
#define CAP1166_DATA_SCALING_FACTOR_32X     0x05
#define CAP1166_DATA_SCALING_FACTOR_64X     0x06
#define CAP1166_DATA_SCALING_FACTOR_128X    0x07
#define CAP1166_DATA_SCALING_FACTOR_256X    0x08

/*
 * Configuration
 */
#define CAP1166_TIMEOUT_DIS             0x00
#define CAP1166_TIMEOUT_EN              0x80
#define CAP1166_TIMEOUT                 (0x01 << 7)

#define CAP1166_WAKE_PIN_NOT_ASSERTED   0x00
#define CAP1166_WAKE_PIN_ASSERTED       0x40
#define CAP1166_WAKE_CFG_MASK           (0x01 << 6)

#define CAP1166_DIG_NOISE_THRESHOLD_EN  0x00
#define CAP1166_DIG_NOISE_THRESHOLD_DIS 0x20
#define CAP1166_DIS_DIG_NOISE_MASK      (0x01 << 5)

#define CAP1166_AN_NOISE_FILTER_EN      0x00
#define CAP1166_AN_NOISE_FILTER_DIS     0x10
#define CAP1166_DIS_ANA_NOISE_MASK      (0x01 << 4)

#define CAP1166_MAX_DUR_RECALIB_DIS     0x00
#define CAP1166_MAX_DUR_RECALIB_EN      0x08
#define CAP1166_MAX_DUR_EN_MASK         (0x01 << 3)

/*
 * Configuration 2
 */
#define CAP1166_NOT_INVERT_TOUCH_SIGNAL        0x00
#define CAP1166_INVERT_TOUCH_SIGNAL            0x80
#define CAP1166_INV_LINK_TRAN_MASK             (0x01 << 7)

#define CAP1166_ALERT_ACTIVE_HIGH              0x00
#define CAP1166_ALERT_ACTIVE_LOW               0x40
#define CAP1166_ALT_POL_MASK                   (0x01 << 6)

#define CAP1166_POWER_CONSUMPTION_REDUCE_EN    0x00
#define CAP1166_POWER_CONSUMPTION_REDUCE_DIS   0x20
#define CAP1166_BLK_PWR_CTRL_MASK              (0x01 << 5)

#define CAP1166_AUTOSET_LED_MIRROR_CON         0x00
#define CAP1166_NOT_AUTOSET_LED_MIRROR_CON     0x10
#define CAP1166_BLK_PWR_MIR_MASK               (0x01 << 4)

#define CAP1166_SHOW_LOW_FREQ_NOISE            0x00
#define CAP1166_NOT_SHOW_LOW_FREQ_NOISE        0x08
#define CAP1166_SHOW_RF_NOISE_MASK             (0x01 << 3)

#define CAP1166_RF_NOISE_FILTER_EN             0x00
#define CAP1166_RF_NOISE_FILTER_DIS            0x04
#define CAP1166_RF_NOISE_FILTER_MASK           (0x01 << 2)

#define CAP1166_AN_CALIB_FAIL_INTERR_DIS       0x00
#define CAP1166_AN_CALIB_FAIL_INTERR_EN        0x02

#define CAP1166_DETECT_RELEASE_EN              0x00
#define CAP1166_DETECT_RELEASE_DIS             0x01
#define CAP1166_INT_REL_N_MASK                 0x01

/*
 * Sensor Input Configuration
 */
#define CAP1166_560MILISEC_BEFORE_RECALIB     0x00
#define CAP1166_840MILISEC_BEFORE_RECALIB     0x10
#define CAP1166_1120MILISEC_BEFORE_RECALIB    0x20
#define CAP1166_1400MILISEC_BEFORE_RECALIB    0x30
#define CAP1166_1680MILISEC_BEFORE_RECALIB    0x40
#define CAP1166_2240MILISEC_BEFORE_RECALIB    0x50
#define CAP1166_2800MILISEC_BEFORE_RECALIB    0x60
#define CAP1166_3360MILISEC_BEFORE_RECALIB    0x70
#define CAP1166_3920MILISEC_BEFORE_RECALIB    0x80
#define CAP1166_4480MILISEC_BEFORE_RECALIB    0x90
#define CAP1166_5600MILISEC_BEFORE_RECALIB    0xA0
#define CAP1166_6720MILISEC_BEFORE_RECALIB    0xB0
#define CAP1166_7840MILISEC_BEFORE_RECALIB    0xC0
#define CAP1166_8906MILISEC_BEFORE_RECALIB    0xD0
#define CAP1166_10080MILISEC_BEFORE_RECALIB   0xE0
#define CAP1166_11200MILISEC_BEFORE_RECALIB   0xF0
#define CAP1166_MAX_DUR_CALIB_MASK            (0x0F << 4)

#define CAP1166_INTERR_REPEAT_RATE_35MILISEC    0x00
#define CAP1166_INTERR_REPEAT_RATE_70MILISEC    0x01
#define CAP1166_INTERR_REPEAT_RATE_105MILISEC   0x02
#define CAP1166_INTERR_REPEAT_RATE_140MILISEC   0x03
#define CAP1166_INTERR_REPEAT_RATE_175MILISEC   0x04
#define CAP1166_INTERR_REPEAT_RATE_210MILISEC   0x05
#define CAP1166_INTERR_REPEAT_RATE_245MILISEC   0x06
#define CAP1166_INTERR_REPEAT_RATE_280MILISEC   0x07
#define CAP1166_INTERR_REPEAT_RATE_315MILISEC   0x08
#define CAP1166_INTERR_REPEAT_RATE_350MILISEC   0x09
#define CAP1166_INTERR_REPEAT_RATE_385MILISEC   0x0A
#define CAP1166_INTERR_REPEAT_RATE_420MILISEC   0x0B
#define CAP1166_INTERR_REPEAT_RATE_455MILISEC   0x0C
#define CAP1166_INTERR_REPEAT_RATE_490MILISEC   0x0D
#define CAP1166_INTERR_REPEAT_RATE_525MILISEC   0x0E
#define CAP1166_INTERR_REPEAT_RATE_560MILISEC   0x0F
#define CAP1166_RPT_RATE_MASK                   0x0F

/*
 * Hold Events
 */
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_35MILISEC   0x00
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_70MILISEC   0x01
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_105MILISEC  0x02
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_140MILISEC  0x03
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_175MILISEC  0x04
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_210MILISEC  0x05
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_245MILISEC  0x06
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_280MILISEC  0x07
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_315MILISEC  0x08
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_350MILISEC  0x09
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_385MILISEC  0x0A
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_420MILISEC  0x0B
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_455MILISEC  0x0C
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_490MILISEC  0x0D
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_525MILISEC  0x0E
#define CAP1166_PRESS_AND_HOLD_EVENT_AFTER_560MILISEC  0x0F

/*
 * Averaging/Sampling Configure
 */
#define CAP1166_1_SAMPLE     0x00
#define CAP1166_2_SAMPLES    0x10
#define CAP1166_4_SAMPLES    0x20
#define CAP1166_8_SAMPLES    0x30
#define CAP1166_16_SAMPLES   0x40
#define CAP1166_32_SAMPLES   0x50
#define CAP1166_64_SAMPLES   0x60
#define CAP1166_128_SAMPLES  0x70
#define CAP1166_AVG_MASK     (0x07 << 4)

#define CAP1166_SAMPLE_TIME_320MICROSEC   0x00
#define CAP1166_SAMPLE_TIME_640MICROSEC   0x04
#define CAP1166_SAMPLE_TIME_1280MICROSEC  0x08
#define CAP1166_SAMPLE_TIME_2560MICROSEC  0x0C
#define CAP1166_SAMP_TIME_MASK            (0x03 << 2)

#define CAP1166_CYCLE_TIME_35MILISEC   0x00
#define CAP1166_CYCLE_TIME_70MILISEC   0x01
#define CAP1166_CYCLE_TIME_105MILISEC  0x02
#define CAP1166_CYCLE_TIME_140MILISEC  0x03
#define CAP1166_CYCLE_TIME_MASK        0x03

/*
 * Input calibration enable
 */
#define CAP1166_ALL_INPUTS_CALIB_DIS  0x00
#define CAP1166_INPUT1_CALIB_EN       0x01
#define CAP1166_INPUT2_CALIB_EN       0x02
#define CAP1166_INPUT3_CALIB_EN       0x04
#define CAP1166_INPUT4_CALIB_EN       0x08
#define CAP1166_INPUT5_CALIB_EN       0x10
#define CAP1166_INPUT6_CALIB_EN       0x20

/*
 * Input error enable
 */
#define CAP1166_ALL_INPUTS_INTERR_DIS   0x00
#define CAP1166_INPUT1_INTERR_EN        0x01
#define CAP1166_INPUT2_INTERR_EN        0x02
#define CAP1166_INPUT3_INTERR_EN        0x04
#define CAP1166_INPUT4_INTERR_EN        0x08
#define CAP1166_INPUT5_INTERR_EN        0x10
#define CAP1166_INPUT6_INTERR_EN        0x20

/*
 * Repeat Rate
 */
#define CAP1166_REPEAT_RATE_DIS        0x00
#define CAP1166_INPUT1_REPEAT_RATE_EN  0x01
#define CAP1166_INPUT2_REPEAT_RATE_EN  0x02
#define CAP1166_INPUT3_REPEAT_RATE_EN  0x04
#define CAP1166_INPUT4_REPEAT_RATE_EN  0x08
#define CAP1166_INPUT5_REPEAT_RATE_EN  0x10
#define CAP1166_INPUT6_REPEAT_RATE_EN  0x20

/*
 * Multiple Touch Configure
 */
#define CAP1166_MULTIPLE_BLOCK_DIS        0x00
#define CAP1166_MULTIPLE_BLOCK_EN         0x80
#define CAP1166_MULTIPLE_BLOCK_MASK       (0x01 << 7)

#define CAP1166_SIMUL_TOUCH_NUM_1         0x00
#define CAP1166_SIMUL_TOUCH_NUM_2         0x04
#define CAP1166_SIMUL_TOUCH_NUM_3         0x08
#define CAP1166_SIMUL_TOUCH_NUM_4         0x0C
#define CAP1166_SIMUL_TOUCH_NUM_MASK      (0x03 << 2)

/*
 * Multiple Touch Pattern Configure
 */
#define CAP1166_MULTIPLE_THRESHOLD_12_PERCENTS   0x00
#define CAP1166_MULTIPLE_THRESHOLD_25_PERCENTS   0x04
#define CAP1166_MULTIPLE_THRESHOLD_37_PERCENTS   0x08
#define CAP1166_MULTIPLE_THRESHOLD_100_PERCENTS  0x0C

#define CAP1166_MODE_MLTP_TOUCH_PATTERN          0x00
#define CAP1166_MODE_PATTERN_RECOGNITION         0x02

#define CAP1166_MLTP_EVENT_ALERT_DIS             0x00
#define CAP1166_MLTP_EVENT_ALERT_EN              0x01

#define CAP1166_MLTP_TOUCH_PATTERN_EN            0x80

/*
 * Multiple Touch Pattern
 */
#define CAP1166_MLTP_TOUCH_PATTERN_INPUTS_DIS     0x00
#define CAP1166_MLTP_TOUCH_PATTERN_INPUT1_EN      0x01
#define CAP1166_MLTP_TOUCH_PATTERN_INPUT2_EN      0x02
#define CAP1166_MLTP_TOUCH_PATTERN_INPUT3_EN      0x04
#define CAP1166_MLTP_TOUCH_PATTERN_INPUT4_EN      0x08
#define CAP1166_MLTP_TOUCH_PATTERN_INPUT5_EN      0x10
#define CAP1166_MLTP_TOUCH_PATTERN_INPUT6_EN      0x20
#define CAP1166_MLTP_TOUCH_PATTERN_ALL_INPUTS_EN  0x3F

/*
 * Sensor Input Threshold
 */
#define CAP1166_SENS_IN_THRESHOLD_1X              0x01
#define CAP1166_SENS_IN_THRESHOLD_2X              0x02
#define CAP1166_SENS_IN_THRESHOLD_4X              0x04
#define CAP1166_SENS_IN_THRESHOLD_8X              0x08
#define CAP1166_SENS_IN_THRESHOLD_16X             0x10
#define CAP1166_SENS_IN_THRESHOLD_32X             0x20
#define CAP1166_SENS_IN_THRESHOLD_64X             0x40

/*
 * Sensor Input noise Threshold
 */
#define CAP1166_SENS_IN_THRESHOLD_25_PERCENT        0x00
#define CAP1166_SENS_IN_THRESHOLD_37_5_PERCENT      0x01
#define CAP1166_SENS_IN_THRESHOLD_50_PERCENT        0x02
#define CAP1166_SENS_IN_THRESHOLD_62_5_PERCENT      0x03

/*
 * Re-calibration Configuration
 */
#define CAP1166_INPUTS_THRESHOLD_UPDT_INDIVIDUALY  0x00
#define CAP1166_INPUT1_THRESHOLD_OVERWRITE         0x80
#define CAP1166_BUT_LD_TH_MASK                     (0x01 << 7)

#define CAP1166_ACC_INTER_DATA_CLEAR               0x00
#define CAP1166_ACC_INTER_DATA_NOT_CLEAR           0x40
#define CAP1166_NO_CLR_INTD_MASK                   (0x01 << 6)

#define CAP1166_CONS_NEG_DELTA_CNT_CLEAR           0x00
#define CAP1166_CONS_NEG_DELTA_CNT_NOT_CLEAR       0x20
#define CAP1166_NO_CLR_NEG_MASK                    (0x01 << 5)

#define CAP1166_CONS_NEG_DELTA_CNT_8               0x00
#define CAP1166_CONS_NEG_DELTA_CNT_16              0x08
#define CAP1166_CONS_NEG_DELTA_CNT_32              0x10
#define CAP1166_CONS_NEG_DELTA_CNT_DIS             0x18
#define CAP1166_NEG_DELTA_CNT_MASK                 (0x03 << 3)

#define CAP1166_REC_SAMPLES_16_UPDT_TIME_16        0x00
#define CAP1166_REC_SAMPLES_32_UPDT_TIME_32        0x01
#define CAP1166_REC_SAMPLES_64_UPDT_TIME_64        0x02
#define CAP1166_REC_SAMPLES_128_UPDT_TIME_128      0x03
#define CAP1166_REC_SAMPLES_256_UPDT_TIME_256      0x04
#define CAP1166_REC_SAMPLES_256_UPDT_TIME_1024     0x05
#define CAP1166_REC_SAMPLES_256_UPDT_TIME_2048     0x06
#define CAP1166_REC_SAMPLES_256_UPDT_TIME_4096     0x07
#define CAP1166_CAL_CFG_RE_CALIB_MASK              0x07

/*
 * Standby configuration
 */
#define CAP1166_AVERAGE_BASED    0x00
#define CAP1166_SUMMATION_BASED  0x80
#define CAP1166_AVG_SUM_MASK     (0x01 << 7)

#define CAP1166_STBY_AVERAGE_1            0x00
#define CAP1166_STBY_AVERAGE_2            0x10
#define CAP1166_STBY_AVERAGE_4            0x20
#define CAP1166_STBY_AVERAGE_8            0x30
#define CAP1166_STBY_AVERAGE_16           0x40
#define CAP1166_STBY_AVERAGE_32           0x50
#define CAP1166_STBY_AVERAGE_64           0x60
#define CAP1166_STBY_AVERAGE_128          0x70
#define CAP1166_STBY_AVG_MASK             (0x07 << 4)

#define CAP1166_STBY_SAMPLE_TIME_320US    0x00
#define CAP1166_STBY_SAMPLE_TIME_640US    0x04
#define CAP1166_STBY_SAMPLE_TIME_1280US   0x08
#define CAP1166_STBY_SAMPLE_TIME_2560US   0x0C
#define CAP1166_STBY_SAMP_TIM_MASK        (0x03 << 2)

#define CAP1166_STBY_CYCLE_TIME_35MS      0x00
#define CAP1166_STBY_CYCLE_TIME_70MS      0x01
#define CAP1166_STBY_CYCLE_TIME_105MS     0x02
#define CAP1166_STBY_CYCLE_TIME_140MS     0x03
#define CAP1166_STBY_CY_TIME_MASK         0x03

/*
 * Standby Sensitivity
 */
#define CAP1166_STBY_SENSE_MULTIPLIER_128X  0x00
#define CAP1166_STBY_SENSE_MULTIPLIER_64X   0x01
#define CAP1166_STBY_SENSE_MULTIPLIER_32X   0x02
#define CAP1166_STBY_SENSE_MULTIPLIER_16X   0x03
#define CAP1166_STBY_SENSE_MULTIPLIER_8X    0x04
#define CAP1166_STBY_SENSE_MULTIPLIER_4X    0x05
#define CAP1166_STBY_SENSE_MULTIPLIER_2X    0x06
#define CAP1166_STBY_SENSE_MULTIPLIER_1X    0x07

/*
 * Standby Sensitivity
 */
#define CAP1166_STBY_THRESHOLD_64X   0x40
#define CAP1166_STBY_THRESHOLD_32X   0x20
#define CAP1166_STBY_THRESHOLD_16X   0x10
#define CAP1166_STBY_THRESHOLD_8X    0x08
#define CAP1166_STBY_THRESHOLD_4X    0x04
#define CAP1166_STBY_THRESHOLD_2X    0x02
#define CAP1166_STBY_THRESHOLD_1X    0x01

/*
 * LED Output Type
 */
#define CAP1166_LED6_PIN_OPEN_DRAIN  0x00
#define CAP1166_LED6_PIN_PUSH_PULL   0x20
#define CAP1166_LED5_PIN_OPEN_DRAIN  0x00
#define CAP1166_LED5_PIN_PUSH_PULL   0x10
#define CAP1166_LED4_PIN_OPEN_DRAIN  0x00
#define CAP1166_LED4_PIN_PUSH_PULL   0x08
#define CAP1166_LED3_PIN_OPEN_DRAIN  0x00
#define CAP1166_LED3_PIN_PUSH_PULL   0x04
#define CAP1166_LED2_PIN_OPEN_DRAIN  0x00
#define CAP1166_LED2_PIN_PUSH_PULL   0x02
#define CAP1166_LED1_PIN_OPEN_DRAIN  0x00
#define CAP1166_LED1_PIN_PUSH_PULL   0x01

/*
 * Input LED Linking
 */
#define CAP1166_LED6_IN6_NOT_LINKED  0x00
#define CAP1166_LED6_IN6_LINKED      0x20
#define CAP1166_LED5_IN5_NOT_LINKED  0x00
#define CAP1166_LED5_IN5_LINKED      0x10
#define CAP1166_LED4_IN4_NOT_LINKED  0x00
#define CAP1166_LED4_IN4_LINKED      0x08
#define CAP1166_LED3_IN3_NOT_LINKED  0x00
#define CAP1166_LED3_IN3_LINKED      0x04
#define CAP1166_LED2_IN2_NOT_LINKED  0x00
#define CAP1166_LED2_IN2_LINKED      0x02
#define CAP1166_LED1_IN1_NOT_LINKED  0x00
#define CAP1166_LED1_IN1_LINKED      0x01

/*
 * LED Output Control
 */
#define CAP1166_LED6_OUT_INVERTED      0x00
#define CAP1166_LED6_OUT_NOT_INVERTED  0x20
#define CAP1166_LED5_OUT_INVERTED      0x00
#define CAP1166_LED5_OUT_NOT_INVERTED  0x10
#define CAP1166_LED4_OUT_INVERTED      0x00
#define CAP1166_LED4_OUT_NOT_INVERTED  0x08
#define CAP1166_LED3_OUT_INVERTED      0x00
#define CAP1166_LED3_OUT_NOT_INVERTED  0x04
#define CAP1166_LED2_OUT_INVERTED      0x00
#define CAP1166_LED2_OUT_NOT_INVERTED  0x02
#define CAP1166_LED1_OUT_INVERTED      0x00
#define CAP1166_LED1_OUT_NOT_INVERTED  0x01

/*
 * LED driven to Min/Max duty
 */
#define CAP1166_LED6_DRIVEN_TO_MIN_DUTY  0x00
#define CAP1166_LED6_DRIVEN_TO_MAX_DUTY  0x20
#define CAP1166_LED5_DRIVEN_TO_MIN_DUTY  0x00
#define CAP1166_LED5_DRIVEN_TO_MAX_DUTY  0x10
#define CAP1166_LED4_DRIVEN_TO_MIN_DUTY  0x00
#define CAP1166_LED4_DRIVEN_TO_MAX_DUTY  0x08
#define CAP1166_LED3_DRIVEN_TO_MIN_DUTY  0x00
#define CAP1166_LED3_DRIVEN_TO_MAX_DUTY  0x04
#define CAP1166_LED2_DRIVEN_TO_MIN_DUTY  0x00
#define CAP1166_LED2_DRIVEN_TO_MAX_DUTY  0x02
#define CAP1166_LED1_DRIVEN_TO_MIN_DUTY  0x00
#define CAP1166_LED1_DRIVEN_TO_MAX_DUTY  0x01

/*
 * LED change state
 */
#define CAP1166_LED6_CHANGE_STATE      0x00
#define CAP1166_LED6_NOT_CHANGE_STATE  0x20
#define CAP1166_LED5_CHANGE_STATE      0x00
#define CAP1166_LED5_NOT_CHANGE_STATE  0x10
#define CAP1166_LED4_CHANGE_STATE      0x00
#define CAP1166_LED4_NOT_CHANGE_STATE  0x08
#define CAP1166_LED3_CHANGE_STATE      0x00
#define CAP1166_LED3_NOT_CHANGE_STATE  0x04
#define CAP1166_LED2_CHANGE_STATE      0x00
#define CAP1166_LED2_NOT_CHANGE_STATE  0x02
#define CAP1166_LED1_CHANGE_STATE      0x00
#define CAP1166_LED1_NOT_CHANGE_STATE  0x01

/*
 * LED behavior LED1/2/3/4
 */
#define CAP1166_LED1_DIRECT_BEHAVIOR   0x00
#define CAP1166_LED1_PULSE1_BEHAVIOR   0x01
#define CAP1166_LED1_PULSE2_BEHAVIOR   0x02
#define CAP1166_LED1_BREATHE_BEHAVIOR  0x03
#define CAP1166_LED2_DIRECT_BEHAVIOR   0x00
#define CAP1166_LED2_PULSE1_BEHAVIOR   ( 0x01 << 2 )
#define CAP1166_LED2_PULSE2_BEHAVIOR   ( 0x02 << 2 )
#define CAP1166_LED2_BREATHE_BEHAVIOR  ( 0x03 << 2 )
#define CAP1166_LED3_DIRECT_BEHAVIOR   0x00
#define CAP1166_LED3_PULSE1_BEHAVIOR   ( 0x01 << 4 )
#define CAP1166_LED3_PULSE2_BEHAVIOR   ( 0x02 << 4 )
#define CAP1166_LED3_BREATHE_BEHAVIOR  ( 0x03 << 4 )
#define CAP1166_LED4_DIRECT_BEHAVIOR   ( 0x00 )
#define CAP1166_LED4_PULSE1_BEHAVIOR   ( 0x01 << 6 )
#define CAP1166_LED4_PULSE2_BEHAVIOR   ( 0x02 << 6 )
#define CAP1166_LED4_BREATHE_BEHAVIOR  ( 0x03 << 6 )

/*
 * LED behavior LED5/6
 */
#define CAP1166_LED5_DIRECT_BEHAVIOR   0x00
#define CAP1166_LED5_PULSE1_BEHAVIOR   0x01
#define CAP1166_LED5_PULSE2_BEHAVIOR   0x02
#define CAP1166_LED5_BREATHE_BEHAVIOR  0x03
#define CAP1166_LED6_DIRECT_BEHAVIOR   0x00
#define CAP1166_LED6_PULSE1_BEHAVIOR   ( 0x01 << 2 )
#define CAP1166_LED6_PULSE2_BEHAVIOR   ( 0x02 << 2 )
#define CAP1166_LED6_BREATHE_BEHAVIOR  ( 0x03 << 2 )

/*
 * LED pulse
 */
#define CAP1166_LED_PULSE_ON_TOUCH            0x00
#define CAP1166_LED_PULSE_ON_RELEASE          0x80
#define CAP1166_LED_PULSE_PERIOD_32MILISEC    0x00
#define CAP1166_LED_PULSE_PERIOD_4064MILISEC  0x7F

/*
 * LED state
 */
#define CAP1166_LED_TURN_ON           0x01
#define CAP1166_LED_TURN_OFF          0x00

/*
 * LED Configuration
 */
#define CAP1166_ALERT_NOT_ASSERTED_ON_LED_ACT   0x00
#define CAP1166_ALERT_ASSERTED_ON_LED_ACT       0x40
#define CAP1166_PULSE1_1_PULSE                  0x00
#define CAP1166_PULSE1_2_PULSES                 0x01
#define CAP1166_PULSE1_3_PULSES                 0x02
#define CAP1166_PULSE1_4_PULSES                 0x03
#define CAP1166_PULSE1_5_PULSES                 0x04
#define CAP1166_PULSE1_6_PULSES                 0x05
#define CAP1166_PULSE1_7_PULSES                 0x06
#define CAP1166_PULSE1_8_PULSES                 0x07
#define CAP1166_PULSE2_1_PULSE                  0x00
#define CAP1166_PULSE2_2_PULSES               ( 0x01 << 3 )
#define CAP1166_PULSE2_3_PULSES               ( 0x02 << 3 )
#define CAP1166_PULSE2_4_PULSES               ( 0x03 << 3 )
#define CAP1166_PULSE2_5_PULSES               ( 0x04 << 3 )
#define CAP1166_PULSE2_6_PULSES               ( 0x05 << 3 )
#define CAP1166_PULSE2_7_PULSES               ( 0x06 << 3 )
#define CAP1166_PULSE2_8_PULSES               ( 0x07 << 3 )

/*
 * Direct Ramp Rates
 */
#define CAP1166_RISE_RATE_0MS                 ( 0x00 << 3 )
#define CAP1166_RISE_RATE_250MS               ( 0x01 << 3 )
#define CAP1166_RISE_RATE_500MS               ( 0x02 << 3 )
#define CAP1166_RISE_RATE_750MS               ( 0x03 << 3 )
#define CAP1166_RISE_RATE_1000MS              ( 0x04 << 3 )
#define CAP1166_RISE_RATE_1250MS              ( 0x05 << 3 )
#define CAP1166_RISE_RATE_1500MS              ( 0x06 << 3 )
#define CAP1166_RISE_RATE_2000MS              ( 0x07 << 3 )

#define CAP1166_FALL_RATE_0MS                   0x00
#define CAP1166_FALL_RATE_250MS                 0x01
#define CAP1166_FALL_RATE_500MS                 0x02
#define CAP1166_FALL_RATE_750MS                 0x03
#define CAP1166_FALL_RATE_1000MS                0x04
#define CAP1166_FALL_RATE_1250MS                0x05
#define CAP1166_FALL_RATE_1500MS                0x06
#define CAP1166_LALL_RATE_2000MS                0x07

/*
 * Off Delay
 */
#define CAP1166_BR_OFF_DLY_0MS             ( 0x00 << 4 )
#define CAP1166_BR_OFF_DLY_250MS           ( 0x01 << 4 )
#define CAP1166_BR_OFF_DLY_500MS           ( 0x02 << 4 )
#define CAP1166_BR_OFF_DLY_750MS           ( 0x03 << 4 )
#define CAP1166_BR_OFF_DLY_1000MS          ( 0x04 << 4 )
#define CAP1166_BR_OFF_DLY_1250MS          ( 0x05 << 4 )
#define CAP1166_BR_OFF_DLY_1500MS          ( 0x06 << 4 )
#define CAP1166_BR_OFF_DLY_2000MS          ( 0x07 << 4 )

#define CAP1166_DR_OFF_DLY_0MS              0x00
#define CAP1166_DR_OFF_DLY_250MS            0x01
#define CAP1166_DR_OFF_DLY_500MS            0x02
#define CAP1166_DR_OFF_DLY_750MS            0x03
#define CAP1166_DR_OFF_DLY_1000MS           0x04
#define CAP1166_DR_OFF_DLY_1250MS           0x05
#define CAP1166_DR_OFF_DLY_1500MS           0x06
#define CAP1166_DR_OFF_DLY_2000MS           0x07
#define CAP1166_DR_OFF_DLY_2500MS           0x08
#define CAP1166_DR_OFF_DLY_3000MS           0x09
#define CAP1166_DR_OFF_DLY_3500MS           0x0A
#define CAP1166_DR_OFF_DLY_4000MS           0x0B
#define CAP1166_DR_OFF_DLY_4500MS           0x0C
#define CAP1166_DR_OFF_DLY_5000MS           0x0D

/*
 * Duty in percent
 */
#define CAP1166_MIN_DUTY_0_PERCENT    0x00
#define CAP1166_MIN_DUTY_7_PERCENTS   0x01
#define CAP1166_MIN_DUTY_9_PERCENTS   0x02
#define CAP1166_MIN_DUTY_11_PERCENTS  0x03
#define CAP1166_MIN_DUTY_14_PERCENTS  0x04
#define CAP1166_MIN_DUTY_17_PERCENTS  0x05
#define CAP1166_MIN_DUTY_20_PERCENTS  0x06
#define CAP1166_MIN_DUTY_23_PERCENTS  0x07
#define CAP1166_MIN_DUTY_26_PERCENTS  0x08
#define CAP1166_MIN_DUTY_30_PERCENTS  0x09
#define CAP1166_MIN_DUTY_35_PERCENTS  0x0A
#define CAP1166_MIN_DUTY_40_PERCENTS  0x0B
#define CAP1166_MIN_DUTY_46_PERCENTS  0x0C
#define CAP1166_MIN_DUTY_53_PERCENTS  0x0D
#define CAP1166_MIN_DUTY_63_PERCENTS  0x0E
#define CAP1166_MIN_DUTY_77_PERCENTS  0x0F

#define CAP1166_MAX_DUTY_7_PERCENTS   0x00
#define CAP1166_MAX_DUTY_9_PERCENTS   0x10
#define CAP1166_MAX_DUTY_11_PERCENTS  0x20
#define CAP1166_MAX_DUTY_14_PERCENTS  0x30
#define CAP1166_MAX_DUTY_17_PERCENTS  0x40
#define CAP1166_MAX_DUTY_20_PERCENTS  0x50
#define CAP1166_MAX_DUTY_23_PERCENTS  0x60
#define CAP1166_MAX_DUTY_26_PERCENTS  0x70
#define CAP1166_MAX_DUTY_30_PERCENTS  0x80
#define CAP1166_MAX_DUTY_35_PERCENTS  0x90
#define CAP1166_MAX_DUTY_40_PERCENTS  0xA0
#define CAP1166_MAX_DUTY_46_PERCENTS  0xB0
#define CAP1166_MAX_DUTY_53_PERCENTS  0xC0
#define CAP1166_MAX_DUTY_63_PERCENTS  0xD0
#define CAP1166_MAX_DUTY_77_PERCENTS  0xE0
#define CAP1166_MAX_DUTY_100_PERCENTS 0xF0

/*
 * Gain and Input enable
 */
#define CAP1166_GAIN_1     0x00
#define CAP1166_GAIN_2     0x40
#define CAP1166_GAIN_4     0x80
#define CAP1166_GAIN_8     0xC0

#define CAP1166_ALL_INPUTS_DISABLE 0x00
#define CAP1166_ALL_INPUTS_ENABLE  0x3F
#define CAP1166_INPUT1_EN  0x01
#define CAP1166_INPUT2_EN  0x02
#define CAP1166_INPUT3_EN  0x04
#define CAP1166_INPUT4_EN  0x08
#define CAP1166_INPUT5_EN  0x10
#define CAP1166_INPUT6_EN  0x20

/*
 * Interrupt reason
 */
#define CAP1166_RESET_INT_MASK                (1<<3)
#define CAP1166_TOUCH_DETECTED_INT_MASK       (1<<0)
#define CAP1166_MULTI_TOUCH_PATTERN_INT_MASK  (1<<1)

/*
 * Touch detection status
 */
#define CAP1166_BUTTON_PRESSED                0x01
#define CAP1166_BUTTON_RELEASED               0x02
#define CAP1166_BUTTON_NOT_DETECTED           0x00

/** @} end addtogroup Registers*/

#endif /* __SL_CAP1166_REGS_H__ */
