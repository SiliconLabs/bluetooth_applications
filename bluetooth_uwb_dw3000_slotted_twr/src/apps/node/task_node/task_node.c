/**
 * @file    task_node.c
 * @brief    Decawave Application level
 *             collection of TWR bare-metal functions for a Node
 *
 * @author Decawave
 *
 * @attention Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 * =============================================================================
 * =                                                                           =
 * =                                                                           =
 * =                D E C A W A V E    C O N F I D E N T I A L                 =
 * =                                                                           =
 * =                                                                           =
 * =============================================================================
 *
 * This software contains Decawave confidential information and techniques,
 * subject to licence and non-disclosure agreements.  No part of this software
 * package may be revealed to any third-party without the express permission of
 * Decawave Ltd.
 *
 * =============================================================================
 */
#include <math.h>

#include "task_node.h"
#include "deca_dbg.h"
#include "util.h"
#include "port_common.h"
#include "usb_uart_tx.h"
#include "node.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846f
#endif

// -----------------------------------------------------------------------------
// extern functions to report output data
extern void send_to_pc_diag_acc(rx_mail_t *pRxMailPckt);
extern void send_to_pc_twr(result_t *pRes);
extern void trilat_extension_node_put(result_t *pRes);

// -----------------------------------------------------------------------------

/*
 * @brief Node RTOS implementation
 *          This is normal priority task, which is awaiting for
 *          a mail with a new reception of raw data from a tag.
 *
 *          On reception of a raw data mail it's performing the calculation
 *          of range and PDOA and reports the results.
 * */
static void CalcTask(void const *arg)
{
  node_info_t  *pNodeInfo;

  while (!(pNodeInfo = getNodeInfoPtr()))
  {
    osDelay(5);
  }

  const osMutexAttr_t thread_mutex_attr = { .attr_bits = osMutexPrioInherit };
  app.calcTask.MutexId = osMutexNew(&thread_mutex_attr);

  do {
    osMutexRelease(app.calcTask.MutexId);

    rx_mail_t *pRxMailPckt = NULL;
    osStatus_t sts = osMessageQueueGet(app.rxPcktQueue_q_id,
                                       &pRxMailPckt,
                                       NULL,
                                       osWaitForever);
    osMutexAcquire(app.calcTask.MutexId, 0);

    // Received the mail in RxPckt structure format : it includes all necessary
    //   raw data to
    // calculate range, phase difference, (diagnostics and accumulators if any),
    //   etc.
    if ((osOK != sts) || (NULL == pRxMailPckt)) {
      continue;
    }

    uint16_t slot = pRxMailPckt->tag->slot;

    result_t *pRes = &pNodeInfo->result[slot];

    int32_t tofi = INVALID_TOF;

    {        // calculate the time of flight and phase difference
      memcpy(pRes, &pRxMailPckt->res, sizeof(result_t));

      int64_t     Rb, Da, Ra, Db;

      uint64_t    tagPollTxTime;
      uint64_t    tagRespRxTime;
      uint64_t    tagFinalTxTime;

      uint64_t    nodeRespTxTime;
      uint64_t    nodePollRxTime;
      uint64_t    nodeFinalRxTime;

      float    RaRbxDaDb = 0;
      float    RbyDb = 0;
      float    RayDa = 0;

      TS2U64_MEMCPY(tagRespRxTime, pRxMailPckt->tagRespRx_ts);
      TS2U64_MEMCPY(tagPollTxTime, pRxMailPckt->tagPollTx_ts);
      TS2U64_MEMCPY(tagFinalTxTime, pRxMailPckt->tagFinalTx_ts);

      TS2U64_MEMCPY(nodeRespTxTime, pRxMailPckt->nodeRespTx_ts);
      TS2U64_MEMCPY(nodePollRxTime, pRxMailPckt->nodePollRx_ts);
      TS2U64_MEMCPY(nodeFinalRxTime, pRxMailPckt->nodeFinalRx_ts);

      pRes->pdoa_raw_deg =
        ((float)pRes->finalPDOA.pdoa / (1 << 11)) * 180 / M_PI;
      pRes->pdoa_raw_degP =
        ((float)pRes->pollPDOA.pdoa / (1 << 11)) * 180 / M_PI;

      Ra = (int64_t)((tagRespRxTime - tagPollTxTime) & MASK_40BIT);
      Db = (int64_t)((nodeRespTxTime - nodePollRxTime) & MASK_40BIT);

      Rb = (int64_t)((nodeFinalRxTime - nodeRespTxTime) & MASK_40BIT);
      Da = (int64_t)((tagFinalTxTime - tagRespRxTime) & MASK_40BIT);

      RaRbxDaDb = (((float)Ra)) * (((float)Rb)) - (((float)Da)) * (((float)Db));

      RbyDb = ((float)Rb + (float)Db);

      RayDa = ((float)Ra + (float)Da);

      tofi = (int32_t) (RaRbxDaDb / (RbyDb + RayDa));

      // Compute clock offset, in parts per million
      pRes->clockOffset_pphm = ((((float)Ra / 2) - (float)tofi) / (float)Db)
                               - ((((float)Rb / 2) - (float)tofi) / (float)Da);

      pRes->clockOffset_pphm *= 1e8;        // hundreds of ppm
    }

    /* reportTOF : correct Range bias if needed */
    float r_m;

    if (tof2range(app.pConfig, &r_m, tofi) == _NO_ERR) {
      pRes->dist_cm = r_m * 100.0;

      pdoa2XY(pRes, app.pConfig->dwt_config.chan);

      if (app.trilatTask.Handle) {
        trilat_extension_node_put(pRes);
      } else {
        /* Transmit the report to the PC std TWR reports */
        send_to_pc_twr(pRes);
        send_to_pc_diag_acc(pRxMailPckt);               // This maybe a slow
                                                        //   blocking sending of
                                                        //   large amount of
                                                        //   data
                                                        // FlushTask should have
                                                        //   higher priority
                                                        //   than CalckTask
      }
    } else {
      pRes->dist_cm = (float)(0xDEADBEEF);
      pRes->x_cm = (float)(0xDEADBEEF);
      pRes->y_cm = (float)(0xDEADBEEF);

      if (app.pConfig->s.debugEn) {
        const char text[] = "Bad Range \r\n";
        port_tx_msg((uint8_t *)text, strlen(text));
      }
    }

    /* remove the message from the mail queue */
    osMemoryPoolFree(app.rxPcktPool_q_id, pRxMailPckt);
  }while (1);

  UNUSED(arg);
}

