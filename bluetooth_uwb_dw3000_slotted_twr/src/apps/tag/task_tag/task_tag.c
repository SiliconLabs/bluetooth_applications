/*
 * @file      task_tag.c
 * @brief     Decawave Application Layer
 *            RTOS tag implementation
 *
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include "task_tag.h"
#include "util.h"
#include "app.h"
#include "port_common.h"
#include "tag.h"

// -----------------------------------------------------------------------------

#define BLINK_PERIOD_MS            (500)    /* range init phase - Blink send
                                             *   period, ms */

// -----------------------------------------------------------------------------

/*
 * @brief
 *     The thread is initiating the transmission of the blink
 *     on reception of app.blinkTask.Signal
 *
 * */
static void
BlinkTask(void const *arg)
{
  tag_info_t     *p;

  do{
    osDelay(100);
  }while (!(p = getTagInfoPtr()));     // wait for initialisation of psTagInfo

  do {
    osMutexRelease(app.blinkTask.MutexId);

    osThreadFlagsWait(app.blinkTask.Signal, osFlagsWaitAny, osWaitForever);

    osMutexAcquire(app.blinkTask.MutexId, 0);        // we do not want the task
                                                     //   can be deleted in the
                                                     //   middle of operation

    if (app.DwSpiReady == DW_SPI_SLEEPING) {
      taskENTER_CRITICAL();
      tag_wakeup_dw3000_blink_poll(p);
      taskEXIT_CRITICAL();
    } else {
      taskENTER_CRITICAL();
      dwt_restoreconfig();       // restore configuration which has not been
                                 //   saved in AON
      app.DwEnterSleep = DW_NOT_SLEEPING;
      taskEXIT_CRITICAL();

      initiator_send_blink(p);
    }
  }while (1);

  UNUSED(arg);
}

/*
 * @brief
 *     The thread is initiating the TWR sequence
 *  on reception of .signal.twrTxPoll
 *
 * */
static void
TagPollTask(void const *arg)
{
  tag_info_t     *p = getTagInfoPtr();

  do{
    osDelay(100);
  }while (!(p = getTagInfoPtr()));     // wait for initialisation of pTwrInfo

  do {
    osMutexRelease(app.pollTask.MutexId);

    osThreadFlagsWait(app.pollTask.Signal, osFlagsWaitAny, osWaitForever);

    osMutexAcquire(app.pollTask.MutexId, 0);

    if (app.DwSpiReady == DW_SPI_SLEEPING) {
      taskENTER_CRITICAL();
      tag_wakeup_dw3000_blink_poll(p);
      taskEXIT_CRITICAL();
    } else {
      taskENTER_CRITICAL();
      dwt_restoreconfig();       // restore configuration which has not been
                                 //   saved in AON
      app.DwEnterSleep = DW_NOT_SLEEPING;
      taskEXIT_CRITICAL();

      if (p->faultyRangesCnt < app.pConfig->s.faultyRanges) {
        initiator_send_poll(p);
      } else {
        rtc_disable_irq();

        /* This will restart Tag task completely from discovery phase */
        xEventGroupSetBits(app.xStartTaskEvent, Ev_Tag_Task);
      }
    }
  }while (1);

  UNUSED(arg);
}

/* @brief DW3000 RX : RTOS implementation
 *
 * */
static void
TagRxTask(void const *arg)
{
  error_e     ret;
  uint16_t    head, tail;

  tag_info_t  *ptwrInfo;

  do{
    osDelay(10);
  }while (!(ptwrInfo = getTagInfoPtr()));     // wait for initialisation of
                                              //   pTwrInfo

  int size = sizeof(ptwrInfo->rxPcktBuf.buf)
             / sizeof(ptwrInfo->rxPcktBuf.buf[0]);

  do {
    osMutexRelease(app.rxTask.MutexId);

    osThreadFlagsWait(app.rxTask.Signal, osFlagsWaitAny, osWaitForever);

    osMutexAcquire(app.rxTask.MutexId, 0);

    taskENTER_CRITICAL();
    head = ptwrInfo->rxPcktBuf.head;
    tail = ptwrInfo->rxPcktBuf.tail;
    taskEXIT_CRITICAL();

    /* We are using circular buffer + Signal to safely deliver packets from ISR
     *   to APP */
    if (CIRC_CNT(head, tail, size) > 0) {
      rx_pckt_t_t       *prxPckt = &ptwrInfo->rxPcktBuf.buf[tail];
      twr_res_ext_t   p;

      ret = twr_initiator_algorithm_rx(prxPckt,
                                       ptwrInfo); /**< Run bare
                                                   * twr_initiator_algorithm
                                                   */

      switch (ret)
      {
        case _NO_Err_Response:
          ptwrInfo->faultyRangesCnt = 0;
          p.addr = AR2U16(ptwrInfo->env.tagAddr);
          p.node_addr = AR2U16(ptwrInfo->env.nodeAddr);
          p.rNum = prxPckt->msg.respExtMsg.resp.rNum;
          p.x_cm = (int16_t)AR2U16(prxPckt->msg.respExtMsg.resp.x_cm);
          p.y_cm = (int16_t)AR2U16(prxPckt->msg.respExtMsg.resp.y_cm);
          p.clkOffset_pphm = (int16_t)AR2U16(
            prxPckt->msg.respExtMsg.resp.clkOffset_pphm); /* Crystal Clock
                                                           * offset value
                                                           * reported back
                                                           * from the Node
                                                           */

          {      // XTAL trimming will be performed after sending of Final.
            /* Instead of using a clkOffset_pphm from Response, which calculated
             *   by Node based on distances measurements,
             * the more precise and direct method of adjusting clock offset
             *   using
             * carrier integrator counter value will be used.*/
            float co_ppm;
            co_ppm = dwt_readclockoffset();

            /* save the offset value for future apply after Final message been
             *   sent */
            ptwrInfo->clkOffset_pphm =
              (int)(co_ppm * (CLOCK_OFFSET_PPM_TO_RATIO * 1e6 * 100));
          }
          break;
        case _ERR_DelayedTX_Late:
        case _ERR_Not_Twr_Frame:
        case _NO_Err_Can_Sleep:
#if (DW_TAG_NOT_SLEEPING == 0)
          app.DwCanSleepInIRQ = DW_CAN_SLEEP_APP;
#endif
          break;
        default:
          break;
      }

#if (DW_TAG_NOT_SLEEPING == 0)
      if ((app.DwCanSleepInIRQ == DW_CAN_SLEEP_APP)
          && (app.DwEnterSleep != DW_IS_SLEEPING_IRQ)) {
        taskENTER_CRITICAL();
        app.DwEnterSleep = DW_IS_SLEEPING_RX;
        dwt_entersleep(DWT_DW_IDLE_RC);               // manual sleeping
        app.DwSpiReady = DW_SPI_SLEEPING;
        taskEXIT_CRITICAL();
      }
#endif

      taskENTER_CRITICAL();
      tail = (tail + 1) & (size - 1);
      ptwrInfo->rxPcktBuf.tail = tail;
      taskEXIT_CRITICAL();

      /* ready to serve next raw reception */

      /* Report previous range/coordinates/offset back to UART/USB */
      if ((app.pConfig->s.reportLevel) && (ret == _NO_Err_Response)) {
//                send_to_pc_tag_location(&p); //TODO: tag can report the
//   distance to the PC from the previous location
        // TODO: also tag can calculate the location from SSTWR
      }

      UNUSED(p);
    }

    osThreadYield();
  }while (1);

  UNUSED(arg);
}

