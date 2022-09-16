/***************************************************************************//**
 * @file nb_gnss.h
 * @brief NB IoT GNSS high level command driver source
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
#include "nb_gnss.h"
#include "at_parser_core.h"
#include "at_parser_events.h"
#include "at_parser_utility.h"

/**************************************************************************//**
 * @brief
 *    Initialization of GNSS function.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t gnss_start(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_qgps = { "AT+QGPS=1", at_ok_error_cb,
  AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_qgps));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 * @brief
 *    Get the GNSS location string.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t gnss_get_position(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_gpsloc = { "AT+QGPSLOC?", at_gpsloc_cb,
  AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_gpsloc));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief
 *    Stop GNSS function.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t gnss_stop(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_imei = { "AT+QGPSEND", at_ok_error_cb,
  AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_imei));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return SL_STATUS_OK;
}

