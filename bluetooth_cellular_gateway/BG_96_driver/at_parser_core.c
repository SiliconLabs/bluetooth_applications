/***************************************************************************//**
 * @file at_parser_core.c
 * @brief AT command parser core driver source
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
#include "at_parser_core.h"
#include <string.h>
#include "circular_queue.h"
#include <stdlib.h>

/******************************************************************************
 **********************   MACRO UTILITY FUNCTIONS   ***************************
 *****************************************************************************/
#define has_substring(container, substr) \
  (NULL != strstr ((const char*) new_line, (const char*)substr))

static Queue_t cmd_q;
static at_cmd_scheduler_state_t sch_state = SCH_READY;
static at_scheduler_status_t *global_status;

static void general_platform_cb(uint8_t *data,
                                uint8_t call_number);

/**************************************************************************//**
 * @brief
 *    AT parser core initialization
 *****************************************************************************/
void at_parser_init()
{
  queueInit(&cmd_q, CMD_Q_SIZE);
  at_platform_init(general_platform_cb);
}

void at_parser_report_data(uint8_t *data)
{
  if (global_status != NULL) {
    size_t data_length = strlen((const char*) data);
    if (CMD_MAX_SIZE >= data_length) {
      strcpy((char*) global_status->response_data, (const char*) data);
    } else {
      memcpy(global_status->response_data, data, data_length);
      global_status->response_data[CMD_MAX_SIZE - 1] = '\0';
    }
  }
}

