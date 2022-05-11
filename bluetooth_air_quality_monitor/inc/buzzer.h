/***************************************************************************//**
 * @file buzzer.h
 * @brief Define driver structures and APIs for the magnetic buzzer
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
#ifndef BUZZER_H
#define BUZZER_H

#ifdef __cplusplus
extern "C"{
#endif

#include "buzzer_pwm.h"
#include "buzzer_pwm_config.h"

/***************************************************************************//**
 * @addtogroup Buzzer Driver
 * @brief Buzzer Driver
 * @{
 *
 * Basic example for using Buzzer driver:
 *
 * ```c
 *
 * #include "buzzer.h"
 *
 * static buzzer_t buzzer = BUZZER_INIT_DEFAULT;
 * 
 * int main( void )
 * {
 *
 *   ...
 *
 *   // Initialize Buzzer
 *   buzzer_init(&buzzer);
 *
 *   // Play a melody
 *   buzzer_play_melody(&buzzer_melody);
 *
 *   ...
 *
 * }
 * ```
 *
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    Buzzer volume settings enum
 ******************************************************************************/
typedef enum {
  buzzer_VOL0 = 0U,    /**< 0% */
  buzzer_VOL10 = 1U,   /**< 10% */
  buzzer_VOL20 = 2U,   /**< 20% */
  buzzer_VOL30 = 3U,   /**< 30% */
  buzzer_VOL40 = 4U,   /**< 40% */
  buzzer_VOL50 = 5U,   /**< 50% */
  buzzer_VOL60 = 6U,   /**< 60% */
  buzzer_VOL70 = 7U,   /**< 70% */
  buzzer_VOL80 = 8U,   /**< 80% */
  buzzer_VOL90 = 9U,   /**< 90% */
  buzzer_VOL100 = 10U  /**< 100% */
} buzzer_volume_t;

/***************************************************************************//**
 * @brief Buzzer context structure.
 *
 * @note The minimal PWM Clock frequency required at which the CMT-8540S-SMT
 * buzzer from tone C6 - 1047 Hz to above. For other buzzers, another minimal
 * frequency may be used.
 ******************************************************************************/
typedef struct
{
  buzzer_pwm_instance_t pwm;    /**< PWM instance */
  buzzer_pwm_config_t config;   /**< PWM configuration */
  buzzer_volume_t volume;       /**< buzzer volume level */
} buzzer_t;

/***************************************************************************//**
 * @brief
 *    Buzzer musical note structure
 ******************************************************************************/
typedef struct buzzer_note {
  uint16_t  note;       /**< musical note */
  uint16_t  duration;   /**< beat of the musical note */
} buzzer_note_t;

/***************************************************************************//**
 * @brief
 *    Buzzer melody structure
 ******************************************************************************/
typedef struct {
  const buzzer_note_t *melody;  /**< sequence of musical notes */
  uint16_t len;                 /**< length of an array melody */
  buzzer_t *buzzer;             /**< the instance of buzzer_t object */
} buzzer_melody_t;

/***************************************************************************//**
 * @addtogroup buzzer_cfg
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief The buzzer default PWM frequency.
 *
 * @note The frequency at which the CMT-8540S-SMT buzzer is to be driven.
 * If modifying this driver for other buzzers, another frequency may be used.
 ******************************************************************************/
#define BUZZER_FREQ  4000

#if defined (BUZZER_PWM_OUTPUT_LOC)
#define BUZZER_PWM_OUTPUT_LOCATION  BUZZER_PWM_OUTPUT_LOC
#else
#define  BUZZER_PWM_OUTPUT_LOCATION  0 // 0 for compatibility
#endif

/***************************************************************************//**
 * @brief
 *    Default struct for PWM initialization
 ******************************************************************************/
#define PWM_INSTANCE_DEFAULT                                              \
  {                                                                       \
    BUZZER_PWM_PERIPHERAL,                   /* PWM peripheral */             \
    (uint8_t)BUZZER_PWM_OUTPUT_CHANNEL,      /* PWM channel */                \
    (uint8_t)BUZZER_PWM_OUTPUT_PORT,         /* PWM output port */            \
    (uint8_t)BUZZER_PWM_OUTPUT_PIN,          /* PWM output pin */             \
    (uint8_t)BUZZER_PWM_OUTPUT_LOCATION,     /* PWM output location */        \
  }

/***************************************************************************//**
 * @brief
 *    Default struct for PWM configuration
 ******************************************************************************/
#define PWM_CFG_DEFAULT                                                   \
  {                                                                       \
    BUZZER_FREQ,                         /* PWM frequency */              \
    BUZZER_PWM_POLARITY,                 /* PWM Polarity */               \
  }

/***************************************************************************//**
 * @brief
 *    Default init struct for buzzer initialization
 ******************************************************************************/
#define BUZZER_INIT_DEFAULT                                               \
  {                                                                       \
    PWM_INSTANCE_DEFAULT,                /* PWM instance */               \
    PWM_CFG_DEFAULT,                     /* PWM config */                 \
    buzzer_VOL0,                         /* buzzer volume */              \
  }

/** @} (end addtogroup buzzer_cfg) */

/***************************************************************************//**
 * @addtogroup buzzer_freq
 * @details Setting frequencies for the buzzer
 * @{
 ******************************************************************************/
