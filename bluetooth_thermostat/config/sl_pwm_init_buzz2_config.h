/***************************************************************************//**
 * @file
 * @brief PWM Driver
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef PWM_INIT_BUZZ2_CONFIG_H
#define PWM_INIT_BUZZ2_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

// <<< Use Configuration Wizard in Context Menu >>>

// <h>PWM configuration

// <o SL_PWM_BUZZ2_FREQUENCY> PWM frequency [Hz]
// <i> Default: 10000
#define SL_PWM_BUZZ2_FREQUENCY       10000

// <o SL_PWM_BUZZ2_POLARITY> Polarity
// <PWM_ACTIVE_HIGH=> Active high
// <PWM_ACTIVE_LOW=> Active low
// <i> Default: PWM_ACTIVE_HIGH
#define SL_PWM_BUZZ2_POLARITY        PWM_ACTIVE_HIGH
// </h> end pwm configuration

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>

// <timer channel=OUTPUT> SL_PWM_BUZZ2
// $[TIMER_SL_PWM_BUZZ2]
#define SL_PWM_BUZZ2_PERIPHERAL                  TIMER4
#define SL_PWM_BUZZ2_PERIPHERAL_NO               4

#define SL_PWM_BUZZ2_OUTPUT_CHANNEL              0
// TIMER4 CC0 on PB03
#define SL_PWM_BUZZ2_OUTPUT_PORT                 gpioPortB
#define SL_PWM_BUZZ2_OUTPUT_PIN                  3
// [TIMER_SL_PWM_BUZZ2]$

// <<< sl:end pin_tool >>>

#ifdef __cplusplus
}
#endif

#endif // PWM_INIT_BUZZ2_CONFIG_H
