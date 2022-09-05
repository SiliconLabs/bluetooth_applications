/***************************************************************************//**
 * @file bg96_driver.c
 * @brief BG96 power state driver source
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
#include "bg96_driver.h"
#include "em_gpio.h"
#include "sl_sleeptimer.h"
#include "sl_emlib_gpio_init_bg96_pwk_config.h"
#include "sl_emlib_gpio_init_bg96_sta_config.h"
#include "at_parser_core.h"

static enum {
  bg96_ready = 0, bg96_wait_for_gpio, bg96_wait_for_device,
} bg96_state;
static bool required_state = false;
static sl_sleeptimer_timer_handle_t bg96_timer;
static at_scheduler_status_t *global_output;

static void bg96_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                          void *data);

/**************************************************************************//**
 * @brief
 *   BG96 module initialize function.
 *
 *****************************************************************************/
void bg96_init(void)
{

  at_parser_init();
}

/**************************************************************************//**
 * @brief
 *   BG96 is alive status getter function.
 *
 *****************************************************************************/
bool bg96_is_alive(void)
{
  return (bool) GPIO_PinInGet(SL_EMLIB_GPIO_INIT_BG96_STA_PORT,
  SL_EMLIB_GPIO_INIT_BG96_STA_PIN);
}

/**************************************************************************//**
 * @brief
 *   BG96 module sleep function.
 *
 * @return
 *    SL_STATUS_OK if wake-up initialization was successful.
 *    SL_STATUS_BUSY if sleep/wake up sequence has been already started.
 *    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG timer error.
 *    SL_STATUS_INVALID_PARAMETER timer related error.
 *****************************************************************************/
sl_status_t bg96_sleep(at_scheduler_status_t *output_object)
{
  sl_status_t timer_status;

  if (bg96_ready == bg96_state) {
    at_platform_disable_ir();
    if (bg96_is_alive()) {
      if (NULL == output_object) {
        return SL_STATUS_ALLOCATION_FAILED;
      }
      global_output = output_object;
      GPIO_PinOutSet(SL_EMLIB_GPIO_INIT_BG96_PWK_PORT,
      SL_EMLIB_GPIO_INIT_BG96_PWK_PIN);
      timer_status = sl_sleeptimer_restart_timer_ms(&bg96_timer,
      BG96_GPIO_H_TIME, bg96_timer_cb, (void*) NULL, 0, 0);
      if (SL_STATUS_OK == timer_status) {

        required_state = false;
        bg96_state = bg96_wait_for_gpio;
        output_object->status = SL_STATUS_BUSY;
        output_object->error_code = 0;
        return SL_STATUS_OK;
      } else {
        return timer_status;
      }
    } else {
      //device is already sleeping
      output_object->status = SL_STATUS_OK;
      output_object->error_code = 0;
    }
    return SL_STATUS_OK;
  } else {
    return SL_STATUS_BUSY;
  }
}

/**************************************************************************//**
 * @brief
 *   BG96 module wake up function.
 *
 * @return
 *    SL_STATUS_OK if wake-up initialization was successful.
 *    SL_STATUS_BUSY if sleep/wake up sequence has been already started.
 *    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG timer error.
 *    SL_STATUS_INVALID_PARAMETER timer related error.
 *****************************************************************************/
sl_status_t bg96_wake_up(at_scheduler_status_t *output_object)
{
  sl_status_t timer_status;

  if (bg96_ready == bg96_state) {
    at_platform_disable_ir();
    if (!bg96_is_alive()) {
        global_output = output_object;
      GPIO_PinOutSet(SL_EMLIB_GPIO_INIT_BG96_PWK_PORT,
      SL_EMLIB_GPIO_INIT_BG96_PWK_PIN);
      timer_status = sl_sleeptimer_restart_timer_ms(&bg96_timer,
      BG96_GPIO_H_TIME, bg96_timer_cb, (void*) NULL, 0, 0);
      if (SL_STATUS_OK == timer_status) {
        required_state = true;
        bg96_state = bg96_wait_for_gpio;
        output_object->status = SL_STATUS_BUSY;
        output_object->error_code = 0;
        return SL_STATUS_OK;
      } else {
        return timer_status;
      }
    } else {
      //device is already sleeping
      output_object->status = SL_STATUS_OK;
      output_object->error_code = 0;
    }
    return SL_STATUS_OK;
  } else {
    return SL_STATUS_BUSY;
  }
}

/**************************************************************************//**
 * @brief
 *   Local callback function for bg96 timer.
 *
 *****************************************************************************/
static void bg96_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                          void *data)
{
  (void) handle;
  (void) data;
  switch (bg96_state) {
  case bg96_wait_for_gpio:
    GPIO_PinOutClear(SL_EMLIB_GPIO_INIT_BG96_PWK_PORT,
    SL_EMLIB_GPIO_INIT_BG96_PWK_PIN);
    sl_sleeptimer_restart_timer_ms(&bg96_timer, BG96_TIMEOUT_MS, bg96_timer_cb,
        (void*) NULL, 0, 0);
    bg96_state = bg96_wait_for_device;
    break;
  case bg96_wait_for_device:
    global_output->error_code = SL_STATUS_TIMEOUT;
    global_output->status = SL_STATUS_OK;
    bg96_state = bg96_ready;
    break;
  default:
    bg96_state = bg96_ready;
  }
}

/**************************************************************************//**
 * @brief
 *   B96 process function. This is a non-blocking function.
 *   SHALL be called periodically in the main loop.
 *
 *****************************************************************************/
void bg96_process(void)
{
  switch (bg96_state) {
  case bg96_ready:
    break;
  case bg96_wait_for_gpio:
    break;
  case bg96_wait_for_device:
    if (bg96_is_alive() == required_state) {
      sl_sleeptimer_stop_timer(&bg96_timer);
      global_output->error_code = 0;
      global_output->status = SL_STATUS_OK;
      bg96_state = bg96_ready;
    }
    break;
  }
}

