/***************************************************************************//**
 * @file buzzer_pwm_config.h
 * @brief PWM config
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef BUZZER_PWM_CONFIG_H
#define BUZZER_PWM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// PWM frequency [Hz]
#define BUZZER_PWM_FREQUENCY                    10000

/*
 * Polarity
 * - <PWM_ACTIVE_HIGH=> Active high
 * - <PWM_ACTIVE_LOW=> Active low
 * Default: PWM_ACTIVE_HIGH
 */
#define BUZZER_PWM_POLARITY                     PWM_ACTIVE_HIGH

#define BUZZER_PWM_PERIPHERAL                   WTIMER0
#define BUZZER_PWM_PERIPHERAL_NO                0
#define BUZZER_PWM_OUTPUT_CHANNEL               0

// WTIMER0 CC0 on PA7
#define BUZZER_PWM_OUTPUT_PORT                  gpioPortA
#define BUZZER_PWM_OUTPUT_PIN                   7
#define BUZZER_PWM_OUTPUT_LOC                   7

#ifdef __cplusplus
}
#endif

#endif // BUZZER_PWM_CONFIG_H
