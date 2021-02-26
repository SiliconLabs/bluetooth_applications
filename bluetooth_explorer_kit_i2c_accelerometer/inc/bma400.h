/***************************************************************************//**
* @file
* @brief Silicon Labs BMA400 driver
*
* BMA accelerometer sensor i2C driver header file.
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
#ifndef BMA400_H
#define BMA400_H

/* ########################################################################## */
/*                           System includes                                  */
/* ########################################################################## */

/* ########################################################################## */
/*                          Non system includes                               */
/* ########################################################################## */
#include "em_i2c.h" /* I2C bus definitions */
#include "em_gpio.h" /* Clock definitions */
#include "em_cmu.h" /* GPIO definitions */
#if 0
#include "em_spi.h" /* SPI bus definitions */
#endif

#include "bma400_conf.h" /* Module configuration */

/* ########################################################################## */
/*                             Mecros Defn                                    */
/* ########################################################################## */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ TO BE REMOVED ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Macro to SET and GET BITS of a register */
#define BMA400_SET_BITS(reg_data, bitname, data) \
    ((reg_data & ~(bitname##_MSK)) |   \
     ((data << bitname##_POS) & bitname##_MSK))

#define BMA400_GET_BITS(reg_data, bitname)       ((reg_data & (bitname##_MSK)) >> \
                                                  (bitname##_POS))

#define BMA400_SET_BITS_POS_0(reg_data, bitname, data) \
    ((reg_data & ~(bitname##_MSK)) |         \
     (data & bitname##_MSK))

#define BMA400_GET_BITS_POS_0(reg_data, bitname) (reg_data & (bitname##_MSK))

#define BMA400_SET_BIT_VAL_0(reg_data, bitname)  (reg_data & ~(bitname##_MSK))

#define BMA400_GET_LSB(var)                      (uint8_t)(var & BMA400_SET_LOW_BYTE)
#define BMA400_GET_MSB(var)                      (uint8_t)((var & BMA400_SET_HIGH_BYTE) >> 8)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ TO BE REMOVED ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* API warning codes */
#define BMA400_W_SELF_TEST_FAIL        (1)

/* Registers */
#define BMA400_CHIP_ID                 (0x90) /* CHIP ID VALUE */

/* Power mode configurations */
#define BMA400_NORMAL_MODE             (0x02)
#define BMA400_SLEEP_MODE              (0x00)
#define BMA400_LOW_POWER_MODE          (0x01)

/* Enable / Disable macros */
#define BMA400_DISABLE                 (0)
#define BMA400_ENABLE                  (1)

/* Data/sensortime selection macros */
#define BMA400_DATA_ONLY               (0x00)
#define BMA400_DATA_SENSOR_TIME        (0x01)

/* ODR configurations  */
#define BMA400_ODR_12_5HZ              (0x05)
#define BMA400_ODR_25HZ                (0x06)
#define BMA400_ODR_50HZ                (0x07)
#define BMA400_ODR_100HZ               (0x08)
#define BMA400_ODR_200HZ               (0x09)
#define BMA400_ODR_400HZ               (0x0A)
#define BMA400_ODR_800HZ               (0x0B)

/* Accel Range configuration */
#define BMA400_2G_RANGE                (0x00)
#define BMA400_4G_RANGE                (0x01)
#define BMA400_8G_RANGE                (0x02)
#define BMA400_16G_RANGE               (0x03)

/* Accel Axes selection settings for
 * DATA SAMPLING, WAKEUP, ORIENTATION CHANGE,
 * GEN1, GEN2 , ACTIVITY CHANGE
 */
#define BMA400_X_AXIS_EN               (0x01)
#define BMA400_Y_AXIS_EN               (0x02)
#define BMA400_Z_AXIS_EN               (0x04)
#define BMA400_XYZ_AXIS_EN             (0x07)

/* Accel filter(data_src_reg) selection settings */
#define BMA400_DATA_SRC_ACCEL_FILT_1   (0x00)
#define BMA400_DATA_SRC_ACCEL_FILT_2   (0x01)
#define BMA400_DATA_SRC_ACCEL_FILT_LP  (0x02)

/* Accel OSR (OSR,OSR_LP) settings */
#define BMA400_ACCEL_OSR_SETTING_0     (0x00)
#define BMA400_ACCEL_OSR_SETTING_1     (0x01)
#define BMA400_ACCEL_OSR_SETTING_2     (0x02)
#define BMA400_ACCEL_OSR_SETTING_3     (0x03)

/* Accel filt1_bw settings */
/* Accel filt1_bw = 0.48 * ODR */
#define BMA400_ACCEL_FILT1_BW_0        (0x00)

/* Accel filt1_bw = 0.24 * ODR */
#define BMA400_ACCEL_FILT1_BW_1        (0x01)

/* Auto wake-up timeout value of 10.24s */
#define BMA400_AUTO_WAKEUP_TIMEOUT_MAX (0x0FFF)

/* Auto low power timeout value of 10.24s */
#define BMA400_AUTO_LP_TIMEOUT_MAX     (0x0FFF)

/* Reference Update macros */
#define BMA400_MANUAL_UPDATE           (0x00)
#define BMA400_ONE_TIME_UPDATE         (0x01)
#define BMA400_EVERY_TIME_UPDATE       (0x02)
#define BMA400_LP_EVERY_TIME_UPDATE    (0x03)

/* Reference Update macros for orient interrupts */
#define BMA400_ORIENT_REFU_ACC_FILT_2  (0x01)
#define BMA400_ORIENT_REFU_ACC_FILT_LP (0x02)

/* Stability evaluation macros for orient interrupts */
#define BMA400_STABILITY_DISABLED      (0x00)
#define BMA400_STABILITY_ACC_FILT_2    (0x01)
#define BMA400_STABILITY_ACC_FILT_LP   (0x02)

/* Number of samples needed for Auto-wakeup interrupt evaluation  */
#define BMA400_SAMPLE_COUNT_1          (0x00)
#define BMA400_SAMPLE_COUNT_2          (0x01)
#define BMA400_SAMPLE_COUNT_3          (0x02)
#define BMA400_SAMPLE_COUNT_4          (0x03)
#define BMA400_SAMPLE_COUNT_5          (0x04)
#define BMA400_SAMPLE_COUNT_6          (0x05)
#define BMA400_SAMPLE_COUNT_7          (0x06)
#define BMA400_SAMPLE_COUNT_8          (0x07)

/* Auto low power configurations */
/* Auto low power timeout disabled  */
#define BMA400_AUTO_LP_TIMEOUT_DISABLE (0x00)

/* Auto low power entered on drdy interrupt */
#define BMA400_AUTO_LP_DRDY_TRIGGER    (0x01)

/* Auto low power entered on GEN1 interrupt */
#define BMA400_AUTO_LP_GEN1_TRIGGER    (0x02)

/* Auto low power entered on timeout of threshold value */
#define BMA400_AUTO_LP_TIMEOUT_EN      (0x04)

/* Auto low power entered on timeout of threshold value
 * but reset on activity detection
 */
#define BMA400_AUTO_LP_TIME_RESET_EN   (0x08)

/*    TAP INTERRUPT CONFIG MACROS   */
/* Axes select for TAP interrupt */
#define BMA400_X_AXIS_EN_TAP           (0x02)
#define BMA400_Y_AXIS_EN_TAP           (0x01)
#define BMA400_Z_AXIS_EN_TAP           (0x00)

/* TAP tics_th setting */

/* Maximum time between upper and lower peak of a tap, in data samples
 * this time depends on the mechanics of the device tapped onto
 * default = 12 samples
 */

/* Configures 6 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_6_DATA_SAMPLES  (0x00)

/* Configures 9 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_9_DATA_SAMPLES  (0x01)

/* Configures 12 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_12_DATA_SAMPLES (0x02)

/* Configures 18 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_18_DATA_SAMPLES (0x03)

/* TAP Sensitivity setting */
/* It modifies the threshold for minimum TAP amplitude */
/* BMA400_TAP_SENSITIVITY_0 correspond to highest sensitivity */
#define BMA400_TAP_SENSITIVITY_0       (0x00)
#define BMA400_TAP_SENSITIVITY_1       (0x01)
#define BMA400_TAP_SENSITIVITY_2       (0x02)
#define BMA400_TAP_SENSITIVITY_3       (0x03)
#define BMA400_TAP_SENSITIVITY_4       (0x04)
#define BMA400_TAP_SENSITIVITY_5       (0x05)
#define BMA400_TAP_SENSITIVITY_6       (0x06)

/* BMA400_TAP_SENSITIVITY_7 correspond to lowest sensitivity */
#define BMA400_TAP_SENSITIVITY_7       (0x07)

/*  BMA400 TAP - quiet  settings */

/* Quiet refers to minimum quiet time before and after double tap,
 * in the data samples This time also defines the longest time interval
 * between two taps so that they are considered as double tap
 */

/* Configures 60 data samples quiet time between single or double taps */
#define BMA400_QUIET_60_DATA_SAMPLES  (0x00)

/* Configures 80 data samples quiet time between single or double taps */
#define BMA400_QUIET_80_DATA_SAMPLES  (0x01)

/* Configures 100 data samples quiet time between single or double taps */
#define BMA400_QUIET_100_DATA_SAMPLES (0x02)

/* Configures 120 data samples quiet time between single or double taps */
#define BMA400_QUIET_120_DATA_SAMPLES (0x03)

/*  BMA400 TAP - quiet_dt  settings */

/* quiet_dt refers to Minimum time between the two taps of a
 * double tap, in data samples
 */

/* Configures 4 data samples minimum time between double taps */
#define BMA400_QUIET_DT_4_DATA_SAMPLES   (0x00)

/* Configures 8 data samples minimum time between double taps */
#define BMA400_QUIET_DT_8_DATA_SAMPLES   (0x01)

/* Configures 12 data samples minimum time between double taps */
#define BMA400_QUIET_DT_12_DATA_SAMPLES  (0x02)

/* Configures 16 data samples minimum time between double taps */
#define BMA400_QUIET_DT_16_DATA_SAMPLES  (0x03)

/*    ACTIVITY CHANGE CONFIG MACROS   */
/* Data source for activity change detection */
#define BMA400_DATA_SRC_ACC_FILT1        (0x00)
#define BMA400_DATA_SRC_ACC_FILT2        (0x01)

/* Number of samples to evaluate for activity change detection */
#define BMA400_ACT_CH_SAMPLE_CNT_32      (0x00)
#define BMA400_ACT_CH_SAMPLE_CNT_64      (0x01)
#define BMA400_ACT_CH_SAMPLE_CNT_128     (0x02)
#define BMA400_ACT_CH_SAMPLE_CNT_256     (0x03)
#define BMA400_ACT_CH_SAMPLE_CNT_512     (0x04)

/* Interrupt pin configuration macros */
#define BMA400_INT_PUSH_PULL_ACTIVE_0    (0x00)
#define BMA400_INT_PUSH_PULL_ACTIVE_1    (0x01)
#define BMA400_INT_OPEN_DRIVE_ACTIVE_0   (0x02)
#define BMA400_INT_OPEN_DRIVE_ACTIVE_1   (0x03)

/* Interrupt Assertion status macros */
#define BMA400_WAKEUP_INT_ASSERTED       (0x0001)
#define BMA400_ORIENT_CH_INT_ASSERTED    (0x0002)
#define BMA400_GEN1_INT_ASSERTED         (0x0004)
#define BMA400_GEN2_INT_ASSERTED         (0x0008)
#define BMA400_INT_OVERRUN_ASSERTED      (0x0010)
#define BMA400_FIFO_FULL_INT_ASSERTED    (0x0020)
#define BMA400_FIFO_WM_INT_ASSERTED      (0x0040)
#define BMA400_DRDY_INT_ASSERTED         (0x0080)
#define BMA400_STEP_INT_ASSERTED         (0x0300)
#define BMA400_S_TAP_INT_ASSERTED        (0x0400)
#define BMA400_D_TAP_INT_ASSERTED        (0x0800)
#define BMA400_ACT_CH_X_ASSERTED         (0x2000)
#define BMA400_ACT_CH_Y_ASSERTED         (0x4000)
#define BMA400_ACT_CH_Z_ASSERTED         (0x8000)

/* Generic interrupt criterion_sel configuration macros */
#define BMA400_ACTIVITY_INT              (0x01)
#define BMA400_INACTIVITY_INT            (0x00)

/* Generic interrupt axes evaluation logic configuration macros */
#define BMA400_ALL_AXES_INT              (0x01)
#define BMA400_ANY_AXES_INT              (0x00)

/* Generic interrupt hysteresis configuration macros */
#define BMA400_HYST_0_MG                 (0x00)
#define BMA400_HYST_24_MG                (0x01)
#define BMA400_HYST_48_MG                (0x02)
#define BMA400_HYST_96_MG                (0x03)

/* BMA400 Register Address */
#define BMA400_CHIP_ID_ADDR              (0x00)
#define BMA400_STATUS_ADDR               (0x03)
#define BMA400_ACCEL_DATA_ADDR           (0x04)
#define BMA400_INT_STAT0_ADDR            (0x0E)
#define BMA400_TEMP_DATA_ADDR            (0x11)
#define BMA400_FIFO_LENGTH_ADDR          (0x12)
#define BMA400_FIFO_DATA_ADDR            (0x14)
#define BMA400_STEP_CNT_0_ADDR           (0x15)
#define BMA400_ACCEL_CONFIG_0_ADDR       (0x19)
#define BMA400_ACCEL_CONFIG_1_ADDR       (0x1A)
#define BMA400_ACCEL_CONFIG_2_ADDR       (0x1B)
#define BMA400_INT_CONF_0_ADDR           (0x1F)
#define BMA400_INT_12_IO_CTRL_ADDR       (0x24)
#define BMA400_INT_MAP_ADDR              (0x21)
#define BMA400_FIFO_CONFIG_0_ADDR        (0x26)
#define BMA400_FIFO_READ_EN_ADDR         (0x29)
#define BMA400_AUTO_LOW_POW_0_ADDR       (0x2A)
#define BMA400_AUTO_LOW_POW_1_ADDR       (0x2B)
#define BMA400_AUTOWAKEUP_0_ADDR         (0x2C)
#define BMA400_AUTOWAKEUP_1_ADDR         (0x2D)
#define BMA400_WAKEUP_INT_CONF_0_ADDR    (0x2F)
#define BMA400_ORIENTCH_INT_CONFIG_ADDR  (0x35)
#define BMA400_GEN1_INT_CONFIG_ADDR      (0x3F)
#define BMA400_GEN2_INT_CONFIG_ADDR      (0x4A)
#define BMA400_ACT_CH_CONFIG_0_ADDR      (0x55)
#define BMA400_TAP_CONFIG_ADDR           (0x57)
#define BMA400_SELF_TEST_ADDR            (0x7D)
#define BMA400_COMMAND_REG_ADDR          (0x7E)

/* BMA400 Command register */
#define BMA400_SOFT_RESET_CMD            (0xB6)
#define BMA400_FIFO_FLUSH_CMD            (0xB0)

/* BMA400 Delay definitions */
#define BMA400_SOFT_RESET_DELAY_MS       (5*1000)  /* delay in millisecond */
#define BMA400_SELF_TEST_DELAY_MS        (7*1000)  /* delay in millisecond */
#define BMA400_SELF_TEST_DATA_READ_MS    (50*1000) /* delay in millisecond */

/* Interface selection macro */
#define BMA400_SPI_WR_MASK               (0x7F)
#define BMA400_SPI_RD_MASK               (0x80)

/* UTILITY MACROS */
#define BMA400_SET_LOW_BYTE              (0x00FF)
#define BMA400_SET_HIGH_BYTE             (0xFF00)

/* Interrupt mapping selection */
#define BMA400_DATA_READY_INT_MAP        (0x01)
#define BMA400_FIFO_WM_INT_MAP           (0x02)
#define BMA400_FIFO_FULL_INT_MAP         (0x03)
#define BMA400_GEN2_INT_MAP              (0x04)
#define BMA400_GEN1_INT_MAP              (0x05)
#define BMA400_ORIENT_CH_INT_MAP         (0x06)
#define BMA400_WAKEUP_INT_MAP            (0x07)
#define BMA400_ACT_CH_INT_MAP            (0x08)
#define BMA400_TAP_INT_MAP               (0x09)
#define BMA400_STEP_INT_MAP              (0x0A)
#define BMA400_INT_OVERRUN_MAP           (0x0B)

/* BMA400 FIFO configurations */
#define BMA400_FIFO_AUTO_FLUSH           (0x01)
#define BMA400_FIFO_STOP_ON_FULL         (0x02)
#define BMA400_FIFO_TIME_EN              (0x04)
#define BMA400_FIFO_DATA_SRC             (0x08)
#define BMA400_FIFO_8_BIT_EN             (0x10)
#define BMA400_FIFO_X_EN                 (0x20)
#define BMA400_FIFO_Y_EN                 (0x40)
#define BMA400_FIFO_Z_EN                 (0x80)

/* BMA400 FIFO data configurations */
#define BMA400_FIFO_EN_X                 (0x01)
#define BMA400_FIFO_EN_Y                 (0x02)
#define BMA400_FIFO_EN_Z                 (0x04)
#define BMA400_FIFO_EN_XY                (0x03)
#define BMA400_FIFO_EN_YZ                (0x06)
#define BMA400_FIFO_EN_XZ                (0x05)
#define BMA400_FIFO_EN_XYZ               (0x07)

/* BMA400 Self test configurations */
#define BMA400_DISABLE_SELF_TEST         (0x00)
#define BMA400_ENABLE_POSITIVE_SELF_TEST (0x07)
#define BMA400_ENABLE_NEGATIVE_SELF_TEST (0x0F)

/* BMA400 FIFO data masks */
#define BMA400_FIFO_HEADER_MASK          (0x3E)
#define BMA400_FIFO_BYTES_OVERREAD       (25)
#define BMA400_AWIDTH_MASK               (0xEF)
#define BMA400_FIFO_DATA_EN_MASK         (0x0E)

/* BMA400 Step status field - Activity status */
#define BMA400_STILL_ACT                 (0x00)
#define BMA400_WALK_ACT                  (0x01)
#define BMA400_RUN_ACT                   (0x02)

/* It is inserted when FIFO_CONFIG0.fifo_data_src
 * is changed during the FIFO read
 */
#define BMA400_FIFO_CONF0_CHANGE         (0x01)

/* It is inserted when ACC_CONFIG0.filt1_bw
 * is changed during the FIFO read
 */
#define BMA400_ACCEL_CONF0_CHANGE        (0x02)

/* It is inserted when ACC_CONFIG1.acc_range
 * acc_odr or osr is changed during the FIFO read
 */
#define BMA400_ACCEL_CONF1_CHANGE        (0x04)

/* Accel width setting either 12/8 bit mode */
#define BMA400_12_BIT_FIFO_DATA          (0x01)
#define BMA400_8_BIT_FIFO_DATA           (0x00)

/* BMA400 FIFO header configurations */
#define BMA400_FIFO_SENSOR_TIME          (0xA0)
#define BMA400_FIFO_EMPTY_FRAME          (0x80)
#define BMA400_FIFO_CONTROL_FRAME        (0x48)
#define BMA400_FIFO_XYZ_ENABLE           (0x8E)
#define BMA400_FIFO_X_ENABLE             (0x82)
#define BMA400_FIFO_Y_ENABLE             (0x84)
#define BMA400_FIFO_Z_ENABLE             (0x88)
#define BMA400_FIFO_XY_ENABLE            (0x86)
#define BMA400_FIFO_YZ_ENABLE            (0x8C)
#define BMA400_FIFO_XZ_ENABLE            (0x8A)

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
#define BMA400_ST_ACC_X_AXIS_SIGNAL_DIFF (1500)
#define BMA400_ST_ACC_Y_AXIS_SIGNAL_DIFF (1200)
#define BMA400_ST_ACC_Z_AXIS_SIGNAL_DIFF (250)

/* ########################################################################## */
/*                            Enum & and Typedefs                             */
/* ########################################################################## */
typedef uint8_t (*bma400_com_fptr_t)(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t length);
typedef void (*bma400_delay_fptr_t)(uint32_t period); /* Delay function pointer */

/* Sensor selection enums */
typedef enum bma400_sensor {
  BMA400_OK=0,
  BMA400_E_COM_FAIL,
  BMA400_E_DEV_NOT_FOUND,
  BMA400_E_INVALID_CONFIG,
  BMA400_E_NULL_PTR,
  BMA400_E_FAILURE
} eBMA400_State;

/* Sensor selection enums */
enum bma400_selection {
    BMA400_ACCEL=0,
    BMA400_TAP_INT,
    BMA400_ACTIVITY_CHANGE_INT,
    BMA400_GEN1_INT,
    BMA400_GEN2_INT,
    BMA400_ORIENT_CHANGE_INT,
    BMA400_STEP_COUNTER_INT
};

/*
 * Interrupt channel selection enums
 */
enum bma400_int_chan {
    BMA400_UNMAP_INT_PIN,
    BMA400_INT_CHANNEL_1,
    BMA400_INT_CHANNEL_2,
    BMA400_MAP_BOTH_INT_PINS
};

/*
 * Interrupt pin hardware configurations
 */
struct bma400_int_pin_conf
{
    /* Interrupt channel selection enums */
    enum bma400_int_chan int_chan;

    /* Interrupt pin configuration
     * Assignable Macros :
     *  - BMA400_INT_PUSH_PULL_ACTIVE_0
     *  - BMA400_INT_PUSH_PULL_ACTIVE_1
     *  - BMA400_INT_OPEN_DRIVE_ACTIVE_0
     *  - BMA400_INT_OPEN_DRIVE_ACTIVE_1
     */
    uint8_t pin_conf;
};

/*
 * Accel basic configuration
 */
struct bma400_acc_conf
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
    enum bma400_int_chan int_chan;
};

/*
 * Tap interrupt configurations
 */
struct bma400_tap_conf
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
    enum bma400_int_chan int_chan;
};

/*
 * Activity change interrupt configurations
 */
struct bma400_act_ch_conf
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
    enum bma400_int_chan int_chan;
};

/*
 * Generic interrupt configurations
 */
struct bma400_gen_int_conf
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
    enum bma400_int_chan int_chan;
};

/*
 * Orient interrupt configurations
 */
struct bma400_orient_int_conf
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
    enum bma400_int_chan int_chan;
};

/* Step counter configurations */
struct bma400_step_int_conf
{
    /* Interrupt channel to be mapped */
    enum bma400_int_chan int_chan;
};

/*
 * Union of sensor Configurations
 */
union bma400_set_param
{
    /* Accel configurations */
    struct bma400_acc_conf accel;

    /* TAP configurations */
    struct bma400_tap_conf tap;

    /* Activity change configurations */
    struct bma400_act_ch_conf act_ch;

    /* Generic interrupt configurations */
    struct bma400_gen_int_conf gen_int;

    /* Orient configurations */
    struct bma400_orient_int_conf orient;

    /* Step counter configurations */
    struct bma400_step_int_conf step_cnt;
};

/*
 * Sensor selection and their configurations
 */
struct bma400_sensor_conf
{
    /* Sensor selection */
    enum bma400_sensor type;

    /* Sensor configuration */
    union bma400_set_param param;
};

/*
 * enum to select device settings
 */
enum bma400_device {
    BMA400_AUTOWAKEUP_TIMEOUT,
    BMA400_AUTOWAKEUP_INT,
    BMA400_AUTO_LOW_POWER,
    BMA400_INT_PIN_CONF,
    BMA400_INT_OVERRUN_CONF,
    BMA400_FIFO_CONF
};

/*
 * BMA400 auto-wakeup configurations
 */
struct bma400_auto_wakeup_conf
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
};

/*
 * BMA400 wakeup configurations
 */
struct bma400_wakeup_conf
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
    enum bma400_int_chan int_chan;
};

/*
 * BMA400 auto-low power configurations
 */
struct bma400_auto_lp_conf
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
};

/*
 * FIFO configurations
 */
struct bma400_fifo_conf
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
    enum bma400_int_chan fifo_full_channel;

    /* Interrupt pin mapping for FIFO water-mark interrupt */
    enum bma400_int_chan fifo_wm_channel;
};

/*
 * Interrupt overrun configurations
 */
struct bma400_int_overrun
{
    /* Interrupt pin mapping for interrupt overrun */
    enum bma400_int_chan int_chan;
};

/*
 * Union of device configuration parameters
 */
union bma400_device_params
{
    /* Auto wakeup configurations */
    struct bma400_auto_wakeup_conf auto_wakeup;

    /* Wakeup interrupt configurations */
    struct bma400_wakeup_conf wakeup;

    /* Auto Low power configurations */
    struct bma400_auto_lp_conf auto_lp;

    /* Interrupt pin configurations */
    struct bma400_int_pin_conf int_conf;

    /* FIFO configuration */
    struct bma400_fifo_conf fifo_conf;

    /* Interrupt overrun configuration */
    struct bma400_int_overrun overrun_int;
};

/*
 * BMA400 device configuration
 */
struct bma400_device_conf
{
    /* Device feature selection */
    enum bma400_device type;

    /* Device feature configuration */
    union bma400_device_params param;
};

/*
 * BMA400 sensor data
 */
struct bma400_sensor_data
{
    /* X-axis sensor data */
    int16_t x;

    /* Y-axis sensor data */
    int16_t y;

    /* Z-axis sensor data */
    int16_t z;

    /* sensor time */
    uint32_t sensortime;
};

/*
 * BMA400 interrupt selection
 */
enum bma400_int_type {
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
};

/*
 * Interrupt enable/disable configurations
 */
struct bma400_int_enable
{
    /* Enum to choose the interrupt to be enabled */
    enum bma400_int_type type;

    /* Enable/ disable selected interrupts
     * Assignable Macros :
     *   - BMA400_ENABLE
     *   - BMA400_DISABLE
     */
    uint8_t conf;
};
struct bma400_fifo_data
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
};

