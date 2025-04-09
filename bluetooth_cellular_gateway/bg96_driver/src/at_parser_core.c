/***************************************************************************//**
 * @file at_parser_core.c
 * @brief AT command parser core driver source
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#include "at_parser_core.h"

/******************************************************************************
 **********************   MACRO UTILITY FUNCTIONS   ***************************
 *****************************************************************************/

static Queue_t cmd_q;
static at_cmd_scheduler_state_t sch_state = SCH_READY;
static at_scheduler_status_t *global_status;

static void general_platform_cb(uint8_t *data, uint8_t call_number);

/**************************************************************************//**
 * @brief
 *    AT parser core initialization
 *****************************************************************************/
void at_parser_init(mikroe_uart_handle_t handle)
{
  queueInit(&cmd_q, CMD_Q_SIZE);
  at_platform_init(handle, general_platform_cb);
}

/**************************************************************************//**
 * @brief
 *    AT parser output object initialization.
 *    Sets the status to SL_STATUS_NOT_INITIALIZED.
 *    Sets the error code to 0.
 *    Clears the response buffer.
 *
 * @param[in] output_object
 *    Pointer to the output object which should be initialized.
 *
 *****************************************************************************/
void at_parser_init_output_object(at_scheduler_status_t *output_object)
{
  if (output_object != NULL) {
    output_object->error_code = 0;
    output_object->status = SL_STATUS_NOT_INITIALIZED;
    memset((void *) output_object->response_data, '\0', CMD_MAX_SIZE);
  }
}

/**************************************************************************//**
 * @brief
 *    AT parser extend command.
 *    Extends the command sting of the command descriptor.
 *
 * @param[in] at_cmd_descriptor
 *    Pointer to the command descriptor.
 *
 * @param[in] data_to_add
 *    String which the command string should be extended with.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if at_cmd_descriptor == NULL or
 *    data_to_add == NULL
 *
 *****************************************************************************/
sl_status_t at_parser_extend_cmd(at_cmd_desc_t *at_cmd_descriptor,
                                 uint8_t *data_to_add)
{
  if ((at_cmd_descriptor != NULL) && (data_to_add != NULL)) {
    size_t sum_length = sl_strlen((char *) at_cmd_descriptor->cms_string);
    sum_length += sl_strlen((char *) data_to_add);

    if (sum_length <= CMD_MAX_SIZE) {
      sl_strcat_s((char *) at_cmd_descriptor->cms_string,
                  CMD_MAX_SIZE,
                  (const char *) data_to_add);
      return SL_STATUS_OK;
    }
  }

  return SL_STATUS_INVALID_PARAMETER;
}

/**************************************************************************//**
 * @brief
 *    Start AT command scheduler.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the status and response.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *
 *****************************************************************************/
sl_status_t at_parser_start_scheduler(at_scheduler_status_t *output_object)
{
  if (NULL != output_object) {
    at_cmd_desc_t at_cmd_descriptor;

    if (SCH_READY != sch_state) {
      return SL_STATUS_BUSY;
    }
    if (queueIsEmpty(&cmd_q)) {
      return SL_STATUS_OK;
    }

    sch_state = SCH_SENDING;
    global_status = output_object;
    at_parser_init_output_object(global_status);
    at_cmd_descriptor = *((at_cmd_desc_t *)queuePeek(&cmd_q));
    return at_platform_send_cmd(at_cmd_descriptor.cms_string,
                                at_cmd_descriptor.timeout_ms);
  }

  return SL_STATUS_INVALID_PARAMETER;
}

/**************************************************************************//**
 * @brief
 *    Get scheduler state.
 *
 * @return
 *    Actual state of the command scheduler.
 *    SCH_READY, SCH_SENDING, SCH_PROCESSED, SCH_ERROR
 *
 *****************************************************************************/
at_cmd_scheduler_state_t at_parser_get_scheduler_state()
{
  return sch_state;
}

/**************************************************************************//**
 * @brief
 *    Add a command descriptor to the command queue.
 *    Command descriptor MUST be allocated until the scheduler runs.
 *
 * @param[in] at_cmd_descriptor
 *    Pointer to the command descriptor to add.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_ALLOCATION_FAILED if command queue is full.
 *    SL_STATUS_INVALID_PARAMETER if at_cmd_descriptor == NULL
 *
 *****************************************************************************/
