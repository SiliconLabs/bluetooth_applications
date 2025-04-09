/*! ----------------------------------------------------------------------------
 * @file    deca_uart.c
 * @brief   HW specific definitions and functions for UART Interface
 *
 * @attention
 *
 * Copyright 2016 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Deepa Gopinath
 */

#include "port_common.h"
#ifdef HAL_UART_PERIPHERAL
#include <string.h>
#include "app.h"
#include "uartdrv.h"
#include "sl_common.h"
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
#include "sl_power_manager.h"
#endif
#include "sl_sleeptimer.h"

#define UART_RX_IRQ_NUMBER                      HAL_CAT3(HAL_UART_TYPE,   \
                                                         HAL_UART_NUMBER, \
                                                         _RX_IRQn)
#define UART_RX_IRQ_HANDLER                     HAL_CAT3(HAL_UART_TYPE,   \
                                                         HAL_UART_NUMBER, \
                                                         _RX_IRQHandler)

#if HAL_UART_IS_EUART
#define UART_INIT_STRUCTURE_T                   UARTDRV_InitEuart_t
#define uart_init(handle, init_data)         UARTDRV_InitEuart(handle, \
                                                               init_data)
#define uart_is_rx_data_available(eusart)    (EUSART_STATUS_RXFL & \
                                              EUSART_StatusGet(eusart))
#define uart_rx_data_get(eusart)             EUSART_Rx(eusart)
#define uart_is_rx_timeout(eusart)           (EUSART_IF_RXTO & EUSART_IntGet( \
                                                eusart))
#define uart_clear_all_interrupt(eusart)     EUSART_IntClear(eusart, \
                                                             _EUSART_IF_MASK)
#define uart_rx_timeout_start(eusart)                  \
  do {                                                 \
    EUSART_Enable(eusart, eusartDisable);              \
    eusart->CFG1 |= EUSART_CFG1_RXTIMEOUT_SEVENFRAMES; \
    EUSART_Enable(eusart, eusartEnable);               \
    EUSART_IntEnable(eusart, EUSART_IF_RXTO);          \
  }while (0)
#define uart_rx_timeout_restart(eusart)
#define uart_blocking_tx(eusart, data)       EUSART_Tx(eusart, data)
#else
#define UART_RX_TIMEOUT_CFG                  (USART_TIMECMP1_TSTOP_RXACT    \
                                              | USART_TIMECMP1_TSTART_RXEOF \
                                              | (0xff <<                    \
                                                 _USART_TIMECMP1_TCMPVAL_SHIFT))
#define UART_INIT_STRUCTURE_T                UARTDRV_InitUart_t
#define uart_init(handle, init_data)         UARTDRV_InitUart(handle, init_data)
#define uart_is_rx_data_available(usart)     (USART_STATUS_RXDATAV & \
                                              USART_StatusGet(usart))
#define uart_rx_data_get(usart)              USART_RxDataGet(usart)
#define uart_is_rx_timeout(usart)            (USART_IF_TCMP1 & USART_IntGet( \
                                                usart))
#define uart_clear_all_interrupt(usart)      USART_IntClear(usart, \
                                                            _USART_IF_MASK)
#define uart_rx_timeout_start(usart)        \
  do {                                      \
    usart->TIMECMP1 = UART_RX_TIMEOUT_CFG;  \
    USART_IntEnable(usart, USART_IF_TCMP1); \
  }while (0)
#define uart_rx_timeout_restart(usart)               \
  do {                                               \
    usart->TIMECMP1 &= ~_USART_TIMECMP1_TSTART_MASK; \
    usart->TIMECMP1 = UART_RX_TIMEOUT_CFG;           \
  }while (0)
#define uart_blocking_tx(usart, data)        USART_Tx(usart, data)
#endif
//
#ifndef HAL_UART_SEND_BLOCKING
#define HAL_UART_SEND_BLOCKING              1
#endif
#if !HAL_UART_SEND_BLOCKING && !defined(HAL_UART_SIMPLE_ASYNCH_SEND)
// Simple asynchronous send expects a static buffer until the send is finished,
//   currently it is not always true.
#define HAL_UART_SIMPLE_ASYNCH_SEND         0
#endif