#define BUZZER_NOTE_C6              1047
#define BUZZER_NOTE_Db6             1109
#define BUZZER_NOTE_D6              1175
#define BUZZER_NOTE_Eb6             1245
#define BUZZER_NOTE_E6              1319
#define BUZZER_NOTE_F6              1397
#define BUZZER_NOTE_Gb6             1480
#define BUZZER_NOTE_G6              1568
#define BUZZER_NOTE_Ab6             1661
#define BUZZER_NOTE_A6              1760
#define BUZZER_NOTE_Bb6             1865
#define BUZZER_NOTE_B6              1976
#define BUZZER_NOTE_C7              2093
#define BUZZER_NOTE_Db7             2217
#define BUZZER_NOTE_D7              2349
#define BUZZER_NOTE_Eb7             2489
#define BUZZER_NOTE_E7              2637
#define BUZZER_NOTE_F7              2794
#define BUZZER_NOTE_Gb7             2960
#define BUZZER_NOTE_G7              3136
#define BUZZER_NOTE_Ab7             3322
#define BUZZER_NOTE_A7              3520
#define BUZZER_NOTE_Bb7             3729
#define BUZZER_NOTE_B7              3951
#define BUZZER_NOTE_C8              4186
#define BUZZER_NOTE_Db8             4435
#define BUZZER_NOTE_D8              4699
#define BUZZER_NOTE_Eb8             4978

// Note rest
#define BUZZER_NOTE_REST            1000

// End of melody marker
#define BUZZER_END_MELODY           {BUZZER_NOTE_REST, 0x07D0}

/** @} (end addtogroup buzzer_freq) */

/***************************************************************************//**
 * @addtogroup buzzer driver
 * @brief API for configuring and manipulating buzzer driver
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief 
 *  Initializes all necessary pins and peripherals used for the buzzer.
 *
 * @param[in] buzzer
 *  The instance of buzzer_t.
 *  See #buzzer_t object definition for the detailed explanation.
 *
 * @return
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER Pointer to NULL.
 ******************************************************************************/
sl_status_t buzzer_init(buzzer_t *buzzer);

/***************************************************************************//**
 * @brief
 *  Deinitializes all necessary pins and peripherals used for the buzzer.
 *
 * @param[in] buzzer
 *  The instance of buzzer_t.
 *  See #buzzer_t object definition for the detailed explanation.
 *
 * @return
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER Pointer to NULL.
 ******************************************************************************/
sl_status_t buzzer_deinit(buzzer_t *buzzer);

/***************************************************************************//**
 * @brief
 *  Sets the buzzer volume level in percentages (range[0..100%]).
 *
 * @param[in] buzzer
 *  The instance of buzzer_t.
 *  See #buzzer_t object definition for the detailed explanation.
 * 
 * @param[in] volume
 *  The instance of buzzer_volume_t.
 *  Buzzer sound volume.
 *   - <b>minimum :</b> buzzer_VOL0
 *   - <b>maximum :</b> buzzer_VOL100
 *  See #buzzer_volume_t object definition for the detailed explanation.
 *
 * @return
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER  Pointer to NULL.
 ******************************************************************************/
sl_status_t buzzer_set_volume(buzzer_t *buzzer, buzzer_volume_t volume);

/***************************************************************************//**
 * @brief
 *  Gets the buzzer volume level in percentages (range[0..100%]).
 *
 * @param[in] buzzer
 *  The instance of buzzer_t.
 *  See #buzzer_t object definition for the detailed explanation.
 * 
 * @param[out] volume
 *  The instance of buzzer_volume_t.
 *  Buzzer sound volume.
 *   - <b>minimum :</b> buzzer_VOL0
 *   - <b>maximum :</b> buzzer_VOL100
 *  See #buzzer_volume_t object definition for the detailed explanation.
 *
 * @return
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER  Pointer to NULL.
 ******************************************************************************/
sl_status_t buzzer_get_volume(buzzer_t *buzzer, buzzer_volume_t *volume);

/***************************************************************************//**
 * @brief
 *  Plays a sound on the buzzer.
 *
 * @param[in] buzzer
 *  The instance of buzzer_t.
 *  See buzzer_t object definition for the detailed explanation.
 * 
 * @param[in] freq
 *  Buzzer sound frequency.
 * 
 * @param[in] duration
 *  Buzzer sound duration in milliseconds.
 *
 * @return 
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER  Pointer to NULL.
 *  SL_STATUS_INVALID_RANGE Out of range.
 ******************************************************************************/
sl_status_t buzzer_play_sound(buzzer_t *buzzer,
                              uint16_t freq,
                              uint16_t duration);

/***************************************************************************//**
 * @brief
 *  Plays a melody on the buzzer.
 *
 * @param[in] buzzer
 *  The instance of buzzer_melody_t.
 *  See #buzzer_melody_t object definition for the detailed explanation.
 *
 * @return
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER  Pointer to NULL.
 ******************************************************************************/
sl_status_t buzzer_play_melody(buzzer_melody_t *melody);

/***************************************************************************//**
 * @brief
 *  Ends playing a sound.
 *
 * @param[in] buzzer
 *  The instance of buzzer_t.
 *  See #buzzer_t object definition for the detailed explanation.
 *
 * @return
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER  Pointer to NULL.
 ******************************************************************************/
sl_status_t buzzer_end_sound(buzzer_t *buzzer);

/***************************************************************************//**
 * @brief
 *  Begins playing a sound.
 *
 * @param[in] buzzer
 *  The instance of buzzer_t.
 *  See #buzzer_t object definition for the detailed explanation.
 * 
 * @param[in] freq
 *  Buzzer sound frequency.
 *
 * @return
 *  SL_STATUS_OK if there are no errors.
 *  SL_STATUS_NULL_POINTER Pointer to NULL.
 ******************************************************************************/
sl_status_t buzzer_begin_sound(buzzer_t *buzzer, uint16_t freq);

/** @} (end addtogroup buzzer driver) */

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
