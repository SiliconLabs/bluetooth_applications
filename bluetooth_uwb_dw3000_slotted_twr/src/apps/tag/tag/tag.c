/**
 * @file    tag.c
 * @brief    Decawave Application Layer
 *             TWR functions collection
 *
 * @attention
 *
 * Copyright 2016-2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author
 *
 * Decawave
 */

/* Includes */
#include "tag.h"

#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "util.h"

#include "deca_device_api.h"

#include "assert.h"

#include "deca_dbg.h"
#include "common_n.h"
#include "config.h"

// -----------------------------------------------------------------------------
// Definitions

#ifndef TWR_ENTER_CRITICAL
#define TWR_ENTER_CRITICAL                  taskENTER_CRITICAL
#endif

#ifndef TWR_EXIT_CRITICAL
#define TWR_EXIT_CRITICAL                   taskEXIT_CRITICAL
#endif

/* Two parameters will be sent to the Tag in the Ranging Config message:
 * First, is the ~delay after Poll_Tx, when Tag needs to go to Rx, and the
 *   second is
 * the ~delay between Poll_Tx's and Final_Tx's R-marks.
 * Tag hardware shall be capable to meet the requested timings.
 * Below defined limits, used to specify Tag's maximum HW capability.
 *
 * That limits depend on the MCU speed, code optimization, latency, introduced
 *   by
 * the application architecture, especially number of reading/writing from/to
 *   DW1000 and RTOS latencies.
 * Timings can be improved more (decreased) by placing a ranging-dependent
 * functionality below RTOS, (so app will not have a delay, introduced by RTOS
 *   itself).
 * However, current realization is not optimized for that.
 * */
#define MIN_RESPONSE_CAPABILITY_US          (100)   /**< time when Tag can be
                                                     *   ready to receive the
                                                     *   Respose after its Poll
                                                     */
#define MIN_POLL_TX_FINAL_TX_CAPABILITY_US  (1500)  /**< time for tag to
                                                     *   transmit Poll_TX and
                                                     *   then Final_TX)
                                                     */

/* For best PDOA performance the clock offset between PDOA node and the tag
 * should be >5ppm (but less than 30ppm).
 * Because the initial default value can varry
 * Set it to be in the range 5..8ppm
 * In case Tag's crystal is very off, it also will be trimmed to stay in the |5
 *   .. 6| ppm range,
 * as defined below:
 * */
#define TARGET_XTAL_OFFSET_VALUE_PPHM_MIN   (500)
#define TARGET_XTAL_OFFSET_VALUE_PPHM_MAX   (800)

/* The typical trimming range of DW3000 shield (with 2pF external caps is ~48ppm
 *   (-30ppm to +18ppm) over all steps */
#define AVG_TRIM_PER_PPHM \
  ((XTAL_TRIM_BIT_MASK + 1) / 48.0f / 100) /* Trimming per 1 pphm */

// -----------------------------------------------------------------------------
// TWR structure holds all TWR data
// #define TAG_STATIC_TWRINFO
#ifdef TAG_STATIC_TWRINFO
// static ("safe") implementation
static tag_info_t    sTagInfo;
static tag_info_t   *psTagInfo = &sTagInfo;

#else
// dynamic allocation of TwrInfo
static tag_info_t    *psTagInfo = NULL;

#define TAG_MALLOC  pvPortMalloc
#define TAG_FREE    vPortFree

#endif

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Local functions prototypes
static void rtcWakeUpTimerEventCallback_tag(void);
static void tag_configure_rtc_wakeup_ns(uint32_t);

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Implementation

// -----------------------------------------------------------------------------
// Support section

/*
 * @brief     get pointer to the twrInfo structure
 * */
tag_info_t *
getTagInfoPtr(void)
{
#ifdef TAG_STATIC_TWRINFO
  return (&psTagInfo);
#else
  return (psTagInfo);
#endif
}

/*
 * @brief   ISR level (need to be protected if called from APP level)
 * @param   twr_info_t *pTwrInfo
 *
 * @note    twr_info_t structure has two members xtaltrim - current trimmed
 *   value and
 *          clkOffset_pphmm, these change the DW3000 system clock and shall be
 *   applied
 *          when DW3000 is not in active Send/Receive state.
 * */
void trim_tag_proc(tag_info_t *pTwrInfo)
{
  unsigned tmp = abs(pTwrInfo->clkOffset_pphm);

  if ((tmp > TARGET_XTAL_OFFSET_VALUE_PPHM_MAX)
      || (tmp < TARGET_XTAL_OFFSET_VALUE_PPHM_MIN)) {
    int8_t tmp8 = pTwrInfo->xtaltrim;
    tmp8 -=
      (int8_t)(((TARGET_XTAL_OFFSET_VALUE_PPHM_MAX
                 + TARGET_XTAL_OFFSET_VALUE_PPHM_MIN) / 2
                + pTwrInfo->clkOffset_pphm)
               * AVG_TRIM_PER_PPHM); /*  (TARGET_XTAL_OFFSET_VALUE_PPHM_MAX
                                      *   +
                                      *   TARGET_XTAL_OFFSET_VALUE_PPHM_MIN)/2
                                      */
    pTwrInfo->xtaltrim = (uint8_t)(XTAL_TRIM_BIT_MASK & tmp8);

    /* Configure new Crystal Offset value */
    dwt_setxtaltrim(pTwrInfo->xtaltrim);
  }
}

#if (DIAG_READ_SUPPORT == 1)

/*
 * @brief    ISR layer
 *     read full diagnostic data form the received frame from the two DW1000s
 *     offset 0 / 1
 * */
