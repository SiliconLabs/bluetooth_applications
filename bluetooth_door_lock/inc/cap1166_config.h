/***************************************************************************//**
* @file cap1166_config.h
* @brief
* define the default configuration for the cap1166 Capacitive Touch Sensor
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
#ifndef __SL_CAP1166_CONFIG_H__
#define __SL_CAP1166_CONFIG_H__

#include "cap1166_regs.h"

/* Default configuration for touch sensor */
#define CAP11666_DEFAULT_CONFIG                     \
  {                                                 \
    CAP11666_INIT_SENSOR_INPUT,                     \
    CAP11666_INIT_SENSING_CYCLE,                    \
    CAP11666_INIT_SEN_CTR,                          \
    CAP11666_INIT_NOISE,                            \
    CAP11666_INIT_RECALIB,                          \
    CAP11666_INIT_PATTERN,                          \
    CAP11666_INIT_PROXIMITY,                        \
    CAP11666_INIT_PRESS_AND_HOLD,                   \
    CAP11666_INIT_MUL_TOUCH,                        \
    CAP1166_LED_DEFAULT_CONFIG,                     \
    CAP1166_ALERT_ACTIVE_HIGH,                      \
    CAP1166_ACTIVE,                                 \
    CAP1166_GAIN_1,                                 \
  }                                                 \

/* Default initialization structure for sensor_inputs */
#define CAP11666_INIT_SENSOR_INPUT                  \
  {                                                 \
    /* sensor inputs  */                            \
    {                                               \
      CAP1166_SENSOR_INPUT_ENABLE,                  \
      CAP1166_SENSOR_INPUT_ENABLE,                  \
      CAP1166_SENSOR_INPUT_ENABLE,                  \
      CAP1166_SENSOR_INPUT_ENABLE,                  \
      CAP1166_SENSOR_INPUT_ENABLE,                  \
      CAP1166_SENSOR_INPUT_ENABLE,                  \
    },                                              \
    /* sensor inputs interrupt */                   \
    {                                               \
      CAP1166_SENSOR_INTERUPT_ENABLE,               \
      CAP1166_SENSOR_INTERUPT_ENABLE,               \
      CAP1166_SENSOR_INTERUPT_ENABLE,               \
      CAP1166_SENSOR_INTERUPT_ENABLE,               \
      CAP1166_SENSOR_INTERUPT_ENABLE,               \
      CAP1166_SENSOR_INTERUPT_ENABLE,               \
    },                                              \
    /* sensor inputs threshold active mode*/        \
    {                                               \
      CAP1166_SENS_IN_THRESHOLD_64X,                \
      CAP1166_SENS_IN_THRESHOLD_64X,                \
      CAP1166_SENS_IN_THRESHOLD_64X,                \
      CAP1166_SENS_IN_THRESHOLD_64X,                \
      CAP1166_SENS_IN_THRESHOLD_64X,                \
      CAP1166_SENS_IN_THRESHOLD_64X,                \
    },                                              \
    /* sensor inputs threshold standby mode*/       \
    CAP1166_STBY_THRESHOLD_64X,                     \
    /* sensor input repeat rate */                  \
    {                                               \
      CAP1166_SENSOR_REPEAT_ENABLE,                 \
      CAP1166_SENSOR_REPEAT_ENABLE,                 \
      CAP1166_SENSOR_REPEAT_ENABLE,                 \
      CAP1166_SENSOR_REPEAT_ENABLE,                 \
      CAP1166_SENSOR_REPEAT_ENABLE,                 \
      CAP1166_SENSOR_REPEAT_ENABLE,                 \
    },                                              \
  }

/* Default initialization structure for re-calibration */
#define CAP11666_INIT_RECALIB                       \
  {                                                 \
    CAP1166_AUTO_CALIB,                             \
    {                                               \
      CAP1166_SENSOR_RECALIB_ENABLE,                \
      CAP1166_SENSOR_RECALIB_ENABLE,                \
      CAP1166_SENSOR_RECALIB_ENABLE,                \
      CAP1166_SENSOR_RECALIB_ENABLE,                \
      CAP1166_SENSOR_RECALIB_ENABLE,                \
      CAP1166_SENSOR_RECALIB_ENABLE,                \
    },                                              \
      CAP1166_MAX_DUR_RECALIB_EN,                   \
      CAP1166_5600MILISEC_BEFORE_RECALIB,           \
      CAP1166_CONS_NEG_DELTA_CNT_16,                \
      CAP1166_REC_SAMPLES_64_UPDT_TIME_64,          \
  }

/* Default initialization structure for sensing cycle */
#define CAP11666_INIT_SENSING_CYCLE                \
  {                                                \
    CAP1166_8_SAMPLES,                             \
    CAP1166_SAMPLE_TIME_1280MICROSEC,              \
    CAP1166_CYCLE_TIME_70MILISEC,                  \
  }

/* Default initialization structure for multiple touch detection */
#define CAP11666_INIT_MUL_TOUCH                    \
  {                                                \
    CAP1166_SIMUL_TOUCH_NUM_3,                     \
    CAP1166_MULTIPLE_BLOCK_EN,                     \
  }

/* Default initialization structure for sensitivity */
#define CAP11666_INIT_SEN_CTR                      \
  {                                                \
    /* sensitivity in active mode */               \
    CAP1166_SENSITIVITY_MULTIPLIER_32X,            \
    CAP1166_DATA_SCALING_FACTOR_256X,              \
    /* sensitivity in standby mode */              \
    CAP1166_STBY_SENSE_MULTIPLIER_32X,             \
  }

