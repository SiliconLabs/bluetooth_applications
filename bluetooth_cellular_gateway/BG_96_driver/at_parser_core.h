/***************************************************************************//**
 * @file at_parser_core.h
 * @brief header file for AT command parser core driver
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
#ifndef AT_PARSER_CORE_H_
#define AT_PARSER_CORE_H_
#include "sl_status.h"
#include "at_parser_platform.h"

/******************************************************************************
 ******************************   MACROS   ************************************
 *****************************************************************************/
#define CMD_Q_SIZE 20
#define AT_DEFAULT_TIMEOUT 3000
#define AT_OPEN_TIMEOUT 6000
#define AT_SEND_TIMEOUT 20000

/******************************************************************************
 **************************   TYPE DEFINITIONS   ******************************
 *****************************************************************************/
typedef enum {
  SCH_READY = 0, SCH_SENDING, SCH_PROCESSED, SCH_ERROR,
} at_cmd_scheduler_state_t;

typedef struct {
  sl_status_t status;
  uint16_t error_code;
  uint8_t response_data[CMD_MAX_SIZE];
} at_scheduler_status_t;

/**************************************************************************//**
 * @brief
 *    AT parser core initialization
 *****************************************************************************/
void at_parser_init(void);

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
void at_parser_init_output_object(at_scheduler_status_t *output_object);

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
                                 uint8_t *data_to_add);

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
sl_status_t at_parser_start_scheduler(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    Get scheduler state.
 *
 * @return
 *    Actual state of the command scheduler.
 *    SCH_READY, SCH_SENDING, SCH_PROCESSED, SCH_ERROR
 *
 *****************************************************************************/
at_cmd_scheduler_state_t at_parser_get_scheduler_state(void);

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
sl_status_t at_parser_add_cmd_to_q(at_cmd_desc_t *at_cmd_descriptor);

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
void at_parser_clear_cmd(at_cmd_desc_t *at_cmd_descriptor);

/**************************************************************************//**
 * @brief
 *    AT parser process function.
 *    This function SHALL be called periodically in the main loop.
 *
 *****************************************************************************/
void at_parser_process(void);

/******************************************************************************
 *******************   LINE CALLBACK FUNCTIONS   ******************************
 *****************************************************************************/
void at_ok_error_cb(uint8_t *new_line, uint8_t call_number);
void at_cops_cb(uint8_t *new_line, uint8_t call_number);
void at_recv_cb(uint8_t *new_line, uint8_t call_number);
void at_send_cb(uint8_t *new_line, uint8_t call_number);
void at_data_cb(uint8_t *new_line, uint8_t call_number);
void at_ip_cb(uint8_t *new_line, uint8_t call_number);
void at_imei_cb(uint8_t *new_line, uint8_t call_number);
void at_qistate_cb(uint8_t *new_line, uint8_t call_number);
void at_open_cb(uint8_t *new_line, uint8_t call_number);
void at_gpsloc_cb(uint8_t *new_line, uint8_t call_number);

#endif /* AT_PARSER_CORE_H_ */