/* @brief Setup TWR tasks and timers for discovery phase.
 *         - blinking timer
 *         - blinking task
 *          - twr polling task
 *         - rx task
 * Only setup, do not start.
 * */
static void tag_setup_tasks(void)
{
// The RTC Timer is set as a part of tag_process_init()
// The RTC timer will produce Signals to activate either BlinkTask or
//   TagPollTask
// That will help with sending of first Poll on the correct slot.
//
  const osMutexAttr_t thread_mutex_attr = { .attr_bits = osMutexPrioInherit };

  /* Blinking thread for discovery phase of the Anchor until reception of Range
   *   Init */
  CREATE_NEW_TASK(BlinkTask,
                  NULL,
                  "blinkTask",
                  256,
                  PRIO_BlinkTask,
                  &app.blinkTask.Handle);
  app.blinkTask.MutexId = osMutexNew(&thread_mutex_attr);
  app.blinkTask.Signal = 1;

  /* This will be the main poll thread for the Tag after discovery completed
   * Do not reduce the stack size for this thread.
   * */
  CREATE_NEW_TASK(TagPollTask,
                  NULL,
                  "pollTask",
                  256,
                  PRIO_TagPollTask,
                  &app.pollTask.Handle);
  app.pollTask.MutexId = osMutexNew(&thread_mutex_attr);
  app.pollTask.Signal = 1;

  /* rxThread is passing signal from RX IRQ to an actual two-way ranging
   *   algorithm.
   * It awaiting of Rx Signal from RX IRQ ISR and decides what to do next in TWR
   *   exchange process.
   * Do not reduce the stack size for this thread.
   * */
  CREATE_NEW_TASK(TagRxTask,
                  NULL,
                  "rxTask",
                  384,
                  PRIO_TagRxTask,
                  &app.rxTask.Handle);
  app.rxTask.MutexId = osMutexNew(&thread_mutex_attr);
  app.rxTask.Signal = 1;

  if ((app.blinkTask.Handle == NULL)   \
      || (app.pollTask.Handle == NULL) \
      || (app.rxTask.Handle == NULL)) {
    error_handler(1, _ERR_Create_Task_Bad);
  }
}

// -----------------------------------------------------------------------------

/* @brief
 *      Kill all task and timers related to tag/TWR if any
 *      DW3000's RX and IRQ shall be switched off before task termination,
 *      that IRQ will not produce unexpected Signal
 * */
void tag_terminate(void)
{
  rtc_disable_irq();

  TERMINATE_STD_TASK(app.blinkTask);

  TERMINATE_STD_TASK(app.imuTask);

  TERMINATE_STD_TASK(app.rxTask);

  TERMINATE_STD_TASK(app.pollTask);

  tag_process_terminate();      // de-allocate Tag RAM Resources
}

/* @fn      tag_helper
 * @brief   this is a service function which starts the Tag
 *          top-level  application.
 *    Note: If the dynamic memory allocation is used, then the
 *          tag_process_init() will allocate the memory of sizeof(tag_info_t)
 *          from the <b>caller's</b> task stack, see _malloc_r() !
 * */
void tag_helper(void const *argument)
{
  (void) argument;
  error_e     tmp;

  port_disable_dw_irq_and_reset(1);

  taskENTER_CRITICAL();      /**< When the app will setup RTOS tasks, then if
                              *   task has a higher priority,
                              *     the kernel will start it immediately, thus
                              *   we need to stop the scheduler.*/
  set_dw_spi_fast_rate();

  tag_setup_tasks();          /**< "RTOS-based" : setup all RTOS tasks. */

  /* "RTOS-independent" part : initialisation of two-way ranging process */
  tmp = tag_process_init();

  if (tmp != _NO_ERR) {
    error_handler(1, tmp);
  }

  tag_process_start();

  taskEXIT_CRITICAL();      /**< all RTOS tasks can be scheduled */
}
