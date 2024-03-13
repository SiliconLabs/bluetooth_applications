/***************************************************************************//**
 * @file
 * @brief WSTK display driver implementation for user defined displays.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
 ******************************************************************************/
#include <stdint.h>
#include <stdarg.h>
#include "esl_tag_log.h"
#include "esl_tag_display.h"
#include "esl_tag_image_core.h"
#include "esl_tag_user_display_driver.h"
#include "mikroe_e_paper_154_inch.h"
#include "sl_spidrv_instances.h"

#include "esl_tag_log.h"
#include "esl_tag_display.h"
#include "esl_tag_image_core.h"
#include "mikroe_e_paper_154_inch_config.h"

// ESL EPD Display timer fast update interval [tick]
#define ESL_MIKROE_EPD_DISPLAY_TIMER_FAST   2
// ESL EPD Display timer slow update interval [ms]
#define ESL_MIKROE_EPD_DISPLAY_TIMER_SLOW   (sl_sleeptimer_ms_to_tick(250))

// ESL EPD driver states
enum driver_states {
  DRIVER_STANDBY,
  DRIVER_IN_PROGRESS,
};

// ESL EPD driver results
struct driver_results {
  uint32_t    period;
  sl_status_t status;
  esl_error_t error;
};

static enum driver_states state_machine = DRIVER_STANDBY;

static struct driver_results last_result = {
  ESL_MIKROE_EPD_DISPLAY_TIMER_FAST,
  SL_STATUS_OK,
  ESL_ERROR_VENDOR_NOERROR
};

const uint8_t EINK154_LUT_TABLE[30] =
{
  0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
  0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
  0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
  0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

static sl_status_t esl_mikroe_epd_display_yield(void);

static void esl_mikroe_epd_display_step(uint8_t display_index,
                                        uint8_t image_index)
{
  (void)display_index;
  (void)image_index;
  sl_status_t sc = mikroe_e_paper_154_inch_display_image_non_blocking(
    image_index);

  last_result.status = sc;
  switch (state_machine) {
    case DRIVER_STANDBY:
      if (sc == SL_STATUS_IN_PROGRESS) {
        last_result.period = ESL_MIKROE_EPD_DISPLAY_TIMER_FAST;
        last_result.status = esl_mikroe_epd_display_yield();

        if (last_result.status == SL_STATUS_OK) {
          // Prepare for more call: set last error accordingly and step next
          //   state
          last_result.error = ESL_ERROR_RETRY;
          state_machine = DRIVER_IN_PROGRESS;
        } else {
          last_result.error = ESL_ERROR_INSUFFICIENT_RESOURCES;
        }
      }
      break;

    case DRIVER_IN_PROGRESS:
      // Repeat this step until finish
      if (sc != SL_STATUS_IN_PROGRESS) {
        last_result.status = sc;
        last_result.error = esl_core_get_last_error();
        state_machine = DRIVER_STANDBY;
      } else {
        (void)esl_mikroe_epd_display_yield();
      }
      break;
  }
}

static void esl_mikroe_epd_display_writer(sl_sleeptimer_timer_handle_t *handle,
                                          void *data)
{
  (void)handle;
  (void)data;

  if (state_machine == DRIVER_IN_PROGRESS) {
    esl_mikroe_epd_display_step(0, 0);
  }
}

static sl_status_t esl_mikroe_epd_display_yield(void)
{
  static sl_sleeptimer_timer_handle_t display_timer = { 0 };

  return sl_sleeptimer_start_timer_ms(&display_timer,
                                      last_result.period,
                                      esl_mikroe_epd_display_writer,
                                      (void *)NULL,
                                      0,
                                      0);
}

void esl_mikroe_epd_display_power_save(void)
{
  last_result.period = ESL_MIKROE_EPD_DISPLAY_TIMER_SLOW;
}

sl_status_t esl_user_display_write(int param_count, ...)
{
  // Please note that implementing this function is mandatory.

  // Declaring pointer to the argument list
  va_list     ptr;
  // Default status is SL_STATUS_IN_PROGRESS, set to SL_STATUS_OK once the
  // display data has been written successfully.
  sl_status_t status = SL_STATUS_IN_PROGRESS;
  // Variables for va parameters
  uint8_t     display_index;  // 2nd parameter
  uint8_t     image_index;    // 3rd parameter

  // User display write: Invalid parameters!
  sl_bt_esl_assert(param_count == ESL_DISPLAY_WRITE_FUNC_PARAMETERS_COUNT);

  // Initializing argument to the list pointer
  va_start(ptr, param_count);

  // Accessing variables (after each call to va_arg our ptr points to next one)
  display_index = (uint8_t)va_arg(ptr, int);
  image_index = (uint8_t)va_arg(ptr, int);

  // end argument list traversal
  va_end(ptr);

  if (state_machine == DRIVER_STANDBY) {
    // Call step function once to find out the current status of the EPD display
    esl_mikroe_epd_display_step(display_index, image_index);
  }

  // Set return status according to current state and set ESL error if any
  status = last_result.status;
  if ((status != SL_STATUS_OK)
      && (last_result.error != ESL_ERROR_VENDOR_NOERROR)) {
    esl_core_set_last_error(last_result.error);
  }

  return status;
}

sl_status_t esl_user_display_init(int param_count, ...)
{
  // Declaring pointer to the argument list
  va_list     ptr;
  // Default status is SL_STATUS_FAIL, set to SL_STATUS_OK once the display
  // has been initialized successfully.
  sl_status_t status = SL_STATUS_FAIL;
  // Variable for va parameter
  uint8_t     index;  // 2nd parameter

  // User display init: Invalid parameters!
  sl_bt_esl_assert(param_count == ESL_DISPLAY_INIT_FUNC_PARAMETERS_COUNT);

  // Initializing argument to the list pointer
  va_start(ptr, param_count);
  // Extract the index value (the only extra parameter)
  index = (uint8_t)va_arg(ptr, int);
  va_end(ptr);

  (void)index;
  mikroe_e_paper_154_inch_init(sl_spidrv_mikroe_handle);
  mikroe_e_paper_154_inch_start_config();
  mikroe_e_paper_154_inch_set_lut(EINK154_LUT_TABLE, 30);

  status = SL_STATUS_OK;
  return status;
}