/*
 * bma400 device structure
 */
struct bma400_dev
{
    /* Chip Id */
    uint8_t chip_id;

    /* Device Id */
    uint8_t dev_id;

    /* SPI/I2C Interface selection */
    enum bma400_intf intf;

    /* Interface handle pointer */
    void *intf_ptr;

    /* Decide SPI or I2C read mechanism */
    uint8_t dummy_byte;

    /* Bus read function pointer */
    bma400_com_fptr_t read;

    /* Bus write function pointer */
    bma400_com_fptr_t write;

    /* delay(in ms) function pointer */
    bma400_delay_fptr_t delay_ms;

    /* Resolution for FOC */
    uint8_t resolution;

    /* User set read/write length */
    uint16_t read_write_len;
};

/* bma400 device structure */
typedef struct bma400_data
{
  bool start;     /* I2C address, ignored when using SPI  */
  uint8_t * pTxData; /* I2C address, ignored when using SPI  */
  uint8_t * pRxData; /* I2C address, ignored when using SPI  */
  uint8_t bufferLen; /* communication buffer length. Needs to be identical for both buffers */
} stBMA400_Data_t;

/* bma400 device structure */
typedef struct bma400_com
{
  CMU_Clock_TypeDef clock;
  GPIO_Port_TypeDef port;  /* This is assuming that both SDA and SCL are on the same port */
  uint8_t SDAPin;
  uint8_t SCLPin;
  I2C_TypeDef * pI2CIntfc; /* I2C interface pointer, null it if not used (SPI case) */
  uint8_t interrupt; /* I2C interface pointer, null it if not used (SPI case) */
  uint16_t address;        /* I2C address, ignored when using SPI  */
} stBMA400_Com_t;

