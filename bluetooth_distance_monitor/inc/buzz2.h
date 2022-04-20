/***************************************************************************//**
* @file buzz2.h
* @brief define driver structures and APIs for the buzzer
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

#ifndef __BUZZ2_H
#define __BUZZ2_H

#ifdef __cplusplus
extern "C"{
#endif

#include "sl_pwm.h"
#include "sl_sleeptimer.h"

/***************************************************************************//**
 * @addtogroup buzz2_cfg
 * @{
 ******************************************************************************/

/**
 * @brief BUZZ 2 default PWM frequency.
 *
 * @details Specified setting for setting default PWM frequency of BUZZ 2 Click
 * driver.
 */
#define BUZZ2_DEF_FREQ  5000

/** @} (end addtogroup buzz2_cfg) */

/***************************************************************************//**
 * @addtogroup buzz2_freq
 * @details Setting frequencies for BUZZ 2 Click.
 * @{
 ******************************************************************************/
#define BUZZ2_NOTE_C2                65
#define BUZZ2_NOTE_Db2               69
#define BUZZ2_NOTE_D2                73
#define BUZZ2_NOTE_Eb2               78
#define BUZZ2_NOTE_E2                82
#define BUZZ2_NOTE_F2                87
#define BUZZ2_NOTE_Gb2               93
#define BUZZ2_NOTE_G2                98
#define BUZZ2_NOTE_Ab2              104
#define BUZZ2_NOTE_A2               110
#define BUZZ2_NOTE_Bb2              117
#define BUZZ2_NOTE_B2               123
#define BUZZ2_NOTE_C3               131
#define BUZZ2_NOTE_Db3              139
#define BUZZ2_NOTE_D3               147
#define BUZZ2_NOTE_Eb3              156
#define BUZZ2_NOTE_E3               165
#define BUZZ2_NOTE_F3               175
#define BUZZ2_NOTE_Gb3              185
#define BUZZ2_NOTE_G3               196
#define BUZZ2_NOTE_Ab3              208
#define BUZZ2_NOTE_A3               220
#define BUZZ2_NOTE_AS3              233
#define BUZZ2_NOTE_B3               247
#define BUZZ2_NOTE_C4               262
#define BUZZ2_NOTE_Db4              277
#define BUZZ2_NOTE_D4               294
#define BUZZ2_NOTE_Eb4              311
#define BUZZ2_NOTE_E4               330
#define BUZZ2_NOTE_F4               349
#define BUZZ2_NOTE_Gb4              370
#define BUZZ2_NOTE_G4               392
#define BUZZ2_NOTE_Ab4              415
#define BUZZ2_NOTE_A4               440
#define BUZZ2_NOTE_Bb4              466
#define BUZZ2_NOTE_B4               494
#define BUZZ2_NOTE_C5               523
#define BUZZ2_NOTE_Db5              554
#define BUZZ2_NOTE_D5               587
#define BUZZ2_NOTE_Eb5              622
#define BUZZ2_NOTE_E5               659
#define BUZZ2_NOTE_F5               698
#define BUZZ2_NOTE_Gb5              740
#define BUZZ2_NOTE_G5               784
#define BUZZ2_NOTE_Ab5              831
#define BUZZ2_NOTE_A5               880
#define BUZZ2_NOTE_Bb5              932
#define BUZZ2_NOTE_B5               988
#define BUZZ2_NOTE_C6              1047
#define BUZZ2_NOTE_Db6             1109
#define BUZZ2_NOTE_D6              1175
#define BUZZ2_NOTE_Eb6             1245
#define BUZZ2_NOTE_E6              1319
#define BUZZ2_NOTE_F6              1397
#define BUZZ2_NOTE_Gb6             1480
#define BUZZ2_NOTE_G6              1568
#define BUZZ2_NOTE_Ab6             1661
#define BUZZ2_NOTE_A6              1760
#define BUZZ2_NOTE_Bb6             1865
#define BUZZ2_NOTE_B6              1976
#define BUZZ2_NOTE_C7              2093
#define BUZZ2_NOTE_Db7             2217
#define BUZZ2_NOTE_D7              2349
#define BUZZ2_NOTE_Eb7             2489
#define BUZZ2_NOTE_E7              2637
#define BUZZ2_NOTE_F7              2794
#define BUZZ2_NOTE_Gb7             2960
#define BUZZ2_NOTE_G7              3136
#define BUZZ2_NOTE_Ab7             3322
#define BUZZ2_NOTE_A7              3520
#define BUZZ2_NOTE_Bb7             3729
#define BUZZ2_NOTE_B7              3951
#define BUZZ2_NOTE_C8              4186
#define BUZZ2_NOTE_Db8             4435
#define BUZZ2_NOTE_D8              4699
#define BUZZ2_NOTE_Eb8             4978
/** @} (end addtogroup buzz2_freq) */