/* @brief DW3000 RX : Node RTOS implementation
 *          this is a high-priority task, which will be executed immediately
 *          on reception of waiting Signal. Any task with lower priority will be
 *   interrupted.
 *          No other tasks in the system should have higher priority.
 * */
static void RxTask(void const *arg)
{
  int            head, tail, size, tmp;
  error_e     ret;

  node_info_t  *pNodeInfo;

  while (!(pNodeInfo = getNodeInfoPtr()))
  {
    osDelay(5);
  }

  size = sizeof(pNodeInfo->rxPcktBuf.buf) / sizeof(pNodeInfo->rxPcktBuf.buf[0]);

  const osMutexAttr_t thread_mutex_attr = { .attr_bits = osMutexPrioInherit };
  app.rxTask.MutexId = osMutexNew(&thread_mutex_attr);

  taskENTER_CRITICAL();
  dwt_rxenable(DWT_START_RX_IMMEDIATE);      // Start reception on the Node
  taskEXIT_CRITICAL();

  do{
    osMutexRelease(app.rxTask.MutexId);

    /* ISR is delivering RxPckt via circ_buf & Signal.
     * This is the fastest method.
     * */
    osThreadFlagsWait(app.rxTask.Signal, osFlagsWaitAny, osWaitForever);

    osMutexAcquire(app.rxTask.MutexId, 0);

    taskENTER_CRITICAL();
    head = pNodeInfo->rxPcktBuf.head;
    tail = pNodeInfo->rxPcktBuf.tail;
    taskEXIT_CRITICAL();

    if (CIRC_CNT(head, tail, size) > 0) {
      rx_pckt_t *pRxPckt = &pNodeInfo->rxPcktBuf.buf[tail];

      ret = twr_responder_algorithm_rx(pRxPckt, pNodeInfo);    /**< Run the
                                                                *   bare-metal
                                                                *   algorithm
                                                                */

      if (ret == _NO_Err_Final) {
        /* Mail the RxPckt to someone : another RTOS thread is awaiting for it
         */
        final_accel_t *pFinalMsg = &pRxPckt->msg.finalMsg.final;
        rx_mail_t *pMail = osMemoryPoolAlloc(app.rxPcktPool_q_id,
                                             10); //   timeout 10ms

        if (pMail) {
          if (app.pConfig->s.diagEn) {
            memcpy(&pMail->diag_dw3000, &pRxPckt->diag_dw3000,
                   sizeof(pMail->diag_dw3000));
          }

          if (app.pConfig->s.accEn) {
//                        memcpy(&pMail->acc,  &pRxPckt->acc[0],
//   sizeof(pMail->acc));
          }

          /* Tag's information */
          pMail->tag = pRxPckt->tag;
          pMail->res.addr16 = pRxPckt->tag->addr16;
          pMail->res.rangeNum = pFinalMsg->rNum;

          /* PDoA information */
          memcpy(&pMail->res.pollPDOA, &pNodeInfo->pollPDOA,
                 sizeof(pMail->res.pollPDOA));
          memcpy(&pMail->res.finalPDOA, &pNodeInfo->finalPDOA,
                 sizeof(pMail->res.finalPDOA));

          /* Accelerometer state of the tag: from pFinalMsg */
          pMail->res.flag = (uint16_t)(pFinalMsg->flag);
          pMail->res.acc_x = (int16_t)(AR2U16(pFinalMsg->acc_x));
          pMail->res.acc_y = (int16_t)(AR2U16(pFinalMsg->acc_y));
          pMail->res.acc_z = (int16_t)(AR2U16(pFinalMsg->acc_z));

          /* Convert raw reading of temperature using calibrated values for
           *   chips.
           * Note, the calibrated values available only after dwt_initialize()
           */
          // float  temp;
          // temp = (uint8_t)pRxPckt->temperature;
          // temp = 23.0 + (float)(temp - pNodeInfo->Tmeas)*1.14;   /**<
          //   UserManual 2.10: formula for conversion */
          // pMail->res.tMaster_C = (int8_t)(temp);

          /* Add flags for PC application */
          if (app.pConfig->s.pdoaOffset_deg == 0) {
            pMail->res.flag |= RES_FLAG_PDOA_OFFSET_ZERO_BIT;
          }
          if (app.pConfig->s.rngOffset_mm == 0) {
            pMail->res.flag |= RES_FLAG_RANGE_OFFSET_ZERO_BIT;
          }

          /* Tag's ranging times: from pFinalMsg */
          TS2TS_MEMCPY(pMail->tagPollTx_ts, pFinalMsg->pollTx_ts);
          TS2TS_MEMCPY(pMail->tagRespRx_ts, pFinalMsg->responseRx_ts);
          TS2TS_MEMCPY(pMail->tagFinalTx_ts, pFinalMsg->finalTx_ts);

          /* Node's exchange timings for current Ranging */
          TS2TS_MEMCPY(pMail->nodePollRx_ts, pNodeInfo->nodePollRx_ts);
          TS2TS_MEMCPY(pMail->nodeRespTx_ts, pNodeInfo->nodeRespTx_ts);
          TS2TS_MEMCPY(pMail->nodeFinalRx_ts, pRxPckt->timeStamp);

          /* Final message reception time WRT to Node's Superframe (slots are
           *   from RTC)*/
          tmp = pNodeInfo->pollRtcTimeStamp
                - pNodeInfo->gRtcSFrameZeroCnt;  //  NRF:RTC is CC up-counter

          if (tmp < 0) {
            tmp += RTC_WKUP_CNT_OVFLW;
          }

          pMail->res.resTime_us = (tmp * WKUP_RESOLUTION_NS) / 1000;

          if (osMessageQueuePut(app.rxPcktQueue_q_id, &pMail, 0, 0) != osOK) {
            error_handler(1, _ERR_Cannot_Send_Mail);
          }
        } else {
          error_handler(0, _ERR_Cannot_Alloc_Mail);           // non-blocking
                                                              //   error : no
                                                              //   memory :
                                                              //   resources,
                                                              //   etc. This can
                                                              //   happen when
                                                              // tags exchange
                                                              //   received, but
                                                              //   previous
                                                              //   exchange has
                                                              //   not been
                                                              //   reported to
                                                              //   the host yet
                                                              // For example on
                                                              //   Accumulators
                                                              //   readings
        }
      }

      if (ret == _NO_Err_New_Tag) {
        if (addTagToDList(pNodeInfo->newTag_addr64)) {
          signal_to_pc_new_tag_discovered(pNodeInfo->newTag_addr64);

#if DEBUG_NO_GUI
          // this will add tag with assigning of automatic adresss if GUI
          //   application is not in use:
          // this guarantee the tag would have a unique address
          // do not use this when GUI is connected
          add_tag_to_knownTagList(pNodeInfo->newTag_addr64,
                                  (uint16_t)pNodeInfo->newTag_addr64 & 0xFFFF,
                                  1,
                                  1,
                                  1);
#endif
        }

        pNodeInfo->newTag_addr64 = 0;
      }

      taskENTER_CRITICAL();
      tail = (tail + 1) & (size - 1);
      pNodeInfo->rxPcktBuf.tail = tail;
      taskEXIT_CRITICAL();

      if ((ret != _NO_ERR)) {
        /* If the Node is performing a Tx, then the receiver will be enabled
         * after the Tx automatically and twr_responder_algorithm_rx reports
         *   "_NO_ERR".
         *
         * Otherwise always re-enable the receiver : unknown frame, not expected
         *   tag,
         * final message, _Err_DelayedTX_Late, etc.
         * */
        taskENTER_CRITICAL();
        dwt_setrxtimeout(0);
        dwt_rxenable(DWT_START_RX_IMMEDIATE);         // re-enable receiver
                                                      //   again - no timeout
        taskEXIT_CRITICAL();
      }

      /* ready to serve next raw reception */
    }

    osThreadYield();
  }while (1);

  UNUSED(arg);
}

