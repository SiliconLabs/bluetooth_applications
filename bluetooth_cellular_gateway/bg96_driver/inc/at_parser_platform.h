/***************************************************************************//**
 * @file at_parser_platform.h
 * @brief header file for AT command parser platform driver
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

#ifndef AT_PARSER_PLATFORM_H_
#define AT_PARSER_PLATFORM_H_

#include <string.h>
#include <sl_string.h>

#include "sl_sleeptimer.h"
#include "sl_status.h"
#include "drv_uart.h"
#include "mikroe_bg96_config.h"

/*******************************************************************************
 ********************************   MACROS   ***********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   TYPE DEFINITIONS   *******************************
 ******************************************************************************/
typedef void (*ln_cb_t)(uint8_t *response, uint8_t call_number);

typedef enum {
  NOT_INITIALIZED = 0, READY, TRANSMIT
} at_platform_status_t;

typedef struct {
  uint8_t cms_string[CMD_MAX_SIZE];
  ln_cb_t ln_cb;
  uint32_t timeout_ms;
} at_cmd_desc_t;

/**************************************************************************//**
 * @brief
 *   Initialization of platform driver.
 *
 * @param[in] handle
 *   Mikroe UART handle instance.
 * @param[in] line_callback
 *   Callback function for new line (and ">" character for special commands).
 *
 *****************************************************************************/
sl_status_t at_platform_init(mikroe_uart_handle_t handle,
                             ln_cb_t line_callback);

/**************************************************************************//**
 * @brief
 *   Check ready status of platform driver.
 *
 *****************************************************************************/
sl_status_t at_platform_check_device_ready(void);

/**************************************************************************//**
 * @brief
 *   Platform driver send command function.
 *   This function adds \r to the command string.
 *   Command string SHALL be allocated until command is sent.
 *   This function uses UART TX and RX interrupt.
 *
 * @param[in] cmd
 *   Pointer to the command to send.
 *
 * @param[in] timeout_ms
 *    Timeout for the command in milliseconds.
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *   SL_STATUS_INVALID_PARAMETER if cmd == NULL.
 ******************************************************************************/
sl_status_t at_platform_send_cmd(uint8_t *cmd, uint16_t timeout_ms);

/**************************************************************************//**
 * @brief
 *   Platform driver finish function.
 *   Used to end ongoing communication.
 *   Stops timeout and hardware interrupts.
 *
 *****************************************************************************/
void at_platform_finish_cmd(void);

/**************************************************************************//**
 * @brief
 *   Platform driver process function.
 *   This function removes \r and \n characters.
 *   Calls global callback if it is defined.
 *   Used to process incoming uart rx data
 *
 *****************************************************************************/
void at_platform_process(void);

#endif /* AT_PARSER_PLATFORM_H_ */
