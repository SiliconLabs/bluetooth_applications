/*
 * @file      task_listener.c
 * @brief
 *
 * @author Decawave
 *
 * @attention Copyright 2019-2020 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 */
#include <math.h>
#include "deca_dbg.h"
#include "util.h"
#include "port.h"
#include "port_common.h"
#include "usb_uart_tx.h"
#include "listener.h"

// -----------------------------------------------------------------------------
// extern functions to report output data
extern error_e send_to_pc_listener_info(uint8_t *data,
                                        uint8_t size,
                                        uint8_t *ts,
                                        int cfo);

// -----------------------------------------------------------------------------

/* @brief DW3000 RX : Listener RTOS implementation
 *          this is a high-priority task, which will be executed immediately
 *          on reception of waiting Signal. Any task with lower priority will be
 *   interrupted.
 *          No other tasks in the system should have higher priority.
 * */
static void ListenerTask(void const *arg)
{
  int                 head, tail, size;
  listener_info_t     *pListenerInfo;

  while (!(pListenerInfo = getListenerInfoPtr()))
  {
    osDelay(5);
  }

  size = sizeof(pListenerInfo->rxPcktBuf.buf)
         / sizeof(pListenerInfo->rxPcktBuf.buf[0]);

  const osMutexAttr_t thread_mutex_attr = { .attr_bits = osMutexPrioInherit };
  app.listenerTask.MutexId = osMutexNew(&thread_mutex_attr);

  taskENTER_CRITICAL();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);      // Start reception on the Listener
  taskEXIT_CRITICAL();

  do
  {
    osMutexRelease(app.listenerTask.MutexId);

    /* ISR is delivering RxPckt via circ_buf & Signal.
     * This is the fastest method.
     * */
    osThreadFlagsWait(app.listenerTask.Signal, osFlagsWaitAny, 1);
    osMutexAcquire(app.listenerTask.MutexId, 0);

    taskENTER_CRITICAL();
    head = pListenerInfo->rxPcktBuf.head;
    tail = pListenerInfo->rxPcktBuf.tail;
    taskEXIT_CRITICAL();

    if (CIRC_CNT(head, tail, size) > 0) {
      rx_listener_pckt_t *pRx_listener_Pckt =
        &pListenerInfo->rxPcktBuf.buf[tail];

      send_to_pc_listener_info(pRx_listener_Pckt->msg.data,
                               pRx_listener_Pckt->rxDataLen,
                               pRx_listener_Pckt->timeStamp,
                               pRx_listener_Pckt->clock_offset);

      taskENTER_CRITICAL();
      tail = (tail + 1) & (size - 1);
      pListenerInfo->rxPcktBuf.tail = tail;
      taskEXIT_CRITICAL();

      if (app.flushTask.Handle) {
        osThreadFlagsSet(app.flushTask.Handle, app.flushTask.Signal);
      }
    }

    osThreadYield();
  }while (1);

  UNUSED(arg);
}

// -----------------------------------------------------------------------------

/* @brief Setup Listener task, this task will send to received data to UART.
 * Only setup, do not start.
 * */
static void listener_setup_tasks(void)
{
  /* listenerTask is receive the signal from
   * passing signal from RX IRQ to an actual two-way ranging algorithm.
   * It awaiting of an Rx Signal from RX IRQ ISR and decides what to do next in
   *   TWR exchange process
   * */
  CREATE_NEW_TASK(ListenerTask,
                  NULL,
                  "listenerTask",
                  512,
                  PRIO_RxTask,
                  &app.listenerTask.Handle);
  app.listenerTask.Signal = 1;

  if (app.listenerTask.Handle == NULL) {
    error_handler(1, _ERR_Create_Task_Bad);
  }
}

/* @brief Terminate all tasks and timers related to Node functionality, if any
 *        DW3000's RX and IRQ shall be switched off before task termination,
 *        that IRQ will not produce unexpected Signal
 * */
void listener_terminate(void)
{
  TERMINATE_STD_TASK(app.listenerTask);

  listener_process_terminate();
}

/* @fn         listener_helper
 * @brief      this is a service function which starts the
 *             TWR Node functionality
 *             Note: the previous instance of TWR shall be killed
 *             with node_terminate_tasks();
 *
 *             Note: the node_process_init() will allocate the memory of
 *   sizeof(node_info_t)
 *                   from the <b>caller's</b> task stack, see _malloc_r() !
 *
 * */
void listener_helper(void const *argument)
{
  (void) argument;
  error_e   tmp;

  port_disable_dw_irq_and_reset(1);

  /* "RTOS-independent" part : initialization of two-way ranging process */
  tmp = listener_process_init();      /* allocate ListenerInfo */

  if (tmp != _NO_ERR) {
    error_handler(1, tmp);
  }

  listener_setup_tasks();       /**< "RTOS-based" : setup (not start) all
                                 *   necessary tasks for the Node operation. */

  listener_process_start();     /**< IRQ is enabled from MASTER chip and it may
                                *   receive UWB immediately after this point */
}