// -----------------------------------------------------------------------------

/* @brief Setup TWR tasks and timers for discovery phase.
 *         - blinking timer
 *         - blinking task
 *          - twr polling task
 *         - rx task
 * Only setup, do not start.
 * */
static void node_setup_tasks(void)
{
  CREATE_NEW_TASK(CalcTask,
                  NULL,
                  "calcTask",
                  512,
                  PRIO_CalcTask,
                  &app.calcTask.Handle);
  app.calcTask.Signal = 1;

  /* rxTask is receive the signal from
   * passing signal from RX IRQ to an actual two-way ranging algorithm.
   * It awaiting of an Rx Signal from RX IRQ ISR and decides what to do next in
   *   TWR exchange process
   * */
  CREATE_NEW_TASK(RxTask, NULL, "rxTask", 512, PRIO_RxTask, &app.rxTask.Handle);
  app.rxTask.Signal = 1;

  if ((app.rxTask.Handle == NULL) \
      || (app.calcTask.Handle == NULL)) {
    /*(app.imuTask.Handle == NULL))*/
    error_handler(1, _ERR_Create_Task_Bad);
  }
}

/* @brief Terminate all tasks and timers related to Node functionality, if any
 *        DW3000's RX and IRQ shall be switched off before task termination,
 *        that IRQ will not produce unexpected Signal
 * */
void node_terminate(void)
{
  TERMINATE_STD_TASK(app.rxTask);

  // TODO: need more elegant way to deallocate the mail queue
  osDelay(100 / portTICK_PERIOD_MS);   // wait 100ms thus the calcTask should
                                       //   receive all mail sent and delete
                                       //   them

  TERMINATE_STD_TASK(app.calcTask);

  TERMINATE_STD_TASK(app.imuTask);

  node_process_terminate();
}

/* @fn         node_helper
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
void node_helper(void const *argument)
{
  (void) argument;
  error_e   tmp;

  port_disable_dw_irq_and_reset(1);

  initDList();      /**< The List of Discovered Tags during listening of the air
                     */

  /* "RTOS-independent" part : initialization of two-way ranging process */
  tmp = node_process_init();      /* allocate NodeInfo */

  if (tmp != _NO_ERR) {
    error_handler(1, tmp);
  }

  node_setup_tasks();       /**< "RTOS-based" : setup (not start) all necessary
                             *   tasks for the Node operation. */

  node_process_start();     /**< IRQ is enabled from MASTER chip and it may
                             *   receive UWB immediately after this point
                             */
}