static void deca_uart_receive_callback(UARTDRV_Handle_t handle,
                                       Ecode_t transferStatus,
                                       uint8_t *data,
                                       UARTDRV_Count_t transferCount);
static inline void deca_uart_update_fifo(uint32_t received_data_count);

static UARTDRV_HandleData_t uart_driver_handle;
static uint32_t uart_driver_receive_items_remaining;
// Define RX and TX buffer data queues
DEFINE_BUF_QUEUE(1, uart_driver_rx_buffer); // continuous receive into a global
                                            //   buffer --> no need for multiple
                                            //   receive queues
DEFINE_BUF_QUEUE(16, uart_driver_tx_buffer);

/* @fn  deca_uart_init
 *
 * @brief Function for initializing the UART module.
 *
 * @param[in] void
 * */
void deca_uart_init(void)
{
  const UART_INIT_STRUCTURE_T init_data = {
    .port = HAL_UART_PERIPHERAL,
    .baudRate = HAL_UART_BAUDRATE,
    .txPort = HAL_UART_TX_PORT,
    .rxPort = HAL_UART_RX_PORT,
    .txPin = HAL_UART_TX_PIN,
    .rxPin = HAL_UART_RX_PIN,
    .uartNum = HAL_UART_NUMBER,
    .stopBits = HAL_UART_STOP_BITS,
    .parity = HAL_UART_PARITY,
    .oversampling = HAL_UART_OVERSAMPLING,
    .fcType = uartdrvFlowControlNone,
    .rxQueue = (void *)&uart_driver_rx_buffer,
    .txQueue = (void *)&uart_driver_tx_buffer,
  };
  uart_init(&uart_driver_handle, &init_data);

#if defined(HAL_UART_ENABLE_PORT) && defined(HAL_UART_ENABLE_PIN)
  GPIO_PinModeSet(HAL_UART_ENABLE_PORT, HAL_UART_ENABLE_PIN, gpioModePushPull,
                  1);
#endif
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
#endif

  // Clear pending RX interrupt flag in NVIC
  NVIC_ClearPendingIRQ(UART_RX_IRQ_NUMBER);
  NVIC_EnableIRQ(UART_RX_IRQ_NUMBER);
  uart_clear_all_interrupt(HAL_UART_PERIPHERAL);

  uart_rx_timeout_start(HAL_UART_PERIPHERAL);
  deca_uart_receive();
}

void deca_uart_close(void)
{
  UARTDRV_Abort(&uart_driver_handle, uartdrvAbortAll);
  UARTDRV_DeInit(&uart_driver_handle);
#if defined(HAL_UART_ENABLE_PORT) && defined(HAL_UART_ENABLE_PIN)
  GPIO_PinModeSet(HAL_UART_ENABLE_PORT, HAL_UART_ENABLE_PIN, gpioModeDisabled,
                  0);
#endif

  // clean internal data
  app.uartRx.head = 0;
  app.uartRx.tail = 0;
  uart_driver_receive_items_remaining = 0;
  memset(&uart_driver_handle, 0, sizeof(uart_driver_handle));

#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
  // Allow deeper sleep state
  sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
#endif
}

/* @fn  deca_uart_transmit
 *
 * @brief Function for transmitting data on UART
 *
 * @param[in] ptr char Pointer that contains the base address of the data
 *   buffer.
 * @param[in] size      int  The size of the data buffet to be sent.
 * @return    Returns a related error code.
 *
 * */