/* Default initialization structure for noise */
#define CAP11666_INIT_NOISE                        \
  {                                                \
    CAP1166_SENS_IN_THRESHOLD_50_PERCENT,          \
    CAP1166_DIG_NOISE_THRESHOLD_EN,                \
    CAP1166_AN_NOISE_FILTER_EN,                    \
    CAP1166_RF_NOISE_FILTER_EN,                    \
    CAP1166_SHOW_LOW_FREQ_NOISE,                   \
  }

/* Default initialization structure for noise */
#define CAP11666_INIT_PRESS_AND_HOLD               \
  {                                                \
    CAP1166_DETECT_RELEASE_EN,                     \
    CAP1166_INTERR_REPEAT_RATE_175MILISEC,         \
    CAP1166_PRESS_AND_HOLD_EVENT_AFTER_280MILISEC, \
  }

/* Default initialization structure for proximity detection mode */
#define CAP11666_INIT_PROXIMITY                   \
  {                                               \
    {                                             \
        CAP1166_STBY_AVERAGE_64,                  \
        CAP1166_STBY_SAMPLE_TIME_320US,           \
        CAP1166_STBY_CYCLE_TIME_35MS,             \
    },                                            \
        CAP1166_SUMMATION_BASED,                  \
  }

/* Default initialization structure for pattern detection mode */
#define CAP11666_INIT_PATTERN                     \
  {                                               \
    CAP1166_MULTIPLE_THRESHOLD_100_PERCENTS,      \
    CAP1166_MODE_PATTERN_RECOGNITION,             \
    CAP1166_MLTP_EVENT_ALERT_EN,                  \
    /* sensor input enable in MTP */              \
    {                                             \
        CAP1166_SENSOR_PATTERN_DISABLE,           \
        CAP1166_SENSOR_PATTERN_DISABLE,           \
        CAP1166_SENSOR_PATTERN_ENABLE,            \
        CAP1166_SENSOR_PATTERN_ENABLE,            \
        CAP1166_SENSOR_PATTERN_DISABLE,           \
        CAP1166_SENSOR_PATTERN_DISABLE,           \
    }                                             \
  }

/* Default initialization structure for led */
#define CAP1166_LED_DEFAULT_CONFIG               \
{                                                \
  {                                              \
      CAP1166_OUTPUT_OPEN_DRAIN,                 \
      CAP1166_OUTPUT_OPEN_DRAIN,                 \
      CAP1166_OUTPUT_OPEN_DRAIN,                 \
      CAP1166_OUTPUT_OPEN_DRAIN,                 \
      CAP1166_OUTPUT_OPEN_DRAIN,                 \
      CAP1166_OUTPUT_OPEN_DRAIN,                 \
  },                                             \
  {                                              \
      CAP1166_OUTPUT_LINK_TO_SENSOR,             \
      CAP1166_OUTPUT_LINK_TO_SENSOR,             \
      CAP1166_OUTPUT_LINK_TO_SENSOR,             \
      CAP1166_OUTPUT_LINK_TO_SENSOR,             \
      CAP1166_OUTPUT_LINK_TO_SENSOR,             \
      CAP1166_OUTPUT_LINK_TO_SENSOR,             \
  },                                             \
  {                                              \
      CAP1166_OUTPUT_DIRECT_MODE,                \
      CAP1166_OUTPUT_DIRECT_MODE,                \
      CAP1166_OUTPUT_DIRECT_MODE,                \
      CAP1166_OUTPUT_DIRECT_MODE,                \
      CAP1166_OUTPUT_DIRECT_MODE,                \
      CAP1166_OUTPUT_DIRECT_MODE,                \
  },                                             \
  {                                              \
    CAP1166_OUTPUT_INVERTED,                     \
    CAP1166_OUTPUT_INVERTED,                     \
    CAP1166_OUTPUT_INVERTED,                     \
    CAP1166_OUTPUT_INVERTED,                     \
    CAP1166_OUTPUT_INVERTED,                     \
    CAP1166_OUTPUT_INVERTED,                     \
  },                                             \
  {                                              \
      CAP1166_OUTPUT_DO_NOT_MIRROR,              \
      CAP1166_OUTPUT_DO_NOT_MIRROR,              \
      CAP1166_OUTPUT_DO_NOT_MIRROR,              \
      CAP1166_OUTPUT_DO_NOT_MIRROR,              \
      CAP1166_OUTPUT_DO_NOT_MIRROR,              \
      CAP1166_OUTPUT_DO_NOT_MIRROR,              \
  },                                             \
  CAP1166_ALERT_NOT_ASSERTED_ON_LED_ACT,         \
  CAP1166_LED_PULSE_ON_TOUCH,                    \
  /* pulse 1 period */                           \
  0x20,                                          \
  CAP1166_PULSE1_3_PULSES,                       \
  CAP1166_MAX_DUTY_100_PERCENTS,                 \
  CAP1166_MIN_DUTY_0_PERCENT,                    \
  /* pulse 2 period */                           \
  0x0f,                                          \
  CAP1166_PULSE2_3_PULSES,                       \
  CAP1166_MAX_DUTY_100_PERCENTS,                 \
  CAP1166_MIN_DUTY_0_PERCENT,                    \
  /* breathe period */                           \
  0x5D,                                          \
  CAP1166_MAX_DUTY_100_PERCENTS,                 \
  CAP1166_MIN_DUTY_0_PERCENT,                    \
  CAP1166_BR_OFF_DLY_0MS,                        \
  CAP1166_RISE_RATE_0MS,                         \
  CAP1166_FALL_RATE_0MS,                         \
  CAP1166_MAX_DUTY_100_PERCENTS,                 \
  CAP1166_MIN_DUTY_0_PERCENT,                    \
  CAP1166_DR_OFF_DLY_0MS,                        \
}

#endif /* __SL_CAP1166_CONFIG_H__ */
