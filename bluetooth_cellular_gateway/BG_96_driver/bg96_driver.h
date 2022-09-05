/***************************************************************************//**
 * @file bg96_driver.h
 * @brief header file for BG96 power state driver
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

#ifndef BG96_DRIVER_H_
#define BG96_DRIVER_H_
#include <stdbool.h>
#include "sl_status.h"
#include "at_parser_core.h"

/*******************************************************************************
 ********************************   MACROS   ***********************************
 ******************************************************************************/
#define BG96_GPIO_H_TIME 800
#define BG96_TIMEOUT_MS 5000

/**************************************************************************//**
 * @brief
 *   BG96 module initialize function.
 *
 *****************************************************************************/
void bg96_init (void);

/**************************************************************************//**
 * @brief
 *   BG96 is alive status getter function.
 *
 *****************************************************************************/
bool bg96_is_alive (void);

/**************************************************************************//**
 * @brief
 *   BG96 module sleep function.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if wake-up initialization was successful.
 *    SL_STATUS_BUSY if sleep/wake up sequence has been already started
 *    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG timer error.
 *    SL_STATUS_INVALID_PARAMETER timer related error.
 *****************************************************************************/
sl_status_t bg96_sleep (at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *   BG96 module wake up function.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if wake-up initialization was successful.
 *    SL_STATUS_BUSY if sleep/wake up sequence has been already started.
 *    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG timer error.
 *    SL_STATUS_INVALID_PARAMETER timer related error.
 *****************************************************************************/
sl_status_t bg96_wake_up (at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *   B96 process function. This is a non-blocking function.
 *   SHALL be called periodically in the main loop.
 *
 *****************************************************************************/
void bg96_process (void);

#endif /* BG96_DRIVER_H_ */