static int
read_full_diagnostics(rx_pckt_t_t *prxPckt,
                      uint32_t   status)
{
  uint16_t     fpIndex;
  diag_v5_t    *p = &prxPckt->diagnostics;

  p->header = DWT_DIAGNOSTIC_LOG_REV_5;

  memcpy(p->r0F, (uint8_t *) &status, 4);                       // copy 4bytes
                                                                //   of status
                                                                //   (saved on
                                                                //   entry to
                                                                //   ISR)
  dwt_readfromdevice(RX_FINFO_ID, 4, 5, (uint8_t *)(p + 5));    // read MSB from
                                                                //   status and
                                                                //   4byte frame
                                                                //   info
  dwt_readfromdevice(RX_FQUAL_ID, 0, 17, (uint8_t *)(p->r12));  // read 17 bytes
                                                                //   of
                                                                //   diagnostic
                                                                //   data from
                                                                //   0x12,13,14

  memcpy((uint8_t *)p->r15, prxPckt->rxTimeStamp, TS_40B_SIZE); // copy TS
  dwt_readfromdevice(RX_TIME_ID, RX_TIME_FP_INDEX_OFFSET, 9,
                     (uint8_t *)(p->r15 + 5)); // 2FP, 2Diag, 5TSraw

  // Calculate the First Path Index ((LDE0 + LDE1 << 8) / 64)
  fpIndex = (*((uint8_t *)(p + 32)) >> 6) + (*((uint8_t *)(p + 33)) << 2);

  fpIndex = fpIndex * 4 + 1;                 // get location in the accumulator

  // printf("%d FP index %02x %02x %i %i\n", offset, *((uint8_t*)(p+32)),
  //   *((uint8_t*)(p+33)), fpIndex, (fpIndex-1)>>2);
  // Read CIR for the First Path + 3 More samples (4*4 = 16)
  dwt_readaccdata(p->r25, 17, fpIndex - 1); // read 1 extra as first will be
                                            //   dummy byte
  dwt_readfromdevice(LDE_IF_ID, LDE_PPINDX_OFFSET, 2, p->r2E);
  dwt_readfromdevice(DRX_CONF_ID, 0x28, 4, p->r27);
  dwt_readfromdevice(LDE_IF_ID, LDE_PPAMPL_OFFSET, 2, p->r2E2);

  return (int) fpIndex;
}

#endif

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//    DW3000 callbacks section :
//    if RTOS, the preemption priority of the dwt_isr() shall be such, that
//    allows signal to the thread.

/* @brief    ISR layer
 *             Real-time TWR application Tx callback
 *            to be called from dwt_isr()
 * */
static void
twr_tx_tag_cb(const dwt_cb_data_t *txd)
{
  (void) txd;
  tag_info_t *pTwrInfo = getTagInfoPtr();

  if (!pTwrInfo) {
    return;
  }

  uint32_t tmp = rtc_counter_get();   // MCU RTC time : this will be HW
                                      //   timestamped

  // Store the Tx Time Stamp of the transmitted packet
  switch (pTwrInfo->txState)
  {
    case Twr_Tx_Blink_Sent:                     // tag
      pTwrInfo->blinkRtcTimeStamp = tmp;
      dwt_readtxtimestamp(pTwrInfo->blinkTx_ts);
      break;
    case Twr_Tx_Poll_Sent:                      // tag
      pTwrInfo->pollRtcTimeStamp = tmp;
      dwt_readtxtimestamp(pTwrInfo->pollTx_ts);
      break;
    case Twr_Tx_Final_Sent:                     // tag
      pTwrInfo->finalRtcTimeStamp = tmp;
      dwt_readtxtimestamp(pTwrInfo->finalTx_ts);
      trim_tag_proc(pTwrInfo);                  // Trim Tag's offset
                                                //   automatically WRT to the
                                                //   PDOA-Node
#if (DW_TAG_NOT_SLEEPING == 0)
      app.DwCanSleepInIRQ = DW_CAN_SLEEP;
#endif
      break;
    default:
      break;
  }
}

/* @brief     ISR layer
 *             TWR application Rx callback
 *             to be called from dwt_isr() as an Rx call-back
 * */
static void
twr_rx_cb(const dwt_cb_data_t *rxd)
{
  (void) rxd;
  tag_info_t     *pTwrInfo = getTagInfoPtr();

  if (!pTwrInfo) {
    return;
  }

  uint16_t head = pTwrInfo->rxPcktBuf.head;
  uint16_t tail = pTwrInfo->rxPcktBuf.tail;
  uint16_t size = sizeof(pTwrInfo->rxPcktBuf.buf)
                  / sizeof(pTwrInfo->rxPcktBuf.buf[0]);

  if (CIRC_SPACE(head, tail, size) <= 0) {
    return;
  }

  rx_pckt_t_t *p = &pTwrInfo->rxPcktBuf.buf[head];

  p->rtcTimeStamp = rtc_counter_get();                        // MCU RTC
                                                              //   timestamp

  {
    dwt_readrxtimestamp(p->timeStamp);      // Rx TimeStamp from Ipatov CIR when
                                            //   STS OFF or from STS when STS ON
    // FIXME: should check if DW3000 STS quality good or not?
  }

  p->rxDataLen = MIN(rxd->datalength, sizeof(twr_msg_t));

  dwt_readrxdata((uint8_t *)&p->msg.stdMsg, p->rxDataLen, 0);   // Raw message

#if (DIAG_READ_SUPPORT == 1)
  read_full_diagnostics(p, rxd->status);
#endif

  if (app.rxTask.Handle != NULL) { // RTOS : rxTask can be not started
    head = (head + 1) & (size - 1);
    pTwrInfo->rxPcktBuf.head = head;        // IRQ : do not need to protect

    if (osThreadFlagsSet(app.rxTask.Handle, app.rxTask.Signal) & osFlagsError) {
      error_handler(1, _ERR_Signal_Bad);
    }
  }
}

/*
 * @brief    ISR layer
 *
 * */
static void
twr_rx_timeout_cb(const dwt_cb_data_t *rxd)
{
  (void) rxd;
  tag_info_t     *pTwrInfo = getTagInfoPtr();

#if (DW_TAG_NOT_SLEEPING == 0)
  app.DwCanSleepInIRQ = DW_CAN_SLEEP;
#endif
  if (!pTwrInfo) {
    return;
  }

  if (pTwrInfo->txState == Twr_Tx_Poll_Sent) {
    pTwrInfo->faultyRangesCnt++;
  }
}

static void
twr_rx_error_cb(const dwt_cb_data_t *rxd)
{
  twr_rx_timeout_cb(rxd);
}

/*
 * Called on SPI_RDY IRQ by deca_driver
 */
