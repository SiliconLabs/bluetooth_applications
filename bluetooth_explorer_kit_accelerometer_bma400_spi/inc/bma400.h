/***************************************************************************//**
 * @file bma400.h
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
#ifndef BMA400_H
#define BMA400_H

#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Macro to SET and GET BITS of a register */
#define BMA400_SET_BITS(reg_data, bitname, data)  \
  ((reg_data & ~(bitname##_MSK)) | ((data << bitname##_POS) & bitname##_MSK))

#define BMA400_GET_BITS(reg_data, bitname)  \
                  ((reg_data & (bitname##_MSK)) >> (bitname##_POS))

#define BMA400_SET_BITS_POS_0(reg_data, bitname, data) \
    ((reg_data & ~(bitname##_MSK)) | (data & bitname##_MSK))

#define BMA400_GET_BITS_POS_0(reg_data, bitname) (reg_data & (bitname##_MSK))

#define BMA400_SET_BIT_VAL_0(reg_data, bitname)  (reg_data & ~(bitname##_MSK))

#define BMA400_GET_LSB(var)   (uint8_t)(var & BMA400_SET_LOW_BYTE)
#define BMA400_GET_MSB(var)   (uint8_t)((var & BMA400_SET_HIGH_BYTE) >> 8)

/* API warning codes */
#define BMA400_W_SELF_TEST_FAIL        1

/* Registers */
#define BMA400_CHIP_ID                 0x90/* CHIP ID VALUE */

/* Enable / Disable macros */
#define BMA400_DISABLE                 0
#define BMA400_ENABLE                  1

/* ODR configurations  */
#define BMA400_ODR_12_5HZ              0x05
#define BMA400_ODR_25HZ                0x06
#define BMA400_ODR_50HZ                0x07
#define BMA400_ODR_100HZ               0x08
#define BMA400_ODR_200HZ               0x09
#define BMA400_ODR_400HZ               0x0A
#define BMA400_ODR_800HZ               0x0B

/* Accel Range configuration */
#define BMA400_RANGE_2G                0x00
#define BMA400_RANGE_4G                0x01
#define BMA400_RANGE_8G                0x02
#define BMA400_RANGE_16G               0x03

/* Accel Axes selection settings for
 * DATA SAMPLING, WAKEUP, ORIENTATION CHANGE,
 * GEN1, GEN2 , ACTIVITY CHANGE
 */
#define BMA400_AXIS_X_EN               0x01
#define BMA400_AXIS_Y_EN               0x02
#define BMA400_AXIS_Z_EN               0x04
#define BMA400_AXIS_XYZ_EN             0x07

/* Accel filter(data_src_reg) selection settings */
#define BMA400_DATA_SRC_ACCEL_FILT_1   0x00
#define BMA400_DATA_SRC_ACCEL_FILT_2   0x01
#define BMA400_DATA_SRC_ACCEL_FILT_LP  0x02

/* Accel OSR (OSR,OSR_LP) settings */
#define BMA400_ACCEL_OSR_SETTING_0     0x00
#define BMA400_ACCEL_OSR_SETTING_1     0x01
#define BMA400_ACCEL_OSR_SETTING_2     0x02
#define BMA400_ACCEL_OSR_SETTING_3     0x03

/* Accel filt1_bw settings */
/* Accel filt1_bw = 0.48 * ODR */
#define BMA400_ACCEL_FILT1_BW_0        0x00

/* Accel filt1_bw = 0.24 * ODR */
#define BMA400_ACCEL_FILT1_BW_1        0x01

/* Auto wake-up timeout value of 10.24s */
#define BMA400_AUTO_WAKEUP_TIMEOUT_MAX 0x0FFF

/* Auto low power timeout value of 10.24s */
#define BMA400_AUTO_LP_TIMEOUT_MAX     0x0FFF

/* Reference Update macros */
#define BMA400_UPDATE_MANUAL           0x00
#define BMA400_UPDATE_ONE_TIME         0x01
#define BMA400_UPDATE_EVERY_TIME       0x02
#define BMA400_UPDATE_LP_EVERY_TIME    0x03

/* Reference Update macros for orient interrupts */
#define BMA400_ORIENT_REFU_ACC_FILT_2  0x01
#define BMA400_ORIENT_REFU_ACC_FILT_LP 0x02

/* Stability evaluation macros for orient interrupts */
#define BMA400_STABILITY_DISABLED      0x00
#define BMA400_STABILITY_ACC_FILT_2    0x01
#define BMA400_STABILITY_ACC_FILT_LP   0x02

/* Number of samples needed for Auto-wakeup interrupt evaluation  */
#define BMA400_SAMPLE_COUNT_1          0x00
#define BMA400_SAMPLE_COUNT_2          0x01
#define BMA400_SAMPLE_COUNT_3          0x02
#define BMA400_SAMPLE_COUNT_4          0x03
#define BMA400_SAMPLE_COUNT_5          0x04
#define BMA400_SAMPLE_COUNT_6          0x05
#define BMA400_SAMPLE_COUNT_7          0x06
#define BMA400_SAMPLE_COUNT_8          0x07

/* Auto low power configurations */
/* Auto low power timeout disabled  */
#define BMA400_AUTO_LP_TIMEOUT_DISABLE 0x00

/* Auto low power entered on drdy interrupt */
#define BMA400_AUTO_LP_DRDY_TRIGGER    0x01

/* Auto low power entered on GEN1 interrupt */
#define BMA400_AUTO_LP_GEN1_TRIGGER    0x02

/* Auto low power entered on timeout of threshold value */
#define BMA400_AUTO_LP_TIMEOUT_EN      0x04

/* Auto low power entered on timeout of threshold value
 * but reset on activity detection
 */
#define BMA400_AUTO_LP_TIME_RESET_EN   0x08

/*    TAP INTERRUPT CONFIG MACROS   */
/* Axes select for TAP interrupt */
#define BMA400_X_AXIS_EN_TAP           0x02
#define BMA400_Y_AXIS_EN_TAP           0x01
#define BMA400_Z_AXIS_EN_TAP           0x00

/* TAP tics_th setting */

/* Maximum time between upper and lower peak of a tap, in data samples
 * this time depends on the mechanics of the device tapped onto
 * default = 12 samples
 */

/* Configures 6 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_6_DATA_SAMPLES  0x00

/* Configures 9 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_9_DATA_SAMPLES  0x01

/* Configures 12 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_12_DATA_SAMPLES 0x02

/* Configures 18 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_18_DATA_SAMPLES 0x03

/* TAP Sensitivity setting */
/* It modifies the threshold for minimum TAP amplitude */
/* BMA400_TAP_SENSITIVITY_0 correspond to highest sensitivity */
#define BMA400_TAP_SENSITIVITY_0       0x00
#define BMA400_TAP_SENSITIVITY_1       0x01
#define BMA400_TAP_SENSITIVITY_2       0x02
#define BMA400_TAP_SENSITIVITY_3       0x03
#define BMA400_TAP_SENSITIVITY_4       0x04
#define BMA400_TAP_SENSITIVITY_5       0x05
#define BMA400_TAP_SENSITIVITY_6       0x06

/* BMA400_TAP_SENSITIVITY_7 correspond to lowest sensitivity */
#define BMA400_TAP_SENSITIVITY_7       0x07

/*  BMA400 TAP - quiet  settings */

/* Quiet refers to minimum quiet time before and after double tap,
 * in the data samples This time also defines the longest time interval
 * between two taps so that they are considered as double tap
 */

/* Configures 60 data samples quiet time between single or double taps */
#define BMA400_QUIET_60_DATA_SAMPLES  0x00

/* Configures 80 data samples quiet time between single or double taps */
#define BMA400_QUIET_80_DATA_SAMPLES  0x01

/* Configures 100 data samples quiet time between single or double taps */
#define BMA400_QUIET_100_DATA_SAMPLES 0x02

/* Configures 120 data samples quiet time between single or double taps */
#define BMA400_QUIET_120_DATA_SAMPLES 0x03

/*  BMA400 TAP - quiet_dt  settings */

/* quiet_dt refers to Minimum time between the two taps of a
 * double tap, in data samples
 */

/* Configures 4 data samples minimum time between double taps */
#define BMA400_QUIET_DT_4_DATA_SAMPLES   0x00

/* Configures 8 data samples minimum time between double taps */
#define BMA400_QUIET_DT_8_DATA_SAMPLES   0x01

/* Configures 12 data samples minimum time between double taps */
#define BMA400_QUIET_DT_12_DATA_SAMPLES  0x02

/* Configures 16 data samples minimum time between double taps */
#define BMA400_QUIET_DT_16_DATA_SAMPLES  0x03

/*    ACTIVITY CHANGE CONFIG MACROS   */
/* Data source for activity change detection */
#define BMA400_DATA_SRC_ACC_FILT1        0x00
#define BMA400_DATA_SRC_ACC_FILT2        0x01

/* Number of samples to evaluate for activity change detection */
#define BMA400_ACT_CH_SAMPLE_CNT_32      0x00
#define BMA400_ACT_CH_SAMPLE_CNT_64      0x01
#define BMA400_ACT_CH_SAMPLE_CNT_128     0x02
#define BMA400_ACT_CH_SAMPLE_CNT_256     0x03
#define BMA400_ACT_CH_SAMPLE_CNT_512     0x04

/* Interrupt pin configuration macros */
#define BMA400_INT_PUSH_PULL_ACTIVE_0    0x00
#define BMA400_INT_PUSH_PULL_ACTIVE_1    0x01
#define BMA400_INT_OPEN_DRIVE_ACTIVE_0   0x02
#define BMA400_INT_OPEN_DRIVE_ACTIVE_1   0x03

/* Interrupt Assertion status macros */
#define BMA400_ASSERTED_WAKEUP_INT       0x0001
#define BMA400_ASSERTED_ORIENT_CH_INT    0x0002
#define BMA400_ASSERTED_GEN1_INT         0x0004
#define BMA400_ASSERTED_GEN2_INT         0x0008
#define BMA400_ASSERTED_INT_OVERRUN      0x0010
#define BMA400_ASSERTED_FIFO_FULL_INT    0x0020
#define BMA400_ASSERTED_FIFO_WM_INT      0x0040
#define BMA400_ASSERTED_DRDY_INT         0x0080
#define BMA400_ASSERTED_STEP_INT         0x0300
#define BMA400_ASSERTED_S_TAP_INT        0x0400
#define BMA400_ASSERTED_D_TAP_INT        0x0800
#define BMA400_ASSERTED_ACT_CH_X         0x2000
#define BMA400_ASSERTED_ACT_CH_Y         0x4000
#define BMA400_ASSERTED_ACT_CH_Z         0x8000

/* Generic interrupt criterion_sel configuration macros */
#define BMA400_ACTIVITY_INT              0x01
#define BMA400_INACTIVITY_INT            0x00

/* Generic interrupt axes evaluation logic configuration macros */
#define BMA400_ALL_AXES_INT              0x01
#define BMA400_ANY_AXES_INT              0x00

/* Generic interrupt hysteresis configuration macros */
#define BMA400_HYST_0_MG                 0x00
#define BMA400_HYST_24_MG                0x01
#define BMA400_HYST_48_MG                0x02
#define BMA400_HYST_96_MG                0x03

/* BMA400 Register Address */
#define BMA400_REG_CHIP_ID               0x00
#define BMA400_REG_ERR_REG               0x02
#define BMA400_REG_STATUS                0x03
#define BMA400_REG_ACC_X_LSB             0x04
#define BMA400_REG_ACC_X_MSB             0x05
#define BMA400_REG_ACC_Y_LSB             0x06
#define BMA400_REG_ACC_Y_MSB             0x07
#define BMA400_REG_ACC_Z_LSB             0x08
#define BMA400_REG_ACC_Z_MSB             0x09
#define BMA400_REG_SENSOR_TIME_0         0x0A
#define BMA400_REG_SENSOR_TIME_1         0x0B
#define BMA400_REG_SENSOR_TIME_2         0x0C
#define BMA400_REG_EVENT                 0x0D
#define BMA400_REG_INT_STATUS_0          0x0E
#define BMA400_REG_INT_STATUS_1          0x0F
#define BMA400_REG_INT_STATUS_2          0x10
#define BMA400_REG_TEMPERATURE           0x11
#define BMA400_REG_FIFO_LENGTH_0         0x12
#define BMA400_REG_FIFO_LENGTH_1         0x13
#define BMA400_REG_FIFO_DATA             0x14
#define BMA400_REG_STEP_CNT_0            0x15
#define BMA400_REG_STEP_CNT_1            0x16
#define BMA400_REG_STEP_CNT_2            0x17
#define BMA400_REG_STEP_STATUS           0x18
#define BMA400_REG_ACC_CONFIG_0          0x19
#define BMA400_REG_ACC_CONFIG_1          0x1A
#define BMA400_REG_ACC_CONFIG_2          0x1B
#define BMA400_REG_INT_CONFIG_0          0x1F
#define BMA400_REG_INT_CONFIG_1          0x20
#define BMA400_REG_INT1_MAP              0x21
#define BMA400_REG_INT2_MAP              0x22
#define BMA400_REG_INT12_MAP             0x23
#define BMA400_REG_INT12_IO_CTRL         0x24
#define BMA400_REG_FIFO_CONFIG_0         0x26
#define BMA400_REG_FIFO_CONFIG_1         0x27
#define BMA400_REG_FIFO_CONFIG_2         0x28
#define BMA400_REG_FIFO_PWR_CONFIG       0x29
#define BMA400_REG_AUTO_LOW_POW_0        0x2A
#define BMA400_REG_AUTO_LOW_POW_1        0x2B
#define BMA400_REG_AUTO_WAKEUP_0         0x2C
#define BMA400_REG_AUTO_WAKEUP_1         0x2D
#define BMA400_REG_WAKEUP_CONFIG_0       0x2F
#define BMA400_REG_WAKEUP_CONFIG_1       0x30
#define BMA400_REG_WAKEUP_CONFIG_2       0x31
#define BMA400_REG_WAKEUP_CONFIG_3       0x32
#define BMA400_REG_WAKEUP_CONFIG_4       0x33
#define BMA400_REG_ORIENTCH_CONFIG_0     0x35
#define BMA400_REG_ORIENTCH_CONFIG_1     0x36
#define BMA400_REG_ORIENTCH_CONFIG_2     0x37
#define BMA400_REG_ORIENTCH_CONFIG_3     0x38
#define BMA400_REG_ORIENTCH_CONFIG_4     0x39
#define BMA400_REG_ORIENTCH_CONFIG_5     0x3A
#define BMA400_REG_ORIENTCH_CONFIG_6     0x3B
#define BMA400_REG_ORIENTCH_CONFIG_7     0x3C
#define BMA400_REG_ORIENTCH_CONFIG_8     0x3D
#define BMA400_REG_ORIENTCH_CONFIG_9     0x3E
#define BMA400_REG_GEN1_INT_CONFIG_0     0x3F
#define BMA400_REG_GEN1_INT_CONFIG_1     0x40
#define BMA400_REG_GEN1_INT_CONFIG_2     0x41
#define BMA400_REG_GEN1_INT_CONFIG_3     0x42
#define BMA400_REG_GEN1_INT_CONFIG_31    0x43
#define BMA400_REG_GEN1_INT_CONFIG_4     0x44
#define BMA400_REG_GEN1_INT_CONFIG_5     0x45
#define BMA400_REG_GEN1_INT_CONFIG_6     0x46
#define BMA400_REG_GEN1_INT_CONFIG_7     0x47
#define BMA400_REG_GEN1_INT_CONFIG_8     0x48
#define BMA400_REG_GEN1_INT_CONFIG_9     0x49
#define BMA400_REG_GEN2_INT_CONFIG_0     0x4A
#define BMA400_REG_GEN2_INT_CONFIG_1     0x4B
#define BMA400_REG_GEN2_INT_CONFIG_2     0x4C
#define BMA400_REG_GEN2_INT_CONFIG_3     0x4D
#define BMA400_REG_GEN2_INT_CONFIG_31    0x4E
#define BMA400_REG_GEN2_INT_CONFIG_4     0x4F
#define BMA400_REG_GEN2_INT_CONFIG_5     0x50
#define BMA400_REG_GEN2_INT_CONFIG_6     0x51
#define BMA400_REG_GEN2_INT_CONFIG_7     0x52
#define BMA400_REG_GEN2_INT_CONFIG_8     0x53
#define BMA400_REG_GEN2_INT_CONFIG_9     0x54
#define BMA400_REG_ACTCH_CONFIG_0        0x55
#define BMA400_REG_ACTCH_CONFIG_1        0x56
#define BMA400_REG_TAP_CONFIG_0          0x57
#define BMA400_REG_TAP_CONFIG_1          0x58
#define BMA400_REG_STEP_CNT_CONFIG_0     0x59
#define BMA400_REG_STEP_CNT_CONFIG_1     0x5A
#define BMA400_REG_STEP_CNT_CONFIG_2     0x5B
#define BMA400_REG_STEP_CNT_CONFIG_3     0x5C
#define BMA400_REG_STEP_CNT_CONFIG_4     0x5D
#define BMA400_REG_STEP_CNT_CONFIG_5     0x5E
#define BMA400_REG_STEP_CNT_CONFIG_6     0x5F
#define BMA400_REG_STEP_CNT_CONFIG_7     0x60
#define BMA400_REG_STEP_CNT_CONFIG_8     0x61
#define BMA400_REG_STEP_CNT_CONFIG_9     0x62
#define BMA400_REG_STEP_CNT_CONFIG_10    0x63
#define BMA400_REG_STEP_CNT_CONFIG_11    0x64
#define BMA400_REG_STEP_CNT_CONFIG_12    0x65
#define BMA400_REG_STEP_CNT_CONFIG_13    0x66
#define BMA400_REG_STEP_CNT_CONFIG_14    0x67
#define BMA400_REG_STEP_CNT_CONFIG_15    0x68
#define BMA400_REG_STEP_CNT_CONFIG_16    0x69
#define BMA400_REG_STEP_CNT_CONFIG_17    0x6A
#define BMA400_REG_STEP_CNT_CONFIG_18    0x6B
#define BMA400_REG_STEP_CNT_CONFIG_19    0x6C
#define BMA400_REG_STEP_CNT_CONFIG_20    0x6D
#define BMA400_REG_STEP_CNT_CONFIG_21    0x6E
#define BMA400_REG_STEP_CNT_CONFIG_22    0x6F
#define BMA400_REG_STEP_CNT_CONFIG_23    0x70
#define BMA400_REG_STEP_CNT_CONFIG_24    0x71
#define BMA400_REG_IF_CONFIG             0x7C
#define BMA400_REG_SELF_TEST             0x7D
#define BMA400_REG_CMD                   0x7E

/* BMA400 Command register */
#define BMA400_SOFT_RESET_CMD            0xB6
#define BMA400_FIFO_FLUSH_CMD            0xB0

/* BMA400 Delay definitions */
#define BMA400_DELAY_MS_SOFT_RESET           5  /* delay in millisecond */
#define BMA400_DELAY_MS_SELF_TEST            7  /* delay in millisecond */
#define BMA400_DELAY_MS_SELF_TEST_DATA_READ  50 /* delay in millisecond */

/* Interface selection macro */
#define BMA400_SPI_WR_MASK               0x7F
#define BMA400_SPI_RD_MASK               0x80

/* UTILITY MACROS */
#define BMA400_SET_LOW_BYTE              0x00FF
#define BMA400_SET_HIGH_BYTE             0xFF00

/* Interrupt mapping selection */
#define BMA400_DATA_READY_INT_MAP        0x01
#define BMA400_FIFO_WM_INT_MAP           0x02
#define BMA400_FIFO_FULL_INT_MAP         0x03
#define BMA400_GEN2_INT_MAP              0x04
#define BMA400_GEN1_INT_MAP              0x05
#define BMA400_ORIENT_CH_INT_MAP         0x06
#define BMA400_WAKEUP_INT_MAP            0x07
#define BMA400_ACT_CH_INT_MAP            0x08
#define BMA400_TAP_INT_MAP               0x09
#define BMA400_STEP_INT_MAP              0x0A
#define BMA400_INT_OVERRUN_MAP           0x0B

/* BMA400 FIFO configurations */
#define BMA400_FIFO_AUTO_FLUSH           0x01
#define BMA400_FIFO_STOP_ON_FULL         0x02
#define BMA400_FIFO_TIME_EN              0x04
#define BMA400_FIFO_DATA_SRC             0x08
#define BMA400_FIFO_8_BIT_EN             0x10
#define BMA400_FIFO_X_EN                 0x20
#define BMA400_FIFO_Y_EN                 0x40
#define BMA400_FIFO_Z_EN                 0x80

/* BMA400 FIFO data configurations */
#define BMA400_FIFO_EN_X                 0x01
#define BMA400_FIFO_EN_Y                 0x02
#define BMA400_FIFO_EN_Z                 0x04
#define BMA400_FIFO_EN_XY                0x03
#define BMA400_FIFO_EN_YZ                0x06
#define BMA400_FIFO_EN_XZ                0x05
#define BMA400_FIFO_EN_XYZ               0x07

/* BMA400 Self test configurations */
#define BMA400_DISABLE_SELF_TEST         0x00
#define BMA400_ENABLE_POSITIVE_SELF_TEST 0x07
#define BMA400_ENABLE_NEGATIVE_SELF_TEST 0x0F

/* BMA400 FIFO data masks */
#define BMA400_FIFO_HEADER_MASK          0x3E
#define BMA400_FIFO_BYTES_OVERREAD       25
#define BMA400_AWIDTH_MASK               0xEF
#define BMA400_FIFO_DATA_EN_MASK         0x0E

/* BMA400 Step status field - Activity status */
#define BMA400_STILL_ACT                 0x00
#define BMA400_WALK_ACT                  0x01
#define BMA400_RUN_ACT                   0x02

/* It is inserted when FIFO_CONFIG0.fifo_data_src
 * is changed during the FIFO read
 */
#define BMA400_FIFO_CONF0_CHANGE         0x01

/* It is inserted when ACC_CONFIG0.filt1_bw
 * is changed during the FIFO read
 */
#define BMA400_ACCEL_CONF0_CHANGE        0x02

/* It is inserted when ACC_CONFIG1.acc_range
 * acc_odr or osr is changed during the FIFO read
 */
#define BMA400_ACCEL_CONF1_CHANGE        0x04

/* Accel width setting either 12/8 bit mode */
#define BMA400_12_BIT_FIFO_DATA          0x01
#define BMA400_8_BIT_FIFO_DATA           0x00

/* BMA400 FIFO header configurations */
#define BMA400_FIFO_SENSOR_TIME          0xA0
#define BMA400_FIFO_EMPTY_FRAME          0x80
#define BMA400_FIFO_CONTROL_FRAME        0x48
#define BMA400_FIFO_XYZ_ENABLE           0x8E
#define BMA400_FIFO_X_ENABLE             0x82
#define BMA400_FIFO_Y_ENABLE             0x84
#define BMA400_FIFO_Z_ENABLE             0x88
#define BMA400_FIFO_XY_ENABLE            0x86
#define BMA400_FIFO_YZ_ENABLE            0x8C
#define BMA400_FIFO_XZ_ENABLE            0x8A

/* BMA400 bit mask definitions */
#define BMA400_POWER_MODE_STATUS_MSK     (0x06)
#define BMA400_POWER_MODE_STATUS_POS     (1)

#define BMA400_POWER_MODE_MSK            (0x03)

#define BMA400_ACCEL_ODR_MSK             (0x0F)

#define BMA400_ACCEL_RANGE_MSK           (0xC0)
#define BMA400_ACCEL_RANGE_POS           (6)

#define BMA400_DATA_FILTER_MSK           (0x0C)
#define BMA400_DATA_FILTER_POS           (2)

#define BMA400_OSR_MSK                   (0x30)
#define BMA400_OSR_POS                   (4)

#define BMA400_OSR_LP_MSK                (0x60)
#define BMA400_OSR_LP_POS                (5)

#define BMA400_FILT_1_BW_MSK             (0x80)
#define BMA400_FILT_1_BW_POS             (7)

#define BMA400_WAKEUP_TIMEOUT_MSK        (0x04)
#define BMA400_WAKEUP_TIMEOUT_POS        (2)

#define BMA400_WAKEUP_THRES_LSB_MSK      (0x000F)

#define BMA400_WAKEUP_THRES_MSB_MSK      (0x0FF0)
#define BMA400_WAKEUP_THRES_MSB_POS      (4)

#define BMA400_WAKEUP_TIMEOUT_THRES_MSK  (0xF0)
#define BMA400_WAKEUP_TIMEOUT_THRES_POS  (4)

#define BMA400_WAKEUP_INTERRUPT_MSK      (0x02)
#define BMA400_WAKEUP_INTERRUPT_POS      (1)

#define BMA400_AUTO_LOW_POW_MSK          (0x0F)

#define BMA400_AUTO_LP_THRES_MSK         (0x0FF0)
#define BMA400_AUTO_LP_THRES_POS         (4)

#define BMA400_AUTO_LP_THRES_LSB_MSK     (0x000F)

#define BMA400_WKUP_REF_UPDATE_MSK       (0x03)

#define BMA400_AUTO_LP_TIMEOUT_LSB_MSK   (0xF0)
#define BMA400_AUTO_LP_TIMEOUT_LSB_POS   (4)

#define BMA400_SAMPLE_COUNT_MSK          (0x1C)
#define BMA400_SAMPLE_COUNT_POS          (2)

#define BMA400_WAKEUP_EN_AXES_MSK        (0xE0)
#define BMA400_WAKEUP_EN_AXES_POS        (5)

#define BMA400_TAP_AXES_EN_MSK           (0x18)
#define BMA400_TAP_AXES_EN_POS           (3)

#define BMA400_TAP_QUIET_DT_MSK          (0x30)
#define BMA400_TAP_QUIET_DT_POS          (4)

#define BMA400_TAP_QUIET_MSK             (0x0C)
#define BMA400_TAP_QUIET_POS             (2)

#define BMA400_TAP_TICS_TH_MSK           (0x03)

#define BMA400_TAP_SENSITIVITY_MSK       (0X07)

#define BMA400_ACT_CH_AXES_EN_MSK        (0xE0)
#define BMA400_ACT_CH_AXES_EN_POS        (5)

#define BMA400_ACT_CH_DATA_SRC_MSK       (0x10)
#define BMA400_ACT_CH_DATA_SRC_POS       (4)

#define BMA400_ACT_CH_NPTS_MSK           (0x0F)

#define BMA400_INT_AXES_EN_MSK           (0xE0)
#define BMA400_INT_AXES_EN_POS           (5)

#define BMA400_INT_DATA_SRC_MSK          (0x10)
#define BMA400_INT_DATA_SRC_POS          (4)

#define BMA400_INT_REFU_MSK              (0x0C)
#define BMA400_INT_REFU_POS              (2)

#define BMA400_INT_HYST_MSK              (0x03)
#define BMA400_STABILITY_MODE_MSK        (0x03)

#define BMA400_GEN_INT_COMB_MSK          (0x01)

#define BMA400_GEN_INT_CRITERION_MSK     (0x02)
#define BMA400_GEN_INT_CRITERION_POS     (0x01)

#define BMA400_INT_PIN1_CONF_MSK         (0x06)
#define BMA400_INT_PIN1_CONF_POS         (1)

#define BMA400_INT_PIN2_CONF_MSK         (0x60)
#define BMA400_INT_PIN2_CONF_POS         (5)

#define BMA400_INT_STATUS_MSK            (0xE0)
#define BMA400_INT_STATUS_POS            (5)

#define BMA400_EN_DRDY_MSK               (0x80)
#define BMA400_EN_DRDY_POS               (7)

#define BMA400_EN_FIFO_WM_MSK            (0x40)
#define BMA400_EN_FIFO_WM_POS            (6)

#define BMA400_EN_FIFO_FULL_MSK          (0x20)
#define BMA400_EN_FIFO_FULL_POS          (5)

#define BMA400_EN_INT_OVERRUN_MSK        (0x10)
#define BMA400_EN_INT_OVERRUN_POS        (4)

#define BMA400_EN_GEN2_MSK               (0x08)
#define BMA400_EN_GEN2_POS               (3)

#define BMA400_EN_GEN1_MSK               (0x04)
#define BMA400_EN_GEN1_POS               (2)

#define BMA400_EN_ORIENT_CH_MSK          (0x02)
#define BMA400_EN_ORIENT_CH_POS          (1)

#define BMA400_EN_LATCH_MSK              (0x80)
#define BMA400_EN_LATCH_POS              (7)

#define BMA400_EN_ACTCH_MSK              (0x10)
#define BMA400_EN_ACTCH_POS              (4)

#define BMA400_EN_D_TAP_MSK              (0x08)
#define BMA400_EN_D_TAP_POS              (3)

#define BMA400_EN_S_TAP_MSK              (0x04)
#define BMA400_EN_S_TAP_POS              (2)

#define BMA400_EN_STEP_INT_MSK           (0x01)

#define BMA400_STEP_MAP_INT2_MSK         (0x10)
#define BMA400_STEP_MAP_INT2_POS         (4)

#define BMA400_EN_WAKEUP_INT_MSK         (0x01)

#define BMA400_TAP_MAP_INT1_MSK          (0x04)
#define BMA400_TAP_MAP_INT1_POS          (2)

#define BMA400_TAP_MAP_INT2_MSK          (0x40)
#define BMA400_TAP_MAP_INT2_POS          (6)

#define BMA400_ACTCH_MAP_INT1_MSK        (0x08)
#define BMA400_ACTCH_MAP_INT1_POS        (3)

#define BMA400_ACTCH_MAP_INT2_MSK        (0x80)
#define BMA400_ACTCH_MAP_INT2_POS        (7)

#define BMA400_FIFO_BYTES_CNT_MSK        (0x07)

#define BMA400_FIFO_TIME_EN_MSK          (0x04)
#define BMA400_FIFO_TIME_EN_POS          (2)

#define BMA400_FIFO_AXES_EN_MSK          (0xE0)
#define BMA400_FIFO_AXES_EN_POS          (5)

#define BMA400_FIFO_8_BIT_EN_MSK         (0x10)
#define BMA400_FIFO_8_BIT_EN_POS         (4)

/* Macros used for Self test */
/* Self-test: Resulting minimum difference signal in mg for BMA400 */
#define BMA400_ST_ACC_X_AXIS_SIGNAL_DIFF 1500
#define BMA400_ST_ACC_Y_AXIS_SIGNAL_DIFF 1200
#define BMA400_ST_ACC_Z_AXIS_SIGNAL_DIFF 250

/* Sensor selection enums */
typedef enum bma400_selection {
    BMA400_ACCEL = 0,
    BMA400_TAP_INT,
    BMA400_ACTIVITY_CHANGE_INT,
    BMA400_GEN1_INT,
    BMA400_GEN2_INT,
    BMA400_ORIENT_CHANGE_INT,
    BMA400_STEP_COUNTER_INT
} bma400_selection_t;

/*
 * Interrupt channel selection enums
 */
typedef enum bma400_int_chan {
    BMA400_UNMAP_INT_PIN,
    BMA400_INT_CHANNEL_1,
    BMA400_INT_CHANNEL_2,
    BMA400_MAP_BOTH_INT_PINS
} bma400_int_chan_t;

/* Power mode configurations enums */
typedef enum bma400_power_mode {
    BMA400_MODE_SLEEP = 0x00,
    BMA400_MODE_LOW_POWER = 0x01,
    BMA400_MODE_NORMAL = 0x02
} bma400_power_mode_t;

/* Data/sensortime selection enums */
typedef enum data_selection {
    BMA400_DATA_ONLY = 0x00,
    BMA400_DATA_SENSOR_TIME = 0x01
} data_selection_t;

/*
 * Interrupt pin hardware configurations
 */
typedef struct bma400_int_pin_conf
{
    /* Interrupt channel selection enums */
    bma400_int_chan_t int_chan;

    /* Interrupt pin configuration
     * Assignable Macros :
     *  - BMA400_INT_PUSH_PULL_ACTIVE_0
     *  - BMA400_INT_PUSH_PULL_ACTIVE_1
     *  - BMA400_INT_OPEN_DRIVE_ACTIVE_0
     *  - BMA400_INT_OPEN_DRIVE_ACTIVE_1
     */
    uint8_t pin_conf;
} bma400_int_pin_conf_t;

/*
 * Accel basic configuration
 */
typedef struct bma400_acc_conf
{
    /* Output data rate
     * Assignable macros :
     *  - BMA400_ODR_12_5HZ  - BMA400_ODR_25HZ   - BMA400_ODR_50HZ
     *  - BMA400_ODR_100HZ   - BMA400_ODR_200HZ  - BMA400_ODR_400HZ
     *  - BMA400_ODR_800HZ
     */
    uint8_t odr;

    /* Range of sensor
     * Assignable macros :
     *  - BMA400_2G_RANGE   - BMA400_8G_RANGE
     *  - BMA400_4G_RANGE   - BMA400_16G_RANGE
     */
    uint8_t range;

    /* Filter setting for data source
     * Assignable Macros :
     * - BMA400_DATA_SRC_ACCEL_FILT_1
     * - BMA400_DATA_SRC_ACCEL_FILT_2
     * - BMA400_DATA_SRC_ACCEL_FILT_LP
     */
    uint8_t data_src;

    /* Assignable Macros for osr and osr_lp:
     * - BMA400_ACCEL_OSR_SETTING_0     - BMA400_ACCEL_OSR_SETTING_2
     * - BMA400_ACCEL_OSR_SETTING_1     - BMA400_ACCEL_OSR_SETTING_3
     */

    /* OSR setting for data source */
    uint8_t osr;

    /* OSR setting for low power mode */
    uint8_t osr_lp;

    /* Filter 1 Bandwidth
     * Assignable macros :
     *  - BMA400_ACCEL_FILT1_BW_0
     *  - BMA400_ACCEL_FILT1_BW_1
     */
    uint8_t filt1_bw;

    /* Interrupt channel to be mapped */
    bma400_int_chan_t int_chan;
} bma400_acc_conf_t;

/*
 * Tap interrupt configurations
 */
typedef struct bma400_tap_conf
{
    /* Axes enabled to sense tap setting
     * Assignable macros :
     *   - BMA400_X_AXIS_EN_TAP
     *   - BMA400_Y_AXIS_EN_TAP
     *   - BMA400_Z_AXIS_EN_TAP
     */
    uint8_t axes_sel;

    /* TAP sensitivity settings modifies the threshold
     *  for minimum TAP amplitude
     * Assignable macros :
     *   - BMA400_TAP_SENSITIVITY_0  - BMA400_TAP_SENSITIVITY_4
     *   - BMA400_TAP_SENSITIVITY_1  - BMA400_TAP_SENSITIVITY_5
     *   - BMA400_TAP_SENSITIVITY_2  - BMA400_TAP_SENSITIVITY_6
     *   - BMA400_TAP_SENSITIVITY_3  - BMA400_TAP_SENSITIVITY_7
     *
     * @note :
     *  - BMA400_TAP_SENSITIVITY_0 correspond to highest sensitivity
     *  - BMA400_TAP_SENSITIVITY_7 correspond to lowest sensitivity
     */
    uint8_t sensitivity;

    /* TAP tics_th setting is the maximum time between upper and lower
     * peak of a tap, in data samples, This time depends on the
     * mechanics of the device tapped onto  default = 12 samples
     * Assignable macros :
     *   - BMA400_TICS_TH_6_DATA_SAMPLES
     *   - BMA400_TICS_TH_9_DATA_SAMPLES
     *   - BMA400_TICS_TH_12_DATA_SAMPLES
     *   - BMA400_TICS_TH_18_DATA_SAMPLES
     */
    uint8_t tics_th;

    /* BMA400 TAP - quiet  settings to configure minimum quiet time
     *  before and after double tap, in the data samples.
     * This time also defines the longest time interval between two
     * taps so that they are considered as double tap
     * Assignable macros :
     *   - BMA400_QUIET_60_DATA_SAMPLES
     *   - BMA400_QUIET_80_DATA_SAMPLES
     *   - BMA400_QUIET_100_DATA_SAMPLES
     *   - BMA400_QUIET_120_DATA_SAMPLES
     */
    uint8_t quiet;

    /* BMA400 TAP - quiet_dt  settings
     * quiet_dt refers to Minimum time between the two taps of a
     * double tap, in data samples
     * Assignable macros :
     *   - BMA400_QUIET_DT_4_DATA_SAMPLES
     *   - BMA400_QUIET_DT_8_DATA_SAMPLES
     *   - BMA400_QUIET_DT_12_DATA_SAMPLES
     *   - BMA400_QUIET_DT_16_DATA_SAMPLES
     */
    uint8_t quiet_dt;

    /* Interrupt channel to be mapped */
    bma400_int_chan_t int_chan;
} bma400_tap_conf_t;

/*
 * Activity change interrupt configurations
 */
typedef struct bma400_act_ch_conf
{
    /* Threshold for activity change (8 mg/LSB) */
    uint8_t act_ch_thres;

    /* Axes enabled to sense activity change
     * Assignable macros :
     *   - BMA400_X_AXIS_EN
     *   - BMA400_Y_AXIS_EN
     *   - BMA400_Z_AXIS_EN
     *   - BMA400_XYZ_AXIS_EN
     */
    uint8_t axes_sel;

    /* Data Source for activity change
     * Assignable macros :
     *    - BMA400_DATA_SRC_ACC_FILT1
     *    - BMA400_DATA_SRC_ACC_FILT2
     */
    uint8_t data_source;

    /* Sample count for sensing act_ch
     * Assignable macros :
     *  - BMA400_ACT_CH_SAMPLE_CNT_32
     *  - BMA400_ACT_CH_SAMPLE_CNT_64
     *  - BMA400_ACT_CH_SAMPLE_CNT_128
     *  - BMA400_ACT_CH_SAMPLE_CNT_256
     *  - BMA400_ACT_CH_SAMPLE_CNT_512
     */
    uint8_t act_ch_ntps;

    /* Interrupt channel to be mapped */
    bma400_int_chan_t int_chan;
} bma400_act_ch_conf_t;

/*
 * Generic interrupt configurations
 */
typedef struct bma400_gen_int_conf
{
    /* Threshold for the gen1 interrupt (1 LSB = 8mg)
     * if gen_int_thres = 10, then threshold = 10 * 8 = 80mg
     */
    uint8_t gen_int_thres;

    /* Duration for which the condition has to persist until
     *  interrupt can be triggered
     *  duration is measured in data samples of selected data source
     */
    uint16_t gen_int_dur;

    /* Enable axes to sense for the gen1 interrupt
     * Assignable macros :
     *  - BMA400_X_AXIS_EN
     *  - BMA400_Y_AXIS_EN
     *  - BMA400_Z_AXIS_EN
     *  - BMA400_XYZ_AXIS_EN
     */
    uint8_t axes_sel;

    /* Data source to sense for the gen1 interrupt
     * Assignable macros :
     *  - BMA400_DATA_SRC_ACC_FILT1
     *  - BMA400_DATA_SRC_ACC_FILT2
     */
    uint8_t data_src;

    /* Activity/Inactivity selection macros
     * Assignable macros :
     *  - BMA400_ACTIVITY_INT
     *  - BMA400_INACTIVITY_INT
     */
    uint8_t criterion_sel;

    /* Axes selection logic macros
     * Assignable macros :
     *  - BMA400_ALL_AXES_INT
     *  - BMA400_ANY_AXES_INT
     */
    uint8_t evaluate_axes;

    /* Reference x,y,z values updates
     * Assignable macros :
     *  - BMA400_MANUAL_UPDATE
     *  - BMA400_ONE_TIME_UPDATE
     *  - BMA400_EVERY_TIME_UPDATE
     *  - BMA400_LP_EVERY_TIME_UPDATE
     */
    uint8_t ref_update;

    /* Hysteresis value
     * Higher the hysteresis value, Lower the value of noise
     * Assignable macros :
     *  - BMA400_HYST_0_MG
     *  - BMA400_HYST_24_MG
     *  - BMA400_HYST_48_MG
     *  - BMA400_HYST_96_MG
     */
    uint8_t hysteresis;

    /* Threshold value for x axes */
    uint16_t int_thres_ref_x;

    /* Threshold value for y axes */
    uint16_t int_thres_ref_y;

    /* Threshold value for z axes */
    uint16_t int_thres_ref_z;

    /* Interrupt channel to be mapped */
    bma400_int_chan_t int_chan;
} bma400_gen_int_conf_t;

/*
 * Orient interrupt configurations
 */
typedef struct bma400_orient_int_conf
{
    /* Enable axes to sense for the gen1 interrupt
     * Assignable macros :
     *  - BMA400_X_AXIS_EN
     *  - BMA400_Y_AXIS_EN
     *  - BMA400_Z_AXIS_EN
     *  - BMA400_XYZ_AXIS_EN
     */
    uint8_t axes_sel;

    /* Data source to sense for the gen1 interrupt
     * Assignable macros :
     *  - BMA400_DATA_SRC_ACC_FILT1
     *  - BMA400_DATA_SRC_ACC_FILT2
     */
    uint8_t data_src;

    /* Reference x,y,z values updates
     * Assignable macros :
     *  - BMA400_MANUAL_UPDATE
     *  - BMA400_ORIENT_REFU_ACC_FILT_2
     *  - BMA400_ORIENT_REFU_ACC_FILT_LP
     */
    uint8_t ref_update;

    /* Threshold for the orient interrupt (1 LSB = 8mg)
     * if orient_thres = 10, then threshold = 10 * 8 = 80mg
     */
    uint8_t orient_thres;

    /* Threshold to check for stability (1 LSB = 8mg)
     * if stability_thres = 10, then threshold = 10 * 8 = 80mg
     */
    uint8_t stability_thres;

    /* orient_int_dur duration in which orient interrupt
     * should occur, It is 8bit value configurable at 10ms/LSB.
     */
    uint8_t orient_int_dur;

    /* Stability check conditions
     * Assignable macros :
     *  - BMA400_STABILITY_DISABLED
     *  - BMA400_STABILITY_ACC_FILT_2
     *  - BMA400_STABILITY_ACC_FILT_LP
     */
    uint8_t stability_mode;

    /* Reference value for x axes */
    uint16_t orient_ref_x;

    /* Reference value for y axes */
    uint16_t orient_ref_y;

    /* Reference value for z axes */
    uint16_t orient_ref_z;

    /* Interrupt channel to be mapped */
    bma400_int_chan_t int_chan;
} bma400_orient_int_conf_t;

/* Step counter configurations */
typedef struct bma400_step_int_conf
{
    /* Interrupt channel to be mapped */
    bma400_int_chan_t int_chan;
} bma400_step_int_conf_t;

/*
 * Union of sensor Configurations
 */
typedef union bma400_set_param
{
  /* Accel configurations */
  bma400_acc_conf_t accel;

  /* TAP configurations */
  bma400_tap_conf_t tap;

  /* Activity change configurations */
  bma400_act_ch_conf_t act_ch;

  /* Generic interrupt configurations */
  bma400_gen_int_conf_t gen_int;

  /* Orient configurations */
  bma400_orient_int_conf_t orient;

  /* Step counter configurations */
  bma400_step_int_conf_t step_cnt;
} bma400_set_param_t;

/*
 * Sensor selection and their configurations
 */
typedef struct bma400_sensor_conf
{
  /* Sensor selection */
  bma400_selection_t type;

  /* Sensor configuration */
  bma400_set_param_t param;
} bma400_sensor_conf_t;

/*
 * enum to select device settings
 */
typedef enum bma400_device {
    BMA400_AUTOWAKEUP_TIMEOUT,
    BMA400_AUTOWAKEUP_INT,
    BMA400_AUTO_LOW_POWER,
    BMA400_INT_PIN_CONF,
    BMA400_INT_OVERRUN_CONF,
    BMA400_FIFO_CONF
} bma400_device_t;

/*
 * BMA400 auto-wakeup configurations
 */
typedef struct bma400_auto_wakeup_conf
{
    /* Enable auto wake-up by using timeout threshold
     * Assignable Macros :
     *   - BMA400_ENABLE    - BMA400_DISABLE
     */
    uint8_t wakeup_timeout;

    /* Timeout threshold after which auto wake-up occurs
     * It is 12bit value configurable at 2.5ms/LSB
     * Maximum timeout is 10.24s (4096 * 2.5) for
     * which the assignable macro is :
     *      - BMA400_AUTO_WAKEUP_TIMEOUT_MAX
     */
    uint16_t timeout_thres;
} bma400_auto_wakeup_conf_t;

/*
 * BMA400 wakeup configurations
 */
typedef struct bma400_wakeup_conf
{
    /* Wakeup reference update
     *  Assignable macros:
     *   - BMA400_MANUAL_UPDATE
     *   - BMA400_ONE_TIME_UPDATE
     *   - BMA400_EVERY_TIME_UPDATE
     */
    uint8_t wakeup_ref_update;

    /* Number of samples for interrupt condition evaluation
     * Assignable Macros :
     *  - BMA400_SAMPLE_COUNT_1  - BMA400_SAMPLE_COUNT_5
     *  - BMA400_SAMPLE_COUNT_2  - BMA400_SAMPLE_COUNT_6
     *  - BMA400_SAMPLE_COUNT_3  - BMA400_SAMPLE_COUNT_7
     *  - BMA400_SAMPLE_COUNT_4  - BMA400_SAMPLE_COUNT_8
     */
    uint8_t sample_count;

    /* Enable low power wake-up interrupt for X(BIT 0), Y(BIT 1), Z(BIT 2)
     * axes  0 - not active; 1 - active
     * Assignable macros :
     *  - BMA400_X_AXIS_EN
     *  - BMA400_Y_AXIS_EN
     *  - BMA400_Z_AXIS_EN
     *  - BMA400_XYZ_AXIS_EN
     */
    uint8_t wakeup_axes_en;

    /* Interrupt threshold configuration  */
    uint8_t int_wkup_threshold;

    /* Reference acceleration x-axis for the wake-up interrupt */
    uint8_t int_wkup_ref_x;

    /* Reference acceleration y-axis for the wake-up interrupt */
    uint8_t int_wkup_ref_y;

    /* Reference acceleration z-axis for the wake-up interrupt */
    uint8_t int_wkup_ref_z;

    /* Interrupt channel to be mapped */
    bma400_int_chan_t int_chan;
} bma400_wakeup_conf_t;

/*
 * BMA400 auto-low power configurations
 */
typedef struct bma400_auto_lp_conf
{
    /* Enable auto low power mode using  data ready interrupt /
     * Genric interrupt1 / timeout counter value
     * Assignable macros :
     * - BMA400_AUTO_LP_DRDY_TRIGGER
     * - BMA400_AUTO_LP_GEN1_TRIGGER
     * - BMA400_AUTO_LP_TIMEOUT_EN
     * - BMA400_AUTO_LP_TIME_RESET_EN
     * - BMA400_AUTO_LP_TIMEOUT_DISABLE
     */
    uint8_t auto_low_power_trigger;

    /* Timeout threshold after which auto wake-up occurs
     * It is 12bit value configurable at 2.5ms/LSB
     * Maximum timeout is 10.24s (4096 * 2.5) for
     *  which the assignable macro is :
     *  - BMA400_AUTO_LP_TIMEOUT_MAX
     */
    uint16_t auto_lp_timeout_threshold;
} bma400_auto_lp_conf_t;

/*
 * FIFO configurations
 */
typedef struct bma400_fifo_conf
{
    /* Select FIFO configurations to enable/disable
     * Assignable Macros :
     *   - BMA400_FIFO_AUTO_FLUSH
     *   - BMA400_FIFO_STOP_ON_FULL
     *   - BMA400_FIFO_TIME_EN
     *   - BMA400_FIFO_DATA_SRC
     *   - BMA400_FIFO_8_BIT_EN
     *   - BMA400_FIFO_X_EN
     *   - BMA400_FIFO_Y_EN
     *   - BMA400_FIFO_Z_EN
     */
    uint8_t conf_regs;

    /* Enable/ disable selected FIFO configurations
     * Assignable Macros :
     *   - BMA400_ENABLE
     *   - BMA400_DISABLE
     */
    uint8_t conf_status;

    /* Value to set the water-mark */
    uint16_t fifo_watermark;

    /* Interrupt pin mapping for FIFO full interrupt */
    bma400_int_chan_t fifo_full_channel;

    /* Interrupt pin mapping for FIFO water-mark interrupt */
    bma400_int_chan_t fifo_wm_channel;
} bma400_fifo_conf_t;

/*
 * Interrupt overrun configurations
 */
typedef struct bma400_int_overrun
{
    /* Interrupt pin mapping for interrupt overrun */
    bma400_int_chan_t int_chan;
} bma400_int_overrun_t;

/*
 * Union of device configuration parameters
 */
typedef union bma400_device_params
{
    /* Auto wakeup configurations */
    bma400_auto_wakeup_conf_t auto_wakeup;

    /* Wakeup interrupt configurations */
    bma400_wakeup_conf_t wakeup;

    /* Auto Low power configurations */
    bma400_auto_lp_conf_t auto_lp;

    /* Interrupt pin configurations */
    bma400_int_pin_conf_t int_conf;

    /* FIFO configuration */
    bma400_fifo_conf_t fifo_conf;

    /* Interrupt overrun configuration */
    bma400_int_overrun_t overrun_int;
} bma400_device_params_t;

/*
 * BMA400 device configuration
 */
typedef struct bma400_device_conf
{
    /* Device feature selection */
    bma400_device_t type;

    /* Device feature configuration */
    bma400_device_params_t param;
}bma400_device_conf_t;

/*
 * BMA400 sensor data
 */
typedef struct bma400_sensor_data
{
    /* X-axis sensor data */
    int16_t x;

    /* Y-axis sensor data */
    int16_t y;

    /* Z-axis sensor data */
    int16_t z;

    /* sensor time */
    uint32_t sensortime;
} bma400_sensor_data_t;

/*
 * @brief Accel self test diff xyz data structure
 */
typedef struct bma400_selftest_delta_limit
{
    /* Accel X  data */
    int32_t x;

    /* Accel Y  data */
    int32_t y;

    /* Accel Z  data */
    int32_t z;
} bma400_selftest_delta_limit_t;

/*
 * BMA400 interrupt selection
 */
typedef enum bma400_int_type {
    /* DRDY interrupt */
    BMA400_DRDY_INT_EN,

    /* FIFO watermark interrupt */
    BMA400_FIFO_WM_INT_EN,

    /* FIFO full interrupt */
    BMA400_FIFO_FULL_INT_EN,

    /* Generic interrupt 2 */
    BMA400_GEN2_INT_EN,

    /* Generic interrupt 1 */
    BMA400_GEN1_INT_EN,

    /* Orient change interrupt */
    BMA400_ORIENT_CHANGE_INT_EN,

    /* Latch interrupt */
    BMA400_LATCH_INT_EN,

    /* Activity change interrupt */
    BMA400_ACTIVITY_CHANGE_INT_EN,

    /* Double tap interrupt */
    BMA400_DOUBLE_TAP_INT_EN,

    /* Single tap interrupt */
    BMA400_SINGLE_TAP_INT_EN,

    /* Step interrupt */
    BMA400_STEP_COUNTER_INT_EN,

    /* Auto wakeup interrupt */
    BMA400_AUTO_WAKEUP_EN
} bma400_int_type_t;

/*
 * Interrupt enable/disable configurations
 */
typedef struct bma400_int_enable
{
    /* Enum to choose the interrupt to be enabled */
    bma400_int_type_t type;

    /* Enable/ disable selected interrupts
     * Assignable Macros :
     *   - BMA400_ENABLE
     *   - BMA400_DISABLE
     */
    uint8_t conf;
} bma400_int_enable_t;

typedef struct bma400_fifo_data
{
    /* Data buffer of user defined length is to be mapped here */
    uint8_t *data;

    /* While calling the API  "bma400_get_fifo_data" , length stores
     *  number of bytes in FIFO to be read (specified by user as input)
     *  and after execution of the API ,number of FIFO data bytes
     *  available is provided as an output to user
     */
    uint16_t length;

    /* FIFO time enable */
    uint8_t fifo_time_enable;

    /* FIFO 8bit mode enable */
    uint8_t fifo_8_bit_en;

    /* Streaming of the Accelerometer data for selected x,y,z axes
     *   - BMA400_FIFO_X_EN
     *   - BMA400_FIFO_Y_EN
     *   - BMA400_FIFO_Z_EN
     */
    uint8_t fifo_data_enable;

    /* Will be equal to length when no more frames are there to parse */
    uint16_t accel_byte_start_idx;

    /* It stores the value of configuration changes
     * in sensor during FIFO read
     */
    uint8_t conf_change;

    /* Value of FIFO sensor time time */
    uint32_t fifo_sensor_time;
} bma400_fifo_data_t;

/**************************************************************************//**
 * @brief Initialize bma400
 *  This API reads the chip-id of the sensor which is the first step to verify
 *  the sensor and also it configures the read mechanism of SPI and I2C interface.
 *  As this API is the entry point, call this API before using other APIs.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_init(void);

/**************************************************************************//**
 * @brief Soft reset bma400
 *  Soft-resets the sensor where all the registers are reset.
 *  to their default values except 0x4B.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_soft_reset(void);

/**************************************************************************//**
 * @brief Set power mode to bma400
 *
 * @param[in] power_mode
 *  Select power mode of the sensor.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_set_power_mode(bma400_power_mode_t power_mode);

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
sl_status_t bma400_get_power_mode(bma400_power_mode_t *power_mode);

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
                                  bma400_sensor_data_t *accel);

/**************************************************************************//**
 * @brief Writes the fifo_flush command into command register.
 *  This action clears all data in the FIFO.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_set_fifo_flush(void);

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
sl_status_t bma400_get_fifo_data(bma400_fifo_data_t *fifo);

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
                                 uint16_t *frame_count);

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
sl_status_t bma400_get_sensor_conf(bma400_sensor_conf_t *conf, uint16_t n_sett);

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
                                   uint16_t n_sett);

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
sl_status_t bma400_get_device_conf(bma400_device_conf_t *conf, uint8_t n_sett);

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
sl_status_t bma400_set_device_conf(const bma400_device_conf_t *conf, uint8_t n_sett);

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
                                    uint8_t n_sett);

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
sl_status_t bma400_get_interrupt_status(uint16_t *int_status);

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
sl_status_t bma400_get_interrupts_enabled(bma400_int_enable_t *int_select, uint8_t n_sett);

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
sl_status_t bma400_get_temperature_data(int16_t *temperature_data);

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
sl_status_t bma400_set_step_counter_param(const uint8_t *sccr_conf);

/**************************************************************************//**
 * @brief Get the step counter output in form of number of steps in the step_count value.
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
sl_status_t bma400_get_steps_counted(uint32_t *step_count, uint8_t *activity_data);

/**************************************************************************//**
 * @brief Performs a self test of the accelerometer in BMA400..
 *
 * @note The return value of this API is the result of self test.
 * A self test does not soft reset of the sensor. Hence, the user can
 * define the required settings after performing the self test.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
sl_status_t bma400_perform_self_test(void);

/**************************************************************************//**
 * @brief Delay function.
 *
 * @param[in] time_ms
 *  Time delay in ms.
 *
 * @return
 *  SL_STATUS_OK on success, otherwise error code.
 *****************************************************************************/
void bma400_delay_ms(uint16_t time_ms);

#ifdef __cplusplus
}
#endif

#endif