void at_parser_get_ip(uint8_t *response,
                      uint8_t *ip_output)
{
  uint8_t *first_quote, *second_quote;
  uint32_t ip_length;

  first_quote = (uint8_t*) strchr((const char*) response, (int) '\"');
  if (first_quote != NULL) {
    first_quote++;
    second_quote = (uint8_t*) strchr((const char*) first_quote, (int) '\"');
    if (second_quote != NULL) {
      ip_length = (uint32_t) second_quote - (uint32_t) first_quote;
      ip_length--;
      memcpy(ip_output, first_quote, ip_length);
      ip_output[ip_length] = '\0';
    }
  }
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
  output_object->error_code = 0;
  output_object->status = SL_STATUS_NOT_INITIALIZED;
  memset((void*) output_object->response_data, '\0', CMD_MAX_SIZE);
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
 *    SL_STATUS_ALLOCATION_FAILED if command scheduler is already running.
 *
 *****************************************************************************/
sl_status_t at_parser_start_scheduler(at_scheduler_status_t *output_object)
{
  at_cmd_desc_t *at_cmd_descriptor;

  if (SCH_READY != sch_state) {
    return SL_STATUS_BUSY;
  }
  if (queueIsEmpty(&cmd_q)) {
    return SL_STATUS_OK;
  }
  sch_state = SCH_SENDING;
  if (output_object != NULL) {
    global_status = output_object;
    at_parser_init_output_object(global_status);
    at_cmd_descriptor = queuePeek(&cmd_q);
    return at_platform_send_cmd(at_cmd_descriptor->cms_string,
        at_cmd_descriptor->timeout_ms);
  }
  return SL_STATUS_ALLOCATION_FAILED;
}

void at_parser_scheduler_next_cmd()
{
  sch_state = SCH_PROCESSED;
}

void at_parser_scheduler_error(uint8_t error_code)
{
  global_status->error_code = error_code;
  sch_state = SCH_ERROR;
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
 *    SL_STATUS_ALLOCATION_FAILED if command buffer size is overflowed
 *
 *****************************************************************************/
sl_status_t at_parser_extend_cmd(at_cmd_desc_t *at_cmd_descriptor,
                                 uint8_t *data_to_add)
{
  size_t sum_length = strlen((const char*) at_cmd_descriptor->cms_string);
  sum_length += strlen((const char*) data_to_add);
  if (sum_length <= CMD_MAX_SIZE) {
    strcat((char*) at_cmd_descriptor->cms_string, (const char*) data_to_add);
    return SL_STATUS_OK;
  }
  return SL_STATUS_ALLOCATION_FAILED;
}

/**************************************************************************//**
 * @brief
 *    Clears the command string in the command descriptor.
 *
 * @param[in] at_cmd_descriptor
 *    Pointer to the command descriptor to add.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_ALLOCATION_FAILED if command queue is full.
 *
 *****************************************************************************/
void at_parser_clear_cmd(at_cmd_desc_t *at_cmd_descriptor)
{
  memset(at_cmd_descriptor->cms_string, 0, CMD_MAX_SIZE);
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
 *
 *****************************************************************************/
sl_status_t at_parser_add_cmd_to_q(at_cmd_desc_t *at_cmd_descriptor)
{
  if (queueIsFull(&cmd_q)) {
    return SL_STATUS_ALLOCATION_FAILED;
  }
  queueAdd(&cmd_q, (void*) at_cmd_descriptor);
  return SL_STATUS_OK;
}

/**************************************************************************//**
 * @brief
 *    AT parser process function.
 *    This function SHALL be called periodically in the main loop.
 *
 *****************************************************************************/
void at_parser_process(void)
{
  at_cmd_desc_t *at_cmd_descriptor;

  switch (sch_state) {
  case SCH_PROCESSED:
    //remove previous command
    queueRemove(&cmd_q);
    at_platform_finish_cmd();
    if (!queueIsEmpty(&cmd_q)) {
      at_cmd_descriptor = (at_cmd_desc_t*) queuePeek(&cmd_q);
      at_platform_send_cmd(at_cmd_descriptor->cms_string,
          at_cmd_descriptor->timeout_ms);
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
static void general_platform_cb(uint8_t *data,
                                uint8_t call_number)
{
  at_cmd_desc_t *at_cmd_descriptor = queuePeek(&cmd_q);
  //call number == 0 means timeout occurred
  if (call_number == 0) {
    at_platform_finish_cmd();
    at_parser_scheduler_error(SL_STATUS_TIMEOUT);
  } else {
    if (at_cmd_descriptor != NULL) {
      // call line callback of the command descriptor if available
      at_cmd_descriptor->ln_cb(data, call_number);
    }
  }
}

/*******************************************************************************
 **********************   PREDEFINED LINE CALLBACKS   **************************
 ******************************************************************************/
void at_ok_error_cb(uint8_t *new_line,
                    uint8_t call_number)
{
  switch (call_number) {
  case 1:
    if (has_substring(new_line, "OK")) {
      at_parser_scheduler_next_cmd();
    }
    if (has_substring(new_line, "ERROR")) {
      at_parser_report_data(new_line);
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }

}

void at_send_cb(uint8_t *new_line,
                uint8_t call_number)
{
  switch (call_number) {
  case 1:
    if (has_substring(new_line, ">")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_data_cb(uint8_t *new_line,
                uint8_t call_number)
{
  switch (call_number) {
  case 1:
    if (!has_substring(new_line, " ")) {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 2:
    if (!has_substring(new_line, "SEND OK")) {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 3:
    if (!has_substring(new_line, "+QIURC:")) {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 4:
    if (!has_substring(new_line, "[")) {
      at_parser_report_data(new_line);
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 5:
    if (!has_substring(new_line, "+QIURC:")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_cops_cb(uint8_t *new_line,
                uint8_t call_number)
{
  switch (call_number) {
  case 1:
    at_parser_report_data(new_line);
    if (!has_substring(new_line, "+COPS:")) {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 2:
    if (has_substring(new_line, "OK")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_ip_cb(uint8_t *new_line,
              uint8_t call_number)
{
  switch (call_number) {
  case 1:
    if (has_substring(new_line, "+QIACT:")) {
      at_parser_get_ip(new_line, global_status->response_data);
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 2:
    if (has_substring(new_line, "OK")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_imei_cb(uint8_t *new_line,
                uint8_t call_number)
{
  switch (call_number) {
  case 1:
    at_parser_report_data(new_line);
    break;
  case 2:
    if (has_substring(new_line, "OK")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_recv_cb(uint8_t *new_line,
                uint8_t call_number)
{
  uint8_t *space_ptr;
  uint32_t qird_data;
  static bool data_available = false;

  switch (call_number) {
  case 1:
    if (!has_substring(new_line, "+QIRD:")) {
      at_parser_report_data(new_line);
      at_parser_scheduler_error(SL_STATUS_FAIL);
    } else {
      space_ptr = (uint8_t*) strchr((const char*) new_line, ' ');
      if (space_ptr != NULL) {
        qird_data = (uint32_t) strtol((const char*) (++space_ptr), NULL, 10);
        if (qird_data > 0) {
          data_available = true;
        }
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
    }
    break;
  case 2:
    if (data_available) {
      at_parser_report_data(new_line);
    } else {
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
    }
    data_available = false;
    break;
  case 3:
    if (has_substring(new_line, "OK")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_qistate_cb(uint8_t *new_line,
                   uint8_t call_number)
{
  switch (call_number) {
  case 1:
    at_parser_report_data(new_line);
    if (!has_substring(new_line, "+QISTATE:")) {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 2:
    if (has_substring(new_line, "OK")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_open_cb(uint8_t *new_line,
                uint8_t call_number)
{
//wait OK
//wait +QIOPEN: 0,0 if second parameter != 0 then error
  uint8_t *coma;
  uint32_t error_code;
  switch (call_number) {
  case 1:
    if (!has_substring(new_line, "OK")) {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 2:
    at_parser_report_data(new_line);
    if (has_substring(new_line, "+QIOPEN:")) {
      coma = (uint8_t*) strchr((const char*) new_line, ',');
      if (NULL != coma) {
        error_code = strtol((const char*) (++coma), NULL, 10);
        if (error_code != 0) {
          at_parser_report_data(new_line);
          at_parser_scheduler_error(SL_STATUS_FAIL);
        } else {
          at_parser_scheduler_next_cmd();
        }
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

void at_gpsloc_cb(uint8_t *new_line,
                  uint8_t call_number)
{
  //wait for +QGPSLOC: ...
  //wait OK
  switch (call_number) {
  case 1:
    at_parser_report_data(new_line);
    if (!has_substring(new_line, "+QGPSLOC:")) {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  case 2:
    if (has_substring(new_line, "OK")) {
      at_parser_scheduler_next_cmd();
    } else {
      at_parser_scheduler_error(SL_STATUS_FAIL);
    }
    break;
  default:
    at_parser_scheduler_error(SL_STATUS_FAIL);
    break;
  }
}

