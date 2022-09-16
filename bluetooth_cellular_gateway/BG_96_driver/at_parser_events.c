/***************************************************************************//**
 * @file at_parser_events.c
 * @brief source for AT parser event listener
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

#include "at_parser_events.h"
#include <stdbool.h>
#include <stddef.h>

static void (*global_handle)();
static uint8_t global_ok_value;
static uint8_t *global_event_flag;
static bool event_handled = true;
static void *global_handler_data;

/**************************************************************************//**
 * @brief
 *    AT parser event listener listen function.
 *    This function can handle only one event at the same time.
 *
 * @param[in] event_flag
 *    Pointer to the flag to listen to.
 *
 * @param[in] event_flag_ok_value
 *    The decent flag value when the callback function should be called.
 *
 * @param[out] handle
 *    Pointer to a callback function.
 *
 * @param[in] handler_data
 *    Pointer to the data which will be given as callback parameter.
 *
 * @return
 *   SL_STATUS_OK if event listener has been set.
 *   SL_STATUS_ALLOCATION_FAILED if listener is already running.
 *
 *****************************************************************************/
sl_status_t at_listen_event(uint8_t *event_flag,
                            uint8_t event_ok_value,
                            void (*handle)(void*),
                            void *handler_data)
{
  global_event_flag = event_flag;
  global_ok_value = event_ok_value;
  global_handler_data = handler_data;
  if (handle != NULL && event_handled == true) {
    global_handle = handle;
    event_handled = false;
    return SL_STATUS_OK;
  }
  return SL_STATUS_ALLOCATION_FAILED;
}

/**************************************************************************//**
 * @brief
 *    AT parser event listener process function.
 *    This function SHALL be called periodically in the main loop.
 *
 *****************************************************************************/
void at_event_process(void)
{
  if (!event_handled
      && *global_event_flag == global_ok_value&& global_handle !=NULL) {
    global_handle(global_handler_data);
    event_handled = true;
  }
}

