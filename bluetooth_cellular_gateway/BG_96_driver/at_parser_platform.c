/***************************************************************************//**
 * @file at_parser_platform.c
 * @brief AT command parser platform driver source
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

#include "at_parser_platform.h"
#include "sl_iostream.h"
#include "sl_iostream_init_instances.h"
#include "sl_iostream_handles.h"
#include "sl_sleeptimer.h"
#include "uartdrv.h"
#include <string.h>

static void timer_cb(sl_sleeptimer_timer_handle_t *handle,
                     void *data);

volatile uint8_t *output_ptr = NULL;
uint8_t input_buffer[IN_BUFFER_SIZE];
uint8_t input_buffer_index;

at_platform_status_t status = NOT_INITIALIZED;
ln_cb_t global_cb = 0;
sl_sleeptimer_timer_handle_t my_timer;
uint8_t line_counter = 0;

/**************************************************************************//**
 * @brief
 *    CMU initialization.
 *****************************************************************************/
void initCMU(void)
{
  // Enable clock to GPIO and USART0
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);
}

/**************************************************************************//**
 * @brief
 *    GPIO initialization.
 *****************************************************************************/
void initGPIO(void)
{
  // Configure the USART TX pin to the board controller as an output
  GPIO_PinModeSet(MIKROE_TX_PORT, MIKROE_TX_PIN, gpioModePushPull, 0);

  // Configure the USART RX pin to the board controller as an input
  GPIO_PinModeSet(MIKROE_RX_PORT, MIKROE_RX_PIN, gpioModeInput, 0);
}

/**************************************************************************//**
 * @brief
 *   USART0 initialization.
 *
 *****************************************************************************/
void initUSART0(void)
{
  // Default asynchronous initializer (115.2 Kbps, 8N1, no flow control)
  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;

  // Route USART0 TX and RX to the board controller TX and RX pins
  GPIO->USARTROUTE[0].TXROUTE = (MIKROE_TX_PORT
      << _GPIO_USART_TXROUTE_PORT_SHIFT)
      | (MIKROE_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE = (MIKROE_RX_PORT
      << _GPIO_USART_RXROUTE_PORT_SHIFT)
      | (MIKROE_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);

  // Enable RX and TX signals now that they have been routed
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN
      | GPIO_USART_ROUTEEN_TXPEN;

  // Configure and enable USART0
  USART_InitAsync(USART0, &init);

  // Enable NVIC USART sources
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);
  NVIC_EnableIRQ(USART0_RX_IRQn);
  NVIC_ClearPendingIRQ(USART0_TX_IRQn);
  NVIC_EnableIRQ(USART0_TX_IRQn);
}

/**************************************************************************//**
 * @brief
 *    The USART0 receive interrupt saves incoming characters.
 *    This function removes \r and \n characters.
 *    Calls global callback if it is defined.
 *    Uses its own buffer to store.
 *    Do not block in this function!
 *
 *****************************************************************************/
void USART0_RX_IRQHandler(void)
{
  // Get the character just received
  input_buffer[input_buffer_index] = USART0->RXDATA;
  if (input_buffer[input_buffer_index] == '\r') {
    //ignore \r character
    input_buffer[input_buffer_index] = 0;
  } else if (input_buffer[input_buffer_index] == '\n') {
    input_buffer[input_buffer_index] = 0;
    if (input_buffer_index > 0) {
      global_cb(input_buffer, ++line_counter);
      memset((void*) input_buffer, 0, IN_BUFFER_SIZE);
      input_buffer_index = 0;
    }
  } else if (input_buffer[input_buffer_index] == '>') {
    global_cb(input_buffer, ++line_counter);
    memset(input_buffer, 0, IN_BUFFER_SIZE);
    input_buffer_index = 0;
  } else if (input_buffer_index < IN_BUFFER_SIZE - 1) {
    input_buffer_index++;
  } else {
    global_cb(input_buffer, ++line_counter);
    memset(input_buffer, 0, IN_BUFFER_SIZE);
    input_buffer_index = 0;
  }
}

