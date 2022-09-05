/***************************************************************************//**
 * @file at_parser_platform.h
 * @brief header file for AT command parser platform driver
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

#ifndef AT_PARSER_PLATFORM_H_
#define AT_PARSER_PLATFORM_H_
#include "sl_status.h"

/*******************************************************************************
 ********************************   MACROS   ***********************************
 ******************************************************************************/
#define OUT_BUFFER_SIZE 100
#define IN_BUFFER_SIZE 100
#define CMD_MAX_SIZE 100

#define MIKROE_RX_PORT  gpioPortB
#define MIKROE_TX_PORT  gpioPortB
#define MIKROE_RX_PIN 2
#define MIKROE_TX_PIN 1

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
  uint16_t timeout_ms;
} at_cmd_desc_t;

/**************************************************************************//**
 * @brief
 *    CMU initialization.
 *****************************************************************************/
void initCMU(void);

/**************************************************************************//**
 * @brief
 *    GPIO initialization.
 *****************************************************************************/
void initGPIO(void);

/**************************************************************************//**
 * @brief
 *   USART0 initialization.
 *
 *****************************************************************************/
void initUSART0(void);

/**************************************************************************//**
 * @brief
 *   Disable all USART0 interrupts.
 *
 *****************************************************************************/
void at_platform_disable_ir(void);

/**************************************************************************//**
 * @brief
 *    Enable USART0 RX interrupt.
 *
 *****************************************************************************/
void at_platform_enable_ir(void);

/**************************************************************************//**
 * @brief
 *   Initialization of platform driver.
 *
 * @param[in] line_callback
 *   Callback function for new line (and ">" character for special commands).
 *
 *****************************************************************************/
void at_platform_init(ln_cb_t line_callback);

/**************************************************************************//**
 * @brief
 *   Check ready status of platform driver.
 *
 *****************************************************************************/
sl_status_t at_platform_check_device_ready();

/**************************************************************************//**
 * @brief
 *   Platform driver send command function.
 *   This function adds \r\n to the command string.
 *   Command string SHALL be allocated until command is sent.
 *
 * @param[in] cmd
 *   Pointer to the command to send.
 *
 * @param[in] timeout_ms
 *    Timeout for the command in milliseconds.
 *
 * @return
 *   SL_STATUS_OK if there are no errors.
 *   SL_STATUS_ALLOCATION_FAILED if output buffer size is overflowed.
 *    ************************************************************************/
sl_status_t at_platform_send_cmd(volatile uint8_t *cmd,
                                 volatile uint16_t timeout_ms);

/**************************************************************************//**
 * @brief
 *   Platform driver finish function.
 *   Used to end ongoing communication.
 *   Stops timeout and hardware interrupts.
 *
 *****************************************************************************/
void at_platform_finish_cmd(void);

#endif /* AT_PARSER_PLATFORM_H_ */