sl_status_t at_parser_add_cmd_to_q(const at_cmd_desc_t *at_cmd_descriptor)
{
  if (queueIsFull(&cmd_q)) {
    return SL_STATUS_ALLOCATION_FAILED;
  }

  if (at_cmd_descriptor == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  if (queueAdd(&cmd_q, (at_cmd_desc_t *)at_cmd_descriptor) == true) {
    return SL_STATUS_OK;
  }

  return SL_STATUS_ALLOCATION_FAILED;
}

/**************************************************************************//**
 * @brief
 *    Clears the command string in the command descriptor.
 *
 * @param[in] at_cmd_descriptor
 *    Pointer to the command descriptor to clear.
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if at_cmd_descriptor == NULL
 *****************************************************************************/
sl_status_t at_parser_clear_cmd(at_cmd_desc_t *at_cmd_descriptor)
{
  if (at_cmd_descriptor != NULL) {
    memset(at_cmd_descriptor->cms_string, 0, CMD_MAX_SIZE);
    return SL_STATUS_OK;
  }
  return SL_STATUS_INVALID_PARAMETER;
}

/**************************************************************************//**
 * @brief
 *    AT parser scheduler next cmd function.
 *
 *****************************************************************************/
void at_parser_scheduler_next_cmd(void)
{
  sch_state = SCH_PROCESSED;
}

/**************************************************************************//**
 * @brief
 *    AT parser scheduler error function.
 * @param[in] error_code
 *    Error code
 *
 *****************************************************************************/
void at_parser_scheduler_error(uint8_t error_code)
{
  global_status->error_code = error_code;
  sch_state = SCH_ERROR;
}

/**************************************************************************//**
 * @brief
 *    AT parser report data function.
 * @param[in/out] data
 *    Data to parse
 *
 *****************************************************************************/
void at_parser_report_data(uint8_t *data)
{
  if ((global_status != NULL) && (data != NULL)) {
    size_t data_length = sl_strlen((char *) data);
    if (CMD_MAX_SIZE >= data_length) {
      sl_strcpy_s((char *) global_status->response_data, CMD_MAX_SIZE,
                  (const char *) data);
    } else {
      memcpy(global_status->response_data, data, CMD_MAX_SIZE);
      global_status->response_data[CMD_MAX_SIZE - 1] = '\0';
    }
  }
}

/**************************************************************************//**
 * @brief
 *    AT parser process function.
 *    This function SHALL be called periodically in the main loop.
 *
 *****************************************************************************/
void at_parser_process(void)
{
  at_cmd_desc_t at_cmd_descriptor;

  switch (sch_state) {
    case SCH_PROCESSED:
      // remove previous command
      queueRemove(&cmd_q);
      at_platform_finish_cmd();
      if (!queueIsEmpty(&cmd_q)) {
        at_cmd_descriptor = *((at_cmd_desc_t *)queuePeek(&cmd_q));
        at_platform_send_cmd(at_cmd_descriptor.cms_string,
                             at_cmd_descriptor.timeout_ms);
        sch_state = SCH_SENDING;
      } else {
        global_status->status = SL_STATUS_OK;
        sch_state = SCH_READY;
      }
      break;
    case SCH_ERROR:
      at_platform_finish_cmd();
      while (!queueIsEmpty(&cmd_q)) {
        queueRemove(&cmd_q);
      }
      global_status->status = SL_STATUS_OK;
      sch_state = SCH_READY;
      break;
    case SCH_READY:
      break;
    case SCH_SENDING:
      break;
  }
}

/**************************************************************************//**
 * @brief
 *    General platfrom core callback function.
 *    Called in case of new line or timeout.
 *
 * @param[in] data
 *    Pointer to the data received in an new line.
 *
 * @param[in] call_number
 *    Number of received new lines.
 *    Is 0 if timeout occurred
 *
 *****************************************************************************/
static void general_platform_cb(uint8_t *data, uint8_t call_number)
{
  at_cmd_desc_t at_cmd_descriptor;

  if (!queueIsEmpty(&cmd_q)) {
    at_cmd_descriptor = *((at_cmd_desc_t *)queuePeek(&cmd_q));

    // call number == 0 means timeout occurred
    if (call_number == 0) {
      at_platform_finish_cmd();
      at_parser_scheduler_error(SL_STATUS_TIMEOUT);
    } else {
      if (at_cmd_descriptor.ln_cb != NULL) {
        // call line callback of the command descriptor if available
        at_cmd_descriptor.ln_cb(data, call_number);
      }
    }
  }
}
