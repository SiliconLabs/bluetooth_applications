/***************************************************************************//**
 * @file at_parser_events.h
 * @brief header file for AT parser event listener
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

#ifndef AT_PARSER_EVENTS_H_
#define AT_PARSER_EVENTS_H_

#include "sl_status.h"

/*******************************************************************************
 **************************   ENUMS   *******************************
 ******************************************************************************/
typedef enum {
  ONCE, ALWAYS,
} event_type_t;

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
                            void *handler_data);

/**************************************************************************//**
 * @brief
 *    AT parser event listener process function.
 *    This function SHALL be called periodically in the main loop.
 *
 *****************************************************************************/
void at_event_process(void);

#endif /* AT_PARSER_EVENTS_H_ */