int deca_uart_transmit(const char *ptr, int size)
{
  if ((NULL == ptr) || (size <= 0)) {
    return -1;
  }

#if HAL_UART_SEND_BLOCKING
  // Blocking transmit
  for (int i = 0; i < size; i++)
  {
    uart_blocking_tx(HAL_UART_PERIPHERAL, ptr[i]);
  }
  return 0;
#elif HAL_UART_SIMPLE_ASYNCH_SEND
  return UARTDRV_Transmit(&uart_driver_handle, ptr, size, NULL);
#else
  // Will be used as a WR only buffer.
  // ignore overwrite because it should not happen if printing speed and buffer
  //   size is chosen correctly.
  static data_circ_buf_t tx_fifo;

  // We must provide continuous buffer to UARTDRV_Transmit()
  int sc = 0;
  const size_t remaining_space = sizeof(tx_fifo.buf) - tx_fifo.tail;
  if (size > remaining_space) {
    // copy data to static buffer
    memcpy(&tx_fifo.buf[tx_fifo.tail], ptr, remaining_space);
    sc |= UARTDRV_Transmit(&uart_driver_handle,
                           &tx_fifo.buf[tx_fifo.tail],
                           remaining_space,
                           NULL);
    // Update pointers
    size -= remaining_space;
    ptr += remaining_space;
    tx_fifo.tail = 0;
  }
  if (size) {
    memcpy(&tx_fifo.buf[tx_fifo.tail], ptr, size);
    sc |= UARTDRV_Transmit(&uart_driver_handle,
                           &tx_fifo.buf[tx_fifo.tail],
                           size,
                           NULL);
    tx_fifo.tail = (tx_fifo.tail + size) & (sizeof(uartRx->buf) - 1);
  }
  return sc;
#endif
}

/* @fn  deca_uart_receive
 *
 * @brief Function for receive data from UART and store into rx_buf
 *        (global array).
 *
 * @param[in] void
 * */
void deca_uart_receive(void)
{
  if (uart_driver_handle.peripheral.uart              // initialized
      && (0 == uart_driver_receive_items_remaining)   // receiving is completed
      && (CIRC_SPACE(app.uartRx.head, app.uartRx.tail,
                     sizeof(app.uartRx.buf)) > 0)) {  // have free space
    // Start reception with continuous memory blocks
    int start_idx = app.uartRx.head;
    int tail = app.uartRx.tail;
    uart_driver_receive_items_remaining =
      (start_idx
       < tail) ? (tail - start_idx) : (int)sizeof(app.uartRx.buf) - start_idx;
    UARTDRV_Receive(&uart_driver_handle,
                    &app.uartRx.buf[start_idx],
                    uart_driver_receive_items_remaining,
                    deca_uart_receive_callback);
  }
}

void UART_RX_IRQ_HANDLER(void)
{
  // RX timeout, stop transfer and handle what we got in buffer
  if (uart_is_rx_timeout(HAL_UART_PERIPHERAL)) {
    uart_rx_timeout_restart(HAL_UART_PERIPHERAL);

    // notify upper layer (receive restart not needed)
    int items_remaining = 0;
    if (ECODE_OK
        == DMADRV_TransferRemainingCount(uart_driver_handle.rxDmaCh,
                                         &items_remaining)) {
      deca_uart_update_fifo(
        uart_driver_receive_items_remaining - items_remaining);
    }
  }
  uart_clear_all_interrupt(HAL_UART_PERIPHERAL);
}

static void deca_uart_receive_callback(UARTDRV_Handle_t handle,
                                       Ecode_t transferStatus,
                                       uint8_t *data,
                                       UARTDRV_Count_t transferCount)
{
  (void)handle;
  (void)data;
  (void)transferCount;

  if (ECODE_OK == transferStatus) { // DMA transfer completed
    deca_uart_update_fifo(uart_driver_receive_items_remaining);
    deca_uart_receive();
  }
}

static inline void deca_uart_update_fifo(uint32_t received_data_count)
{
  uart_driver_receive_items_remaining -= received_data_count;

  // data is received directly to app.uartRx.buf just update the head
  app.uartRx.head = (app.uartRx.head + received_data_count)
                    & (sizeof(app.uartRx.buf) - 1);

  // Notify upper layer
  if (app.ctrlTask.Handle) { // RTOS : ctrlTask could be not started yet
    // signal to the ctrl thread : USB data ready
    osThreadFlagsSet(app.ctrlTask.Handle, app.ctrlTask.Signal);
  }
}

#endif //HAL_UART_PERIPHERAL
