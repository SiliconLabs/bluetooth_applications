/***************************************************************************//**
 * @file mikroe_bg96.c
 * @brief mikroe_bg96.c
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/
#include "sl_sleeptimer.h"
#include "mikroe_bg96.h"

#define BG96_GPIO_H_TIME 1000
#define BG96_TIMEOUT_MS  15000

static enum {
  bg96_ready = 0, bg96_wait_for_gpio, bg96_wait_for_device,
} bg96_state;

static bool required_state = false;
static at_scheduler_status_t *global_output;

static digital_out_t pwk_pin;
static digital_in_t sta_pin;

static sl_sleeptimer_timer_handle_t bg96_timer_gpio_handle;
static sl_sleeptimer_timer_handle_t bg96_timer_process_handler;
static void bg96_timer_gpio_callback(sl_sleeptimer_timer_handle_t *handle,
                                     void *data);
static void bg96_timer_process_callback(sl_sleeptimer_timer_handle_t *handle,
                                        void *data);

/**************************************************************************//**
 * @brief
 *   BG96 module initialize function.
 *
 *****************************************************************************/
void bg96_init(mikroe_uart_handle_t handle)
{
  at_parser_init(handle);
  digital_out_init(&pwk_pin,
                   hal_gpio_pin_name(BG96_PWK_PORT, BG96_PWK_PIN));
  digital_in_pulldown_init(&sta_pin,
                           hal_gpio_pin_name(BG96_STA_PORT, BG96_STA_PIN));

  sl_sleeptimer_start_periodic_timer_ms(&bg96_timer_process_handler,
                                        200,
                                        bg96_timer_process_callback,
                                        NULL,
                                        0,
                                        0);
}

/**************************************************************************//**
 * @brief
 *   BG96 is alive status getter function.
 *
 *****************************************************************************/
bool bg96_is_alive(void)
{
  return (bool) digital_in_read(&sta_pin);
}

/**************************************************************************//**
 * @brief
 *   BG96 module sleep function.
 *
 * @return
 *    SL_STATUS_OK if sleep initialization was successful.
 *    SL_STATUS_BUSY if sleep/wake up sequence has been already started.
 *    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG timer error.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *****************************************************************************/
sl_status_t bg96_sleep(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  if (bg96_ready != bg96_state) {
    return SL_STATUS_BUSY;
  }

  if (bg96_is_alive()) {
    global_output = output_object;
    digital_out_high(&pwk_pin);
    sl_status_t sc = sl_sleeptimer_restart_timer_ms(&bg96_timer_gpio_handle,
                                                    BG96_GPIO_H_TIME,
                                                    bg96_timer_gpio_callback,
                                                    (void *) NULL, 0, 0);

    if (SL_STATUS_OK != sc) {
      return sc;
    }

    required_state = false;
    bg96_state = bg96_wait_for_gpio;
    output_object->status = SL_STATUS_BUSY;
    output_object->error_code = 0;
  } else {
    // device is already sleeping
    output_object->status = SL_STATUS_OK;
    output_object->error_code = 0;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief
 *   BG96 module wake up function.
 *
 * @return
 *    SL_STATUS_OK if wake-up initialization was successful.
 *    SL_STATUS_BUSY if sleep/wake up sequence has been already started.
 *    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG timer error.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *****************************************************************************/
sl_status_t bg96_wake_up(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  if (bg96_ready != bg96_state) {
    return SL_STATUS_BUSY;
  }

  if (!bg96_is_alive()) {
    global_output = output_object;
    digital_out_high(&pwk_pin);
    sl_status_t sc = sl_sleeptimer_restart_timer_ms(&bg96_timer_gpio_handle,
                                                    BG96_GPIO_H_TIME,
                                                    bg96_timer_gpio_callback,
                                                    (void *) NULL, 0, 0);
    if (SL_STATUS_OK != sc) {
      return sc;
    }

    required_state = true;
    bg96_state = bg96_wait_for_gpio;
    output_object->status = SL_STATUS_BUSY;
    output_object->error_code = 0;
  } else {
    // device is already wakeup
    output_object->status = SL_STATUS_OK;
    output_object->error_code = 0;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief
 *   Local callback function for bg96 timer.
 *
 *****************************************************************************/
static void bg96_timer_gpio_callback(sl_sleeptimer_timer_handle_t *handle,
                                     void *data)
{
  (void)handle;
  (void) data;

  switch (bg96_state) {
    case bg96_wait_for_gpio:
      digital_out_low(&pwk_pin);
      bg96_state = bg96_wait_for_device;

      sl_sleeptimer_restart_timer_ms(&bg96_timer_gpio_handle,
                                     BG96_TIMEOUT_MS,
                                     bg96_timer_gpio_callback,
                                     (void *) NULL,
                                     0,
                                     0);
      break;
    case bg96_wait_for_device:
      if (global_output != NULL) {
        global_output->error_code = SL_STATUS_TIMEOUT;
        global_output->status = SL_STATUS_OK;
      }
      bg96_state = bg96_ready;
      break;
    default:
      bg96_state = bg96_ready;
  }
}

static void bg96_timer_process_callback(sl_sleeptimer_timer_handle_t *handle,
                                        void *data)
{
  (void)handle;
  (void)data;

  switch (bg96_state) {
    case bg96_ready:
      break;
    case bg96_wait_for_gpio:
      break;
    case bg96_wait_for_device:
      if (bg96_is_alive() == required_state) {
        sl_sleeptimer_stop_timer(&bg96_timer_gpio_handle);

        if (global_output != NULL) {
          global_output->error_code = 0;
          global_output->status = SL_STATUS_OK;
        }
        bg96_state = bg96_ready;
      }
      break;
  }
}