/*
 * bma400 device structure
 */
typedef struct bma400_desc
{
  eBMA400_Intfc intf;        /* SPI/I2C Interface selection. SPI is TBD */
  stBMA400_Data_t * pstData;
  stBMA400_Com_t * pstCom;
} stBMA400_Desc_t;

/* ########################################################################## */
/*                                Public API                                  */
/* ########################################################################## */
/* Setup and init */
eBMA400_State bma400_Setup(void); /* a setup is a one off thing */
eBMA400_State bma400_Init(const eBMA400_Intfc eIntfc); /* an init can be called subsequently several times*/

/* Test and Reset */
int8_t bma400_soft_reset(const eBMA400_Intfc eIntfc);
int8_t bma400_perform_self_test(const eBMA400_Intfc eIntfc);
uint8_t bma400_chipid(const eBMA400_Intfc eIntfc);

/* Getters */
int8_t bma400_get_accel_data(uint8_t data_sel, struct bma400_sensor_data *accel, const eBMA400_Intfc eIntfc);
int8_t bma400_get_power_mode(uint8_t *power_mode, const eBMA400_Intfc eIntfc);
int8_t bma400_get_sensor_conf(struct bma400_sensor_conf *conf, uint16_t n_sett, const eBMA400_Intfc eIntfc);
int8_t bma400_get_fifo_data(struct bma400_fifo_data *fifo, const eBMA400_Intfc eIntfc);
int8_t bma400_get_interrupt_status(uint16_t *int_status, const eBMA400_Intfc eIntfc);
int8_t bma400_get_interrupts_enabled(struct bma400_int_enable *int_select, uint8_t n_sett, const eBMA400_Intfc eIntfc);
int8_t bma400_get_temperature_data(int16_t *temperature_data, const eBMA400_Intfc eIntfc);
int8_t bma400_get_steps_counted(uint32_t *step_count, uint8_t *activity_data, const eBMA400_Intfc eIntfc);

/* Setter */
int8_t bma400_set_power_mode(uint8_t power_mode, const eBMA400_Intfc eIntfc);
int8_t bma400_set_device_conf(const struct bma400_device_conf *conf, uint8_t n_sett, const eBMA400_Intfc eIntfc);
int8_t bma400_set_fifo_flush(const eBMA400_Intfc eIntfc);
int8_t bma400_set_step_counter_param(uint8_t *sccr_conf, const eBMA400_Intfc eIntfc);
int8_t bma400_extract_accel(struct bma400_fifo_data *fifo,
                            struct bma400_sensor_data *accel_data,
                            uint16_t *frame_count,
                            const eBMA400_Intfc eIntfc);

/* Interrupts */
void bma400_I2C_ISR_Handler(const eBMA400_Intfc eIntfc);
int8_t bma400_enable_interrupt(const struct bma400_int_enable *int_select, uint8_t n_sett, const eBMA400_Intfc eIntfc);

#endif
