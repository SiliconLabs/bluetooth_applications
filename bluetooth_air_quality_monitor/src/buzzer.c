/***************************************************************************//**
 * @file buzzer.c
 * @brief Driver for the magnetic buzzer
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
#include "buzzer.h"
#include "sl_sleeptimer.h"

// timer handle
static sl_sleeptimer_timer_handle_t timer_play_sound_handle;
static sl_sleeptimer_timer_handle_t timer_play_melody_handle;

// buzzer callback function
static void timer_buzzer_cb(sl_sleeptimer_timer_handle_t *handle, void *data);

/***************************************************************************//**
 * @addtogroup buzzer driver
 * @brief  buzzer driver.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 *  Initializes the buzzer
 ******************************************************************************/
sl_status_t buzzer_init(buzzer_t *buzzer)
{
  sl_status_t retval = SL_STATUS_OK;

  if(buzzer == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  // initializes PWM
  retval = buzzer_pwm_init(&buzzer->pwm , &buzzer->config);
  if(retval != SL_STATUS_OK){
    return retval;
  }

  // sets buzzer volume level
  retval = buzzer_set_volume(buzzer, buzzer->volume);

  // starts PWM
  buzzer_pwm_start(&buzzer->pwm);

  return retval;
}

/***************************************************************************//**
 *  Deinitializes the buzzer
 ******************************************************************************/
sl_status_t buzzer_deinit(buzzer_t *buzzer)
{
  sl_status_t retval = SL_STATUS_OK;

  if(buzzer == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  // stops PWM
  buzzer_pwm_stop(&buzzer->pwm);

  // deinits PWM
  buzzer_pwm_deinit(&buzzer->pwm);

  return retval;
}

/***************************************************************************//**
 *  Sets the buzzer volume
 ******************************************************************************/
sl_status_t buzzer_set_volume(buzzer_t *buzzer, buzzer_volume_t volume)
{
  if(buzzer == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  buzzer->volume = volume;

  return SL_STATUS_OK;
}

/***************************************************************************//**
 *  Gets the buzzer volume
 ******************************************************************************/
sl_status_t buzzer_get_volume(buzzer_t *buzzer, buzzer_volume_t *volume)
{
  if(buzzer == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  *volume = buzzer->volume;

  return SL_STATUS_OK;
}

/***************************************************************************//**
 *  Stops a musical note
 ******************************************************************************/
sl_status_t buzzer_end_sound(buzzer_t *buzzer)
{
  sl_status_t retval = SL_STATUS_OK;

  if(buzzer == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  buzzer_pwm_set_duty_cycle(&buzzer->pwm, buzzer_VOL0);

  return retval;
}

/***************************************************************************//**
 *  Starts a musical note
 ******************************************************************************/
sl_status_t buzzer_begin_sound(buzzer_t *buzzer, uint16_t freq)
{
  sl_status_t retval = SL_STATUS_OK;

  if(buzzer == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  // sets frequency
  buzzer_pwm_set_frequency(&buzzer->pwm, freq);

  // sets duty cycle
  buzzer_pwm_set_duty_cycle(&buzzer->pwm, buzzer->volume);

  return retval;
}

/***************************************************************************//**
 *  Plays a sound
 ******************************************************************************/
sl_status_t buzzer_play_sound(buzzer_t *buzzer,
                               uint16_t freq,
                               uint16_t duration)
{
  sl_status_t retval = SL_STATUS_OK;

  if(buzzer == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  if(freq > BUZZER_NOTE_Eb8 || freq < BUZZER_NOTE_C6){
    return SL_STATUS_INVALID_RANGE;
  }

  // begins playing a sound
  buzzer_begin_sound(buzzer, freq);

  retval = sl_sleeptimer_start_timer_ms(&timer_play_sound_handle,
                                        duration,
                                        timer_buzzer_cb,
                                        (buzzer_t*)buzzer,
                                        1,
                                        0);

  return retval;
}

/***************************************************************************//**
 *  Plays a melody
 ******************************************************************************/
sl_status_t buzzer_play_melody(buzzer_melody_t *buzzer_melody)
{
  sl_status_t retval = SL_STATUS_OK;

  if(buzzer_melody == NULL){
    return SL_STATUS_NULL_POINTER;
  }

  retval = sl_sleeptimer_start_timer_ms(&timer_play_melody_handle,
                                        buzzer_melody->melody->duration,
                                        timer_buzzer_cb,
                                        (buzzer_melody_t*)buzzer_melody,
                                        0,
                                        0);

  return retval;
}

/***************************************************************************//**
 *  Parses the instance of buzzer_melody_t data
 ******************************************************************************/
static sl_status_t buzzer_parse_data(buzzer_melody_t *buzzer_melody,
                                     buzzer_t *buzzer,
                                     uint16_t *note,
                                     uint16_t *duration)
{
  if((buzzer_melody == NULL) ||
     (buzzer == NULL) ||
     (note == NULL) ||
     (duration == NULL)){
    return SL_STATUS_NULL_POINTER;
  }

  *buzzer = *(buzzer_melody->buzzer);
  *note = buzzer_melody->melody->note;
  *duration = buzzer_melody->melody->duration;

  if(BUZZER_NOTE_REST == buzzer_melody->melody->note){
      buzzer_melody->melody -= buzzer_melody->len;
      buzzer_melody->len = 0;

    return SL_STATUS_EMPTY;
  }

  buzzer_melody->melody++;
  buzzer_melody->len++;

  return SL_STATUS_OK;
}

/***************************************************************************//**
 *  Callback when the sleep timer expire
 ******************************************************************************/
static void timer_buzzer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  uint16_t note, duration, retval;
  static buzzer_t buzzer;

  if((data == NULL) || (handle == NULL)){
    return;
  }

  if(handle == &timer_play_sound_handle){
    // end playing a sound
    buzzer_end_sound((buzzer_t*)data);
  }

  if(handle == &timer_play_melody_handle){
    retval = buzzer_parse_data((buzzer_melody_t*)data,
                                &buzzer,
                                &note,
                                &duration);

    if(retval != SL_STATUS_OK){
      // stop melody
      buzzer_end_sound(&buzzer);

      return;
    }

    // begin playing a sound
    buzzer_begin_sound(&buzzer, note);

    // new callback(), schedule the next note in this melody
    retval = sl_sleeptimer_start_timer_ms(&timer_play_melody_handle,
                                          duration,
                                          timer_buzzer_cb,
                                          (buzzer_melody_t*)data,
                                          0,
                                          0);

    if (retval != SL_STATUS_OK){
      return;
    }
  }
}

/** @} (end group buzzer driver) */