/**
 * @brief BUZZ 2 Click context object.
 *
 * @details Context object definition of BUZZ 2 Click driver.
 *
 * @note The minimal PWM Clock frequency required for this example is
 * the frequency of tone C6 - 1047 Hz.
 */
typedef struct
{

  sl_pwm_instance_t pwm;  /**< PWM driver object. */

  sl_pwm_config_t config;  /**< PWM configuration object. */

  uint32_t pwm_freq;  /**< PWM frequency value. */

} buzz2_t;

/**
 * @brief BUZZ 2 Click configuration object.
 *
 * @details Configuration object definition of BUZZ 2 Click driver.
 *
 * @note The minimal PWM Clock frequency required for this example is
 * the frequency of tone C6 - 1047 Hz.
 */
typedef struct
{
  uint32_t dev_pwm_freq;  /**< PWM frequency value. */

} buzz2_cfg_t;

/***************************************************************************//**
 * @addtogroup buzz2 BUZZ 2 Click Driver
 * @brief API for configuring and manipulating BUZZ 2 Click driver
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief BUZZ 2 configuration object setup function.
 *
 * @details This function initializes click configuration structure to initial
 * values.
 *
 * @param[out] cfg
 *   The instance of buzz2_cfg_t.
 * See #buzz2_cfg_t object definition for detailed explanation.
 *
 * @return SL_STATUS_OK successful.
 ******************************************************************************/
sl_status_t buzz2_cfg_setup(buzz2_cfg_t *cfg);

/***************************************************************************//**
 * @brief BUZZ 2 initialization function.
 *
 * @details This function initializes all necessary pins and peripherals used
 * for this click board.
 *
 * @param[out] buzz2
 *   The instance of buzz2_t.
 * See #buzz2_t object definition for detailed explanation.
 * @param[in] cfg
 *   The instance of buzz2_cfg_t.
 * See #buzz2_cfg_t object definition for detailed explanation.
 *
 * @return SL_STATUS_OK Successful.
 ******************************************************************************/
sl_status_t buzz2_init(buzz2_t *buzz2, buzz2_cfg_t *cfg);

/***************************************************************************//**
 * @brief BUZZ 2 sets PWM duty cycle.
 *
 * @details This function sets the PWM duty cycle in percentages (Range[0..9]).
 *
 * @param[out] buzz2
 *   The instance of buzz2_t.
 * See #buzz2_t object definition for detailed explanation.
 * @param[in] duty_cycle
 *   PWM duty cycle.
 *
 * @return SL_STATUS_OK successful.
 ******************************************************************************/
sl_status_t buzz2_set_duty_cycle(buzz2_t *buzz2, uint8_t duty_cycle);

/***************************************************************************//**
 * @brief Play sound function.
 *
 * @details This function plays sound on buzzer.
 *
 * @param[out] buzz2
 *   The instance of buzz2_t.
 * @param[in] freq
 *   Buzz sound frequency.
 * @param[in] volume
 *   Buzz sound volume.
 *    - <b>minimum :</b> 0
 *    - <b>maximum :</b> 9
 * @param[in] duration
 *   Buzz sound duration in milliseconds.
 *
 * @return SL_STATUS_OK Successful.
 ******************************************************************************/
sl_status_t buzz2_play_sound(buzz2_t *buzz2,
                             uint16_t freq,
                             uint16_t volume,
                             uint16_t duration);

/***************************************************************************//**
 * @brief BUZZ 2 stop PWM module.
 *
 * @details This function stops the PWM module output.
 *
 * @param[out] buzz2
 *   The instance of buzz2_t.
 * See #buzz2_t object definition for detailed explanation.
 *
 * @return SL_STATUS_OK Successful.
 ******************************************************************************/
sl_status_t buzz2_pwm_stop(buzz2_t *buzz2);

/***************************************************************************//**
 * @brief BUZZ 2 start PWM module.
 *
 * @details This function starts the PWM module output.
 *
 * @param[out] buzz2
 *   The instance of buzz2_t.
 * See #buzz2_t object definition for detailed explanation.
 *
 * @return SL_STATUS_OK Successful.
 ******************************************************************************/
sl_status_t buzz2_pwm_start(buzz2_t *buzz2);

/** @} (end addtogroup buzz2) */

#ifdef __cplusplus
}
#endif

#endif // __BUZZ2_H