/**************************************************************************//**
 * @brief
 *    UART0 transmission ready interrupt handler. Used only for debugging.
 *    Do not block in this function!
 *****************************************************************************/
static void tx_ready_cb(void)
{
  /* Only for debug purposes */
}

/**************************************************************************//**
 * @brief
 *    The USART0 transmit interrupt outputs characters.
 *****************************************************************************/
void USART0_TX_IRQHandler(void)
{
  // Send a previously received character
  if (*output_ptr != '\0')
    USART0->TXDATA = *(output_ptr++);
  else

  {
    tx_ready_cb();
    USART_IntDisable(USART0, USART_IEN_TXBL);
  }
}

/**************************************************************************//**
 * @brief
 *   Initialization of platform driver.
 *
 * @param[in] line_callback
 *   Callback function for new line (and ">" character for special commands).
 *
 *****************************************************************************/
void at_platform_init(ln_cb_t line_callback)
{
  global_cb = line_callback;
  initCMU();
  initGPIO();
  initUSART0();
  USART_IntEnable(USART0, USART_IEN_RXDATAV);
  status = READY;
}

/**************************************************************************//**
 * @brief
 *   Enable UART0 RX interrupt.
 *
 *****************************************************************************/
void at_platform_enable_ir(void)
{
  USART_IntEnable(USART0, USART_IEN_RXDATAV);
}

/**************************************************************************//**
 * @brief
 *   Disable UART0 RX and TX interrupt.
 *
 *****************************************************************************/
void at_platform_disable_ir(void)
{
  USART_IntDisable(USART0, USART_IEN_TXBL);
  USART_IntDisable(USART0, USART_IEN_RXDATAV);
}

/**************************************************************************//**
 * @brief
 *   Timeout handler function.
 *
 * @param[in] handle
 *    Timer handler which called the callback function.
 *
 * @param [in] data
 *    Timer data sent by the caller of callback function.
 *
 *****************************************************************************/
static void timer_cb(sl_sleeptimer_timer_handle_t *handle,
                     void *data)
{
  (void) (data);
  (void) handle;
  status = READY;
  if (NULL != global_cb) {
    global_cb(NULL, 0);
  }
}

/**************************************************************************//**
 * @brief
 *   Check ready status of platform driver.
 *
 *****************************************************************************/
sl_status_t at_platform_check_device_ready(void)
{
  switch (status) {
  case NOT_INITIALIZED:
    return SL_STATUS_NOT_INITIALIZED;
    break;
  case TRANSMIT:
    return SL_STATUS_BUSY;
    break;
  case READY:
    return SL_STATUS_OK;
    break;
  default:
    return SL_STATUS_INVALID_STATE;
  }
}

/**************************************************************************//**
 * @brief
 *   Platform driver send command function.
 *   This function adds \r\n to the command string.
 *   Command string SHALL be allocated until command is sent.
 *   This function uses UART TX interrupt.
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
 *****************************************************************************/
sl_status_t at_platform_send_cmd(volatile uint8_t *cmd,
                                 volatile uint16_t timeout_ms)
{
  sl_status_t st;
  size_t cmd_length = strlen((const char*) cmd);
  if (strstr(cmd, "\r\n") == NULL) {
    if (cmd_length < OUT_BUFFER_SIZE - 2) {
      strcat((char*) cmd, "\r\n");
    } else {
      return SL_STATUS_ALLOCATION_FAILED;
    }
  }
  output_ptr = cmd;
  USART_IntEnable(USART0, USART_IEN_TXBL);
  at_platform_enable_ir();
  USART0->TXDATA = *output_ptr++;

  line_counter = 0;
  status = TRANSMIT;
  st = sl_sleeptimer_restart_timer_ms(&my_timer, timeout_ms, timer_cb,
      (void*) NULL, 0, 0);
  return st;
}

/**************************************************************************//**
 * @brief
 *   Platform driver finish function.
 *   Used to end ongoing communication.
 *   Stops timeout and hardware interrupts.
 *
 *****************************************************************************/
void at_platform_finish_cmd(void)
{
  status = READY;
  at_platform_disable_ir();
  sl_sleeptimer_stop_timer(&my_timer);
}

