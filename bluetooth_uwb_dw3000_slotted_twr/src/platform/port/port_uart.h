/**
 * @file      port.h
 *
 * @brief     port Hardware Abstraction Layer headers file to EFR32BG22
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#define _HAL_CAT2EXP(a, b)           a ## b
#define HAL_CAT2(a, b)               _HAL_CAT2EXP(a, b)
#define _HAL_CAT3EXP(a, b, c)        a ## b ## c
#define HAL_CAT3(a, b, c)            _HAL_CAT3EXP(a, b, c)

// UART
#define HAL_UART_IS_EUART                      1
#if HAL_UART_IS_EUART
#define HAL_UART_TYPE                          EUSART
#define HAL_UART_STOP_BITS                     eusartStopbits1
#define HAL_UART_PARITY                        eusartNoParity
#define HAL_UART_OVERSAMPLING                  eusartOVS4
#else
#define HAL_UART_TYPE                          USART
#define HAL_UART_STOP_BITS                     usartStopbits1
#define HAL_UART_PARITY                        usartNoParity
#define HAL_UART_OVERSAMPLING                  usartOVS4
#endif
#define HAL_UART_NUMBER                        0
#define HAL_UART_PERIPHERAL                    HAL_CAT2(HAL_UART_TYPE, \
                                                        HAL_UART_NUMBER)
#define HAL_UART_BAUDRATE                      115200
// #define HAL_UART_ENABLE_PORT          gpioPortC
// #define HAL_UART_ENABLE_PIN           9
#define HAL_UART_TX_PORT                       gpioPortA
#define HAL_UART_TX_PIN                        5
#define HAL_UART_RX_PORT                       gpioPortA
#define HAL_UART_RX_PIN                        6
#define HAL_UART_SLEEP_RX_IRQ_NBR              HAL_UART_RX_PIN

// BUTTON
#define HAL_IO_BUTTON_ACTIVE_STATE             0
#define HAL_IO_PORT_BUTTON                     gpioPortB
#define HAL_IO_PIN_BUTTON                      2

// ERROR
#define HAL_IO_PORT_LED_ERROR                  gpioPortA
#define HAL_IO_PIN_LED_ERROR                   4

#ifndef HAL_TRACE_DEBUG_LOG_ENABLED
#if DEBUG
#define HAL_TRACE_DEBUG_LOG_ENABLED            1
#else
#define HAL_TRACE_DEBUG_LOG_ENABLED            0
#endif
#endif
#if HAL_TRACE_DEBUG_LOG_ENABLED
#include <stdio.h>
#include <stdarg.h>
#define HAL_TRACE_DEBUG_LOG_LINE_BUFFER_SIZE   64
int deca_uart_transmit(char *tx_buffer, int size);

#endif

static inline void hal_trace_debug_print(const char *format, ...)
{
#if !HAL_TRACE_DEBUG_LOG_ENABLED
  (void)format;
#else
  char print_buffer[HAL_TRACE_DEBUG_LOG_LINE_BUFFER_SIZE];

  va_list args;
  va_start(args, format);

  int len = vsnprintf(print_buffer, sizeof(print_buffer), format, args);
  deca_uart_transmit(print_buffer, len);
  va_end(args);
#endif
}