static void
tag_spi_rdy_cb(const dwt_cb_data_t *rxd)
{
  (void) rxd;
#if (DW_TAG_NOT_SLEEPING == 0)
  tag_info_t *pTwrInfo = getTagInfoPtr();

  if (pTwrInfo) {
    app.DwSpiReady = DW_SPI_READY;

    if ((app.blinkTask.Handle) && (pTwrInfo->mode == BLINKING_MODE)) {
      // signal that device is awake and poll can be sent
      if (osThreadFlagsSet(app.blinkTask.Handle,
                           app.blinkTask.Signal) & osFlagsError) {
        error_handler(1, _ERR_Signal_Bad);
      }
    } else {
      if ((app.pollTask.Handle) && (pTwrInfo->mode == RANGING_MODE)) {
        // signal that device is awake and poll can be sent
        if (osThreadFlagsSet(app.pollTask.Handle,
                             app.pollTask.Signal) & osFlagsError) {
          error_handler(1, _ERR_Signal_Bad);
        }
      }
    }
  }
#endif
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

/* @brief     app layer
 *     RTOS independent application layer function.
 *     initialising of TWR from scratch.
 *     This MUST be executed in protected mode.
 *
 *     !!!! It is assumed DW IC is reset prior to calling this function !!!!!
 *
 *     This will setup the process of:
 *     1. broadcast blink / wait for Ranging Config response;
 *     2. receive setup parameters from Ranging Config;
 *     3. if version of Ranging Config is not compatible, keep blinking;
 *     4. otherwise setup slot, new panID, framefiltering, address, TWR timings;
 *     6. switch off blinking timer and switch on precise WUP timer;
 *     5. range to the Node addr from MAC of Ranging Config
 * */
error_e tag_process_init(void)
{
#ifdef TAG_STATIC_TWRINFO
  psTagInfo = &TwrInfo;
#else

  psTagInfo = TAG_MALLOC(sizeof(tag_info_t));
#endif

  tag_info_t  *pTagInfo = getTagInfoPtr();

  if (!pTagInfo) {
    return(_ERR_Cannot_Alloc_Memory);
  }

  /* switch off receiver's rxTimeOut, RxAfterTxDelay, delayedRxTime,
   * autoRxEnable, dblBufferMode and autoACK,
   * clear all initial counters, etc.
   * */
  memset(pTagInfo, 0, sizeof(tag_info_t));

  {  // For Trilat demo purposes Fixed tags can supply their locations in the
     //   payload
    pTagInfo->pos.pos_x[0] = app.pConfig->s.fixed_pos_x_mm & 0xFF;
    pTagInfo->pos.pos_x[1] = app.pConfig->s.fixed_pos_x_mm >> 8;

    pTagInfo->pos.pos_y[0] = app.pConfig->s.fixed_pos_y_mm & 0xFF;
    pTagInfo->pos.pos_y[1] = app.pConfig->s.fixed_pos_y_mm >> 8;

    pTagInfo->pos.pos_z[0] = app.pConfig->s.fixed_pos_z_mm & 0xFF;
    pTagInfo->pos.pos_z[1] = app.pConfig->s.fixed_pos_z_mm >> 8;
  }

  /* Tag will receive its configuration, such as
   * panID, tagAddr, node0Addr and TWR delays:
   * pollTx2FinalTxDelay_us and response rx delay from Ranging Config message.
   *
   * But the reception timeouts calculated based on known length of
   * Ranging Config and Response packets.
   * */

  {   // pre-calculate all possible two-way ranging frame timings
    dwt_config_t *pCfg = &app.pConfig->dwt_config;        // dwt_config : holds
                                                          //   node's UWB mode

    msg_t msg;

    msg.dataRate = pCfg->dataRate;                  // Deca define: e.g.
                                                    //   DWT_BR_6M8
    msg.txPreambLength = pCfg->txPreambLength;      // Deca define: e.g.
                                                    //   DWT_PLEN_128
    msg.sfdType = pCfg->sfdType;
    msg.txPcode = pCfg->txCode;
    msg.stsLength =
      (pCfg->stsMode
       & DWT_STS_CONFIG_MASK) ? ((1 << (pCfg->stsLength + 2)) * 8) : 0;

    msg.msg_len = sizeof(rng_cfg_msg_t);
    calculate_msg_time(&msg, &pTagInfo->msg_time.ranging_config);

    msg.msg_len = sizeof(poll_msg_t);
    calculate_msg_time(&msg, &pTagInfo->msg_time.poll);

    msg.msg_len = sizeof(resp_pdoa_msg_t);
    calculate_msg_time(&msg, &pTagInfo->msg_time.response);

    msg.msg_len = sizeof(final_msg_accel_t);
    calculate_msg_time(&msg, &pTagInfo->msg_time.final);
  }

  /* dwt_xx calls in app level Must be in protected mode (DW3000 IRQ disabled)
   */
  disable_dw3000_irq();

  TWR_ENTER_CRITICAL();

  if (dwt_initialise(DWT_DW_INIT | DWT_READ_OTP_PID | DWT_READ_OTP_LID)
      != DWT_SUCCESS) {  /**< set callbacks to NULL inside dwt_initialise */
    TWR_EXIT_CRITICAL();
    diag_printf("Tag init failed. Stop.\r\n");
    return (_ERR_INIT);       // device initialise has failed
  }

  set_dw_spi_fast_rate();

  uint32_t dev_id = dwt_readdevid();

  if ((dev_id == (uint32_t)DWT_DW3000_PDOA_DEV_ID)
      || (dev_id == (uint32_t)DWT_QM33120_PDOA_DEV_ID)) {
    pTagInfo->dw3000ChipType = AOA;
  } else if (dev_id == (uint32_t)DWT_DW3000_DEV_ID) {
    pTagInfo->dw3000ChipType = NON_AOA;

    app.pConfig->dwt_config.pdoaMode = DWT_PDOA_M0;
  } else {
    TWR_EXIT_CRITICAL();
    diag_printf("Found unknown chip 0x%08lX. Stop.\r\n",
                (unsigned long int)dev_id);
    return _ERR_DEVID;
  }

  /* Configure receiver's UWB mode, set power and antenna delays for TWR mode */
  rxtx_configure_t p;
  p.pdwCfg = &app.pConfig->dwt_config;
  p.frameFilter = DWT_FF_DISABLE;      // DWT_FF_ENABLE_802_15_4
  p.frameFilterMode = (DWT_FF_DATA_EN | DWT_FF_ACK_EN);     // FIXME
  p.txAntDelay = app.pConfig->s.antTx_a;
  p.rxAntDelay = app.pConfig->s.antRx_a;
  p.panId = 0x5555;        // PanID : does not matter : DWT_FF_NOTYPE_EN : will
                           //   be reconfigured on reception of RI message
  p.shortadd = 0xAAAA;     // ShortAddr : does not matter : DWT_FF_NOTYPE_EN :
                           //   will be reconfigured on reception of RI message

  rxtx_configure(&p);

  dwt_setleds(DWT_LEDS_ENABLE
              | DWT_LEDS_INIT_BLINK); /**< DEBUG I/O 2&3 : configure
                                       *   the GPIOs which control
                                       *   the LEDs on HW
                                       */
  dwt_setlnapamode(DWT_TXRX_EN); /**< DEBUG I/O 4&5&6 :
                                  * configure LNA/PA,
                                  * 0&1 TX/RX states to output on GPIOs
                                  */

  dwt_setcallbacks(twr_tx_tag_cb,
                   twr_rx_cb,
                   twr_rx_timeout_cb,
                   twr_rx_error_cb,
                   NULL,
                   tag_spi_rdy_cb,
                   NULL);

  dwt_setinterrupt(DWT_INT_SPIRDY_BIT_MASK | DWT_INT_TXFRS_BIT_MASK
                   | DWT_INT_RXFCG_BIT_MASK
                   | (DWT_INT_ARFE_BIT_MASK | DWT_INT_RXFSL_BIT_MASK
                      | DWT_INT_RXSTO_BIT_MASK | DWT_INT_RXPHE_BIT_MASK
                      | DWT_INT_RXFCE_BIT_MASK | DWT_INT_RXFTO_BIT_MASK

                      /*| DWT_INT_RXPTO_BIT_MASK*/),
                   0,
                   DWT_ENABLE_INT_ONLY);

  dwt_configuresleep(DWT_CONFIG | DWT_PGFCAL,
                     DWT_PRES_SLEEP | DWT_WAKE_CSN | DWT_SLP_EN);

  init_dw3000_irq();              /**< manually init EXTI DW3000 lines IRQs */

  /* configure non-zero initial values */

  /* TODO: fix "4".
   * to be able receive long rng_cfg_upd_msg_t relax Response RX timeout time
   */
  pTagInfo->env.responseRxTo_sy = 4 * MAX(pTagInfo->msg_time.response.sy,
                                          pTagInfo->msg_time.ranging_config.sy);

  pTagInfo->seqNum = (uint8_t)(0xff * rand() / RAND_MAX);

  /* below valid after dwt_initialise() : OTP values */
  {
    uint32_t lotId = dwt_getlotid();
    uint32_t partId = dwt_getpartid();

    if ((lotId | partId) == 0) {
      if ((app.pConfig->s.partId | app.pConfig->s.lotId) == 0) {
        app.pConfig->s.lotId = rand();
        app.pConfig->s.partId = rand();
        save_bssConfig(app.pConfig);
      }

      lotId = app.pConfig->s.lotId;
      partId = app.pConfig->s.partId;
    }
    pTagInfo->eui64 = ((uint64_t)lotId << 32) | partId;
  }

  /*
   * The dwt_initialize will read the default XTAL TRIM from the OTP or use the
   *   DEFAULT_XTAL_TRIM.
   * In this case we would apply the user-configured value.
   *
   * Bit 0x80 can be used to overwrite the OTP settings if any.
   * */
  if ((dwt_getxtaltrim() == DEFAULT_XTAL_TRIM)
      || (app.pConfig->s.xtalTrim & ~XTAL_TRIM_BIT_MASK)) {
    dwt_setxtaltrim(app.pConfig->s.xtalTrim & XTAL_TRIM_BIT_MASK);
    pTagInfo->xtaltrim = dwt_getxtaltrim();
  }

  rtc_disable_irq();

  pTagInfo->mode = BLINKING_MODE;

  juniper_configure_hal_rtc_callback(rtcWakeUpTimerEventCallback_tag);

  tag_configure_rtc_wakeup_ns(10 * 1e6);      // first blink will start in 10ms
                                              //   after starting of the Tag
                                              //   application

  app.DwCanSleepInIRQ = DW_CANNOT_SLEEP;
#if (DW_TAG_NOT_SLEEPING == 1)
  app.DwEnterSleep = DW_NOT_SLEEPING;       //
#else
  app.DwEnterSleep = DW_IS_SLEEPING_FIRST;
  dwt_entersleep(DWT_DW_IDLE_RC);
  app.DwSpiReady = DW_SPI_SLEEPING;
#endif

  TWR_EXIT_CRITICAL();

  return (_NO_ERR);
}

/*
 * */
void tag_process_start(void)
{
  enable_dw3000_irq();           /**< enable DW3000 IRQ to start  */

  // start the RTC timer
  rtc_enable_irq();

  diag_printf("Tag Top Application: Started\r\n");
}

/* @brief   app level
 *          RTOS-independent application level function.
 *          deinitialize the pTwrInfo structure.
 *  This must be executed in protected mode.
 *
 * */
void tag_process_terminate(void)
{
  TWR_ENTER_CRITICAL();

  // stop the RTC timer
  rtc_disable_irq();
  juniper_configure_hal_rtc_callback(NULL);

#ifndef TAG_STATIC_TWRINFO
  if (psTagInfo) {
    TAG_FREE(psTagInfo);
    psTagInfo = NULL;
  }
#endif

  TWR_EXIT_CRITICAL();
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

/* @brief
 *          TWR : DISCOVERY PHASE
 *          Tag sends Blinks, waiting for a Ranging Config message
 *
 *          application layer function
 */
error_e    initiator_send_blink(tag_info_t *p)
{
  error_e       ret;
  tx_pckt_t     txPckt;

  memset(&txPckt, 0, sizeof(txPckt));

  blink_msg_t *pTxMsg = &txPckt.msg.blinkMsg;

  // TWR : PHASE : Initiator Sends Blink to Responder
  txPckt.psduLen = sizeof(blink_msg_t);
  txPckt.delayedTxTimeH_sy = 0;
  txPckt.txFlag = (DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

  // Ranging Config: activate receiver this time SY after Blink Tx
  txPckt.delayedRxTime_sy =
    (uint32_t)util_us_to_sy(app.pConfig->s.rcDelay_us);

  // Ranging Config: receiver will be active for this time, SY
  txPckt.delayedRxTimeout_sy =
    (uint32_t)util_us_to_sy(app.pConfig->s.rcRxTo_us);

  pTxMsg->frameCtrl[0] = Head_Msg_BLINK;
  pTxMsg->seqNum = p->seqNum;
  memcpy(&pTxMsg->tagID, &p->eui64, sizeof(pTxMsg->tagID));

  p->seqNum++;
  p->txState = Twr_Tx_Blink_Sent;

  TWR_ENTER_CRITICAL();

  ret = tx_start(&txPckt);

  TWR_EXIT_CRITICAL();

  if (ret != _NO_ERR) {
    p->lateTX++;
  }

  return (ret);
}

/* @brief
 *          TWR: RANGING PHASE
 *          Initiator sends Poll to the Responder
 *
 *          application layer function
 */
error_e    initiator_send_poll(tag_info_t *p)
{
  error_e      ret;
  tx_pckt_t    txPckt;

  poll_msg_t   *pTxMsg = &txPckt.msg.pollMsg;

  /* Construct TxPckt packet: Send Poll immediate and configure delayed wait for
   *   the Response */
  txPckt.psduLen = sizeof(poll_msg_t);
  txPckt.delayedTxTimeH_sy = 0;
  txPckt.txFlag = (DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

  txPckt.delayedRxTime_sy = p->env.delayRx_sy;              // environment,
                                                            //   received from
                                                            //   Ranging Config
                                                            //   message
  txPckt.delayedRxTimeout_sy = p->env.responseRxTo_sy;      // this is
                                                            //   calculated
                                                            //   locally based
                                                            //   on UWB
                                                            //   configuration

  /* Construct TX Final UWB message */

  /* See IEEE frame header description */
  pTxMsg->mac.frameCtrl[0] = Head_Msg_STD;
  pTxMsg->mac.frameCtrl[1] = Frame_Ctrl_SS;
  pTxMsg->mac.panID[0] = p->env.panID & 0xff;
  pTxMsg->mac.panID[1] = (p->env.panID >> 8) & 0xff;
  pTxMsg->mac.destAddr[0] = p->env.nodeAddr[0];
  pTxMsg->mac.destAddr[1] = p->env.nodeAddr[1];
  pTxMsg->mac.sourceAddr[0] = p->env.tagAddr[0];
  pTxMsg->mac.sourceAddr[1] = p->env.tagAddr[1];
  pTxMsg->mac.seqNum = p->seqNum;

  /* Data */
  pTxMsg->poll.fCode = Twr_Fcode_Tag_Poll;
  pTxMsg->poll.rNum = p->rangeNum;

  /* Transmit over the air */
  p->txState = Twr_Tx_Poll_Sent;
  p->seqNum++;
  p->rangeNum++;

  POLL_ENTER_CRITICAL();

  ret = tx_start(&txPckt);

  POLL_EXIT_CRITICAL();

  if (ret != _NO_ERR) {
    p->lateTX++;
  }

  return (ret);
}

/* @brief     Part of twr_initiator_algorithm_tx.
 *            this function is Interrupt level, should be executed with
 * Initiator wakes DW3000 chip to send Blink or Poll message.
 *
 * */
error_e tag_wakeup_dw3000_blink_poll(tag_info_t *p)
{
  error_e  ret;

  p->stationary = p->stationary_imu;

  app.DwCanSleepInIRQ = DW_CANNOT_SLEEP;
#if (DW_TAG_NOT_SLEEPING == 0)
  if (app.DwEnterSleep != DW_NOT_SLEEPING) {
    ret = port_wakeup_dw3000_fast();     // should never return anything other
                                         //   than _NO_ERR
  } else {
    diag_printf("Tag NOT sleeping, skip wakeup\r\n");
    ret = _NO_ERR;
  }
#else

  dwt_forcetrxoff();    // Tag may be in the RX state

  ret = _NO_ERR;
#endif
  return ret;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// The most real-time section of TWR algorithm

/* @brief    app layer
 *             Real-time TWR algorithm implementation (Initiator)
 *
 *             prefer to be called from application UWB Rx,
 *            but can be called from ISR, i.e. twr_rx_cb() directly.
 *             if called from ISR layer, then revise/remove
 *             TWR_ENTER_CRITICAL()
 *             TWR_EXIT_CRITICAL()
 *
 * @return     error_e
 * */
error_e twr_initiator_algorithm_rx(rx_pckt_t_t *pRxPckt, tag_info_t *pTwrInfo)
{
  error_e     ret = _ERR_Not_Twr_Frame;
  fcode_e     fcode = Twr_Fcode_Not_Defined;

  std_msg_t   *pMsg = &pRxPckt->msg.stdMsg;

  if ((pMsg->mac.frameCtrl[0] == Head_Msg_STD)
      || (pMsg->mac.frameCtrl[0] == Head_Msg_STD_AR)) {
    /* Only SS and LS MAC headers supported in current application */
    switch (pMsg->mac.frameCtrl[1] & Frame_Ctrl_MASK)
    {
      case Frame_Ctrl_SS:
        if ((pRxPckt->rxDataLen == sizeof(resp_pdoa_msg_t)) /* Response (with
                                                             *   Coordinates
                                                             *   extension)
                                                             */
            || (pRxPckt->rxDataLen == sizeof(rng_cfg_upd_msg_t))) { /* Ranging
                                                                     * Config
                                                                     * (update)
                                                                     */
          fcode = ((std_msg_ss_t *)pMsg)->messageData[0];
        }
        break;
      case Frame_Ctrl_LS:
        if (pRxPckt->rxDataLen == sizeof(rng_cfg_msg_t)) {       /* Ranging
                                                                  * Config
                                                                  * (discovery)
                                                                  */
          fcode = ((std_msg_ls_t *)pMsg)->messageData[0];
        }
        break;
      default:
        fcode = Twr_Fcode_Not_Defined;
        break;
    }
  }

  switch (fcode)
  {
    case Twr_Fcode_Rng_Config:
      /* Initiator received Range Init message from a Responder in discovery
       *   process.
       * 1. setup Timing parameters in accordance to Range Init message;
       * 2. configure MCU RTC WKUP timer to expire on next slot time
       * */
      ret = initiator_received_ranging_config(pRxPckt, pTwrInfo);

      if (ret != _NO_Err_Ranging_Update) {
        ret = _NO_Err_Can_Sleep;         // all good and wrong RX except Update
                                         //   would lead to dwt_entersleep()
      }
      break;

    case Twr_Fcode_Resp_Ext:
      /* Initiator received a Response message.
       * If the message from our PDOA node:
       * 1. Send delayed Final_TX;
       * 2. Adjust the Wakeup timer for next Poll.
       * */
      if ((pTwrInfo->env.nodeAddr[0]
           == ((resp_pdoa_msg_t *)pMsg)->mac.sourceAddr[0])
          || (pTwrInfo->env.nodeAddr[1]
              == ((resp_pdoa_msg_t *)pMsg)->mac.sourceAddr[1])) {
        // diag_printf("Twr_Fcode_Resp_Ext\r\n");
        // (1)
        ret = initiator_received_response(pRxPckt, pTwrInfo);

        if (ret == _NO_ERR) {
          ret = _NO_Err_Response;
        }

        // (2)
        TWR_ENTER_CRITICAL();

        int32_t         slotCorr_ns;
        uint32_t        nextWakeUpPeriod_ns, rtcNow, tmpNow;

        // casting received bytes to the int,
        // this is a signed correction wrt (gRtcSFrameZeroCnt + expected_slot)
        //   coming from the Node
        pTwrInfo->env.slotCorr_ns = 1000
                                    * (int32_t)AR2U32(
          ((resp_pdoa_msg_t *)pMsg)->resp.slotCorr_us);

        slotCorr_ns = pTwrInfo->env.slotCorr_ns;

        // (A)start of calculation
        rtcNow = rtc_counter_get();                             // MCU RTC time
                                                                //   : this will
                                                                //   be HW
                                                                //   timestamped

        tmpNow = (rtcNow > pTwrInfo->gRtcSFrameZeroCnt)
                 ?(rtcNow - pTwrInfo->gRtcSFrameZeroCnt)
                 :(rtcNow - pTwrInfo->gRtcSFrameZeroCnt + RTC_WKUP_CNT_OVFLW);

        // Next RTC_Wakeup of Tag will be aligned to exact slot expected by the
        //   Node's:
        nextWakeUpPeriod_ns = pTwrInfo->env.sframePeriod_ns
                              - (WKUP_RESOLUTION_NS * tmpNow)
                              - (slotCorr_ns);

        if (nextWakeUpPeriod_ns < (pTwrInfo->env.sframePeriod_ns >> 1)) {
          nextWakeUpPeriod_ns += (pTwrInfo->env.sframePeriod_ns);
        }
        tag_configure_rtc_wakeup_ns(nextWakeUpPeriod_ns);

        TWR_EXIT_CRITICAL();
      }
      break;

    default:
      /* Initiator received unknown data / reset initiator
       * */
      ret = _ERR_Not_Twr_Frame;
      break;
  }

  return (ret);
}

/* @brief check on-the air configuration that it capable
 *           to work on the current hardware
 * */
error_e check_air_config_correct(uint8_t ver,
                                 uint16_t delayRx_sy,
                                 uint16_t pollTxToFinalTx_us)
{
  error_e ret = _NO_ERR;

  if (ver != RC_VERSION_PDOA) {// This Tag knows only one version of Ranging
                               //   Config format: RC_VERSION_PDOA
    ret = _ERR_RC_Version_Unknown;
  } else if ((delayRx_sy < MIN_RESPONSE_CAPABILITY_US) \
             || (pollTxToFinalTx_us < MIN_POLL_TX_FINAL_TX_CAPABILITY_US)) {
    /* This Tag hardware is too slow
     * and cannot range with parameters, supplied by the Node.
     */
    ret = _ERR_Non_Compatible_TWR_Parameters;
  }

  return ret;
}

/* @brief     Part of twr_initiator_algorithm_rx
 *
 * Initiator in discovery phase received RANGING CONFIG message from a
 *   Responder.
 * 1. Check that Tag understand and can comply to requested Ranging Config
 *   parameters;
 * 2. Setup Range Times as defined in the Ranging Config message;
 * 3. Switch off the Blink Timer.
 * 4. Configure & Start the Poll Timer.
 *
 * @parm    *rx_packet, *control structure
 *
 * @return  error codes:
 *          _ERR_Range_Config
 *          _ERR_RC_Version_Unknown
 *          _ERR_Non_Compatible_TWR_Parameters
 *          _NO_Err_Ranging_Config
 *          _NO_Err_Ranging_Update
 * */
error_e initiator_received_ranging_config(rx_pckt_t_t *pRxPckt,
                                          tag_info_t *pTwrInfo)
{
  error_e ret = _ERR_Ranging_Config;

  rng_cfg_t *pRxMsg;

  uint8_t units_ms = 0;

  if (pRxPckt->rxDataLen == sizeof(rng_cfg_msg_t)) {
    /* obtain Node address and PanID from the MAC of Ranging Config */
    pTwrInfo->env.panID = AR2U16(pRxPckt->msg.rngCfgMsg.mac.panID);
    pTwrInfo->env.nodeAddr[0] = pRxPckt->msg.rngCfgMsg.mac.sourceAddr[0];
    pTwrInfo->env.nodeAddr[1] = pRxPckt->msg.rngCfgMsg.mac.sourceAddr[1];

    pRxMsg = &pRxPckt->msg.rngCfgMsg.rngCfg;
  } else if (pRxPckt->rxDataLen == sizeof(rng_cfg_upd_msg_t)) {
    /* update */
    pRxMsg = &pRxPckt->msg.rngCfgUpdMsg.rngCfg;
  } else {
    return (ret);
  }

  /*check units*/
  if ((pRxMsg->version == RC_VERSION_DR)
      && (pRxPckt->msg.rngCfgMsg.rngCfg.ANC_RESP_DLY[1] & 0x80)) {
    // units are ms
    units_ms = 1;
    ret = _NO_ERR;
  } else {
    ret = check_air_config_correct(pRxMsg->version,
                                   AR2U16(pRxMsg->delayRx_us),
                                   AR2U16(pRxMsg->pollTxToFinalTx_us));
  }

  if (ret == _NO_ERR) {
    ret =
      (pRxPckt->rxDataLen
       == sizeof(rng_cfg_msg_t))?(_NO_Err_Ranging_Config):(
        _NO_Err_Ranging_Update);

    TWR_ENTER_CRITICAL();

    rtc_disable_irq();

    /* configure environment parameters from Ranging Config message */
    pTwrInfo->env.version = pRxMsg->version;

    pTwrInfo->env.tagAddr[0] = pRxMsg->tagAddr[0];
    pTwrInfo->env.tagAddr[1] = pRxMsg->tagAddr[1];

    pTwrInfo->env.sframePeriod_ns =
      1000000 * (uint32_t)(AR2U16(pRxMsg->sframePeriod_ms));

    if (units_ms == 1) {
      uint32_t ancResp =
        (AR2U16(pRxPckt->msg.rngCfgMsg.rngCfg.ANC_RESP_DLY) & 0x7FFF);
      uint32_t tagResp =
        (AR2U16(pRxPckt->msg.rngCfgMsg.rngCfg.TAG_RESP_DLY) & 0x7FFF);

      pTwrInfo->env.pollTx2FinalTxDelay64 =
        util_us_to_dev_time((ancResp + tagResp) * 1000); /* Time from
                                                          * RMARKER of Poll to
                                                          * RMARKER of Final.
                                                          */
      // pTwrInfo->env.delayRx_sy            = (uint32_t)(
      //   (((uint32_t)AR2U16(pRxMsg->delayRx_us))<<16) +
      //   AR2U16(pRxMsg->pollTxToFinalTx_us) );      //RX turn on delay
      //   following Poll transmission
      pTwrInfo->env.delayRx_sy =
        (uint32_t)util_us_to_sy(((AR2U16(pRxPckt->msg.rngCfgMsg.rngCfg.
                                         ANC_RESP_DLY) & 0x7FFF)) * 1000
                                - pTwrInfo->msg_time.poll.us); /* RX turn on
                                                                * delay
                                                                * following
                                                                * Poll
                                                                * transmission
                                                                */
    } else {
      pTwrInfo->env.pollTx2FinalTxDelay64 =
        util_us_to_dev_time(AR2U16(pRxMsg->pollTxToFinalTx_us)); /* Time
                                                                  * from RMARKER
                                                                  * of Poll to
                                                                  * RMARKER
                                                                  * of Final.
                                                                  */
      pTwrInfo->env.delayRx_sy =
        (uint32_t)util_us_to_sy(AR2U16(pRxMsg->delayRx_us)); /* Time after
                                                              * Poll Tx
                                                              * completed
                                                              * to activate
                                                              * the RX
                                                              */
    }

    pTwrInfo->env.pollMultFast = AR2U16(pRxMsg->pollMultFast);
    pTwrInfo->env.pollMultSlow = AR2U16(pRxMsg->pollMultSlow);
    pTwrInfo->env.mode = AR2U16(pRxMsg->mode);
    pTwrInfo->env.slotCorr_ns =
      1000 * (int32_t)AR2U32(pRxMsg->slotCorr_us); /* next wakeup correction
                                                    * to fit the slot
                                                    */

    rxtx_configure_t p;
    p.pdwCfg = &app.pConfig->dwt_config;
    p.frameFilter = DWT_FF_DISABLE;    ////DWT_FF_ENABLE_802_15_4
    p.frameFilterMode = (DWT_FF_DATA_EN | DWT_FF_ACK_EN);     // FIXME
    p.txAntDelay = app.pConfig->s.antTx_a;
    p.rxAntDelay = app.pConfig->s.antRx_a;
    p.panId = pTwrInfo->env.panID;
    p.shortadd = AR2U16(pTwrInfo->env.tagAddr);

    /* Setup configured panID, FrameFiltering and antenna delays */
    tn_app_config(&p);

    /* Setup New High resolution RTC Wakup Timer : */
    uint32_t    tmp, rtcNow, tmpNow;

    rtcNow = rtc_counter_get();      // MCU RTC time : this will be HW
                                     //   timestamped

    pTwrInfo->mode = RANGING_MODE;

    // STM32: RTC is counting down
    tmpNow = (rtcNow > pTwrInfo->gRtcSFrameZeroCnt)
             ?(rtcNow - pTwrInfo->gRtcSFrameZeroCnt)
             :(rtcNow - pTwrInfo->gRtcSFrameZeroCnt + RTC_WKUP_CNT_OVFLW);

    // Next RTC_Wakeup of Tag will be aligned to exact slot expected by the
    //   Node's:
    tmp = pTwrInfo->env.sframePeriod_ns
          - (WKUP_RESOLUTION_NS * tmpNow)
          - pTwrInfo->env.slotCorr_ns;

    if (tmp < (pTwrInfo->env.sframePeriod_ns >> 1)) {
      tmp += (pTwrInfo->env.sframePeriod_ns);
    }

    tag_configure_rtc_wakeup_ns(tmp);      // update the RTC Wakup timer to
                                           //   start Ranging in assigned slot

    rtc_enable_irq();

    TWR_EXIT_CRITICAL();
  } else {
  }

  return (ret);
}

/* @brief     Part of twr_initiator_algorithm_rx.
 *
 * Initiator received the RESPONSE message.
 * 1. setup and send delayed Final_TX according to control structure
 * 2. if a use case : setup the delayed receive for reception of REPORT TOF
 *
 * @parm    *rx_packet, *control structure
 *
 * @return delayed TxPool error code
 * */
error_e initiator_received_response(rx_pckt_t_t *prxPckt, tag_info_t *p)
{
  error_e     ret;
  uint64_t    calcFinalTx64;

  tx_pckt_t   TxPckt;                   /**< allocate space for Tx Packet */

  final_msg_accel_t      *pTxMsg = &TxPckt.msg.finalMsg;

  /* Construct TxPckt packet: no reception after Final transmission */
  TxPckt.psduLen = sizeof(final_msg_accel_t);
  TxPckt.txFlag = (DWT_START_TX_DELAYED);
  TxPckt.delayedRxTime_sy = 0;
  TxPckt.delayedRxTimeout_sy = 0;

  /* Calculate timings for transmission of Final Packet */
  // Load Poll Tx time
  TS2U64_MEMCPY(calcFinalTx64, p->pollTx_ts);

  // Add delay from Ranging Config message: PollTx2FinalTx
  calcFinalTx64 = MASK_TXDTS
                  & ((uint64_t)(calcFinalTx64 + p->env.pollTx2FinalTxDelay64));

  // DW1000 will adjust the transmission of Final TxPacket SFD exactly at
  //   PollTx+PollTx2FinalTx+AntTxDelay
  TxPckt.delayedTxTimeH_sy = calcFinalTx64 >> 8;

  // Calculate Time Final message will be sent and embed this number to the
  //   Final message data.
  // Sending time will be delayedReplyTime, snapped to ~125MHz or ~250MHz
  //   boundary by
  // zeroing its low 9 bits, and then having the TX antenna delay added.
  // Getting antenna delay from the config and add it to the Calculated TX Time
  calcFinalTx64 = MASK_40BIT
                  & (uint64_t)(calcFinalTx64 + app.pConfig->s.antTx_a);

  /* Construct TX Final UWB message */

  /* See IEEE frame header description */
  pTxMsg->mac.frameCtrl[0] = Head_Msg_STD;
  pTxMsg->mac.frameCtrl[1] = Frame_Ctrl_SS;
  pTxMsg->mac.panID[0] = p->env.panID & 0xff;
  pTxMsg->mac.panID[1] = (p->env.panID >> 8) & 0xff;
  pTxMsg->mac.destAddr[0] = p->env.nodeAddr[0];
  pTxMsg->mac.destAddr[1] = p->env.nodeAddr[1];
  pTxMsg->mac.sourceAddr[0] = p->env.tagAddr[0];
  pTxMsg->mac.sourceAddr[1] = p->env.tagAddr[1];
  pTxMsg->mac.seqNum = p->seqNum;

  /* Data */
  pTxMsg->final.fCode = (uint8_t)Twr_Fcode_Tag_Accel_Final;
  pTxMsg->final.rNum = (uint8_t)p->rangeNum;
  TS2TS_UWB_MEMCPY(pTxMsg->final.pollTx_ts,
                   p->pollTx_ts); // Embed Poll Tx time to the Final message
  TS2TS_UWB_MEMCPY(pTxMsg->final.responseRx_ts,
                   prxPckt->timeStamp); /* Embed Response Rx time
                                         * to the Final message
                                         */
  U642TS_UWB_MEMCPY(pTxMsg->final.finalTx_ts,
                    calcFinalTx64);     /* Embed Calculated Final TX time
                                         * to the Final message
                                         */
  pTxMsg->final.flag = p->stationary;
  pTxMsg->final.acc_x[0] = p->acc.acc_x[0];
  pTxMsg->final.acc_x[1] = p->acc.acc_x[1];
  pTxMsg->final.acc_y[0] = p->acc.acc_y[0];
  pTxMsg->final.acc_y[1] = p->acc.acc_y[1];
  pTxMsg->final.acc_z[0] = p->acc.acc_z[0];
  pTxMsg->final.acc_z[1] = p->acc.acc_z[1];

  /* Transmit over the air */
  p->txState = Twr_Tx_Final_Sent;       // indicate to TX ISR that the response
                                        //   has been sent
  p->seqNum++;

  TWR_ENTER_CRITICAL();

  ret = tx_start(&TxPckt);

  TWR_EXIT_CRITICAL();

  if (ret != _NO_ERR) {
    p->lateTX++;
  }

  return (ret);
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

/*
 * @brief   setup RTC Wakeup timer
 *          period_ns is awaiting time in ns
 * */
void tag_configure_rtc_wakeup_ns(uint32_t     period_ns)
{
  rtc_configure_wakeup_ns(period_ns);
}

/* RTC Wakeup timer callback.
 *         Use the LED_TAG_Pin to see synchronization of tags
 *         on the oscilloscope if needed
 * */
void rtcWakeUpTimerEventCallback_tag(void)
{
  static int  count = 0;

  tag_info_t  *pTwrInfo = getTagInfoPtr();

  if (pTwrInfo) {   // RTOS : pTwrInfo can be not allocated yet
    pTwrInfo->gRtcSFrameZeroCnt = rtc_counter_get();

    if (pTwrInfo->mode == BLINKING_MODE) {// BLINKING phase
      if (app.blinkTask.Handle) {     // RTOS : blinkTask can be not started yet
        if (osThreadFlagsSet(app.blinkTask.Handle,
                             app.blinkTask.Signal) & osFlagsError) {
          error_handler(1, _ERR_Signal_Bad);
        }
      }

      uint32_t new_blink_period_ms =
        (uint32_t)((float)((BLINK_PERIOD_MS / 3.0) * rand() / RAND_MAX));
      new_blink_period_ms += BLINK_PERIOD_MS;
      rtc_configure_wakeup_ms(new_blink_period_ms);
    } else {// RANGING phase
      rtc_configure_wakeup_ns(pTwrInfo->env.sframePeriod_ns);

      if (pTwrInfo->stationary && !pTwrInfo->stationary_imu) {
        pTwrInfo->stationary = false;
        count = pTwrInfo->env.pollMultFast;
      }

      count++;

      if ((pTwrInfo->stationary && (count >= pTwrInfo->env.pollMultSlow))
          || (!pTwrInfo->stationary && (count >= pTwrInfo->env.pollMultFast))) {
        if (app.pollTask.Handle) {       // RTOS : pollTask can be not started
                                         //   yet
          if (osThreadFlagsSet(app.pollTask.Handle,
                               app.pollTask.Signal) & osFlagsError) {
            error_handler(1, _ERR_Signal_Bad);
          }
        }
        count = 0;
      }
    }
  }
}

// -----------------------------------------------------------------------------
