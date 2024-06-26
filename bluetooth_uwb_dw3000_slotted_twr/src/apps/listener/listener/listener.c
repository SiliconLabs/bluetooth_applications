/**
 * @file    listener.c
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

/* Includes */
#include "listener.h"
#include "deca_device_api.h"
#include "deca_dbg.h"

#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

// ----------------------------------------------------------------------------
// implementation-specific: critical section protection
#ifndef TWR_ENTER_CRITICAL
#define TWR_ENTER_CRITICAL    taskENTER_CRITICAL
#endif

#ifndef TWR_EXIT_CRITICAL
#define TWR_EXIT_CRITICAL     taskEXIT_CRITICAL
#endif

#define LSTNR_MALLOC          pvPortMalloc
#define LSTNR_FREE            vPortFree

// -----------------------------------------------------------------------------
// The psListenerInfo structure holds all Listener's process parameters
static listener_info_t *psListenerInfo = NULL;

/*
 * @brief     get pointer to the twrInfo structure
 * */
listener_info_t * getListenerInfoPtr(void)
{
  return (psListenerInfo);
}

/*
 * @brief     ISR level (need to be protected if called from APP level)
 *             low-level configuration for DW3000
 *
 *             if called from app, shall be performed with DW IRQ off &
 *             TWR_ENTER_CRITICAL(); / TWR_EXIT_CRITICAL();
 *
 *             The SPI for selected chip shall already be chosen
 *
 *
 *
 * @note
 * */
static void
rxtx_listener_configure
(
  dwt_config_t    *pdwCfg,
  uint16_t        frameFilter,
  uint16_t        txAntDelay,
  uint16_t        rxAntDelay
)
{
  (void) frameFilter;
  (void) txAntDelay;
  (void) rxAntDelay;

  if (dwt_configure(pdwCfg)) {   /**< Configure the Physical Channel parameters
                                  *   (PLEN, PRF, etc) */
    error_handler(1, _ERR_INIT);
  }
  dwt_setrxaftertxdelay(0);         /**< no any delays set by default : part of
                                     *   config of receiver on Tx sending */
  dwt_setrxtimeout(0);              /**< no any delays set by default : part of
                                     *   config of receiver on Tx sending */
  dwt_configureframefilter(DWT_FF_DISABLE, 0);
}

/* @brief   ISR level
 *          TWR application Rx callback
 *          to be called from dwt_isr() as an Rx call-back
 * */
void rx_listener_cb(const dwt_cb_data_t *rxd)
{
  listener_info_t  *pListenerInfo = getListenerInfoPtr();

  if (!pListenerInfo) {
    return;
  }

  int16_t stsQual;

  const int  size = sizeof(pListenerInfo->rxPcktBuf.buf)
                    / sizeof(pListenerInfo->rxPcktBuf.buf[0]);

  int head = pListenerInfo->rxPcktBuf.head;
  int tail = pListenerInfo->rxPcktBuf.tail;

  if (CIRC_SPACE(head, tail, size) > 0) {
    rx_listener_pckt_t *p = &pListenerInfo->rxPcktBuf.buf[head];

    dwt_readrxtimestamp(p->timeStamp);          // Raw Rx TimeStamp (STS or
                                                //   IPATOV based on STS config)

    p->clock_offset = dwt_readclockoffset();    // Reading Clock offset for any
                                                //   Rx packets

    p->status = rxd->status;

    if (dwt_readstsquality(&stsQual) < 0) {  // if < 0 then this is "bad" STS
      app.event_counts_sts_bad++;
    } else {
      app.event_counts_sts_good++;
    }

    app.event_counts_sfd_detect++;

    // check if this is an SP3 packet
    if (rxd->rx_flags & DWT_CB_DATA_RX_FLAG_ND) {
      p->rxDataLen = 0;
    } else {
      p->rxDataLen = MIN(rxd->datalength, sizeof(p->msg));

      dwt_readrxdata((uint8_t *)&p->msg, p->rxDataLen, 0);       // Raw message

      // p->msg.data[p->rxDataLen] = stsQual>>8;
      // p->msg.data[p->rxDataLen+1] = stsQual;
      // p->rxDataLen+=2;
    }

    if (app.listenerTask.Handle) {          // RTOS : listenerTask can be not
                                            //   started yet
      head = (head + 1) & (size - 1);
      pListenerInfo->rxPcktBuf.head = head;          // ISR level : do not need
                                                     //   to protect
    }
  }

  if (app.listenerTask.Handle) {        // RTOS : listenerTask can be not
                                        //   started yet
    // Sends the Signal to the application level via OS kernel.
    // This will add a small delay of few us, but
    // this method make sense from a program structure point of view.
    if (osThreadFlagsSet(app.listenerTask.Handle,
                         app.listenerTask.Signal) & osFlagsError) {
      error_handler(1, _ERR_Signal_Bad);
    }
  }

  if (app.pConfig->s.stsStatic) { // value 0 = dynamic STS, 1 = fixed STS)
    // re-load the initial cp_iv value to keep STS the same for each frame
    dwt_configurestsloadiv();
  }

  dwt_readeventcounters(&app.event_counts);   // take a snapshot of event
                                              //   counters

  dwt_rxenable(DWT_START_RX_IMMEDIATE);       // re-enable receiver again - no
                                              //   timeout

  /* ready to serve next raw reception */
}

void listener_timeout_cb(const dwt_cb_data_t *rxd)
{
  (void) rxd;

  if (app.pConfig->s.stsStatic) { // value 0 = dynamic STS, 1 = fixed STS
    // re-load the initial cp_iv value to keep STS the same for each frame
    dwt_configurestsloadiv();
  }

  dwt_readeventcounters(&app.event_counts);

  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

void listener_error_cb(const dwt_cb_data_t *rxd)
{
  listener_timeout_cb(rxd);
}

// -----------------------------------------------------------------------------

/* @brief     app level
 *     RTOS-independent application level function.
 *     initializing of a TWR Node functionality.
 *
 * */
error_e listener_process_init()
{
  if (!psListenerInfo) {
    psListenerInfo = LSTNR_MALLOC(sizeof(listener_info_t));
  }

  listener_info_t  *pListenerInfo = getListenerInfoPtr();

  if (!pListenerInfo) {
    return(_ERR_Cannot_Alloc_NodeMemory);
  }

  /* switch off receiver's rxTimeOut, RxAfterTxDelay, delayedRxTime,
   * autoRxEnable, dblBufferMode and autoACK,
   * clear all initial counters, etc.
   * */
  memset(pListenerInfo, 0, sizeof(listener_info_t));

  memset(&app.event_counts, 0, sizeof(app.event_counts));

  app.event_counts_sts_bad = 0;
  app.event_counts_sts_good = 0;
  app.event_counts_sfd_detect = 0;

  /* Configure non-zero initial variables.1 : from app parameters */

  /* The Listener has its configuration in the app->pConfig, see DEFAULT_CONFIG.
   *
   *
   * */
  pListenerInfo->pSfConfig = &app.pConfig->s.sfConfig;   /**< Super Frame
                                                          *   configuration */

  /* dwt_xx calls in app level Must be in protected mode (DW3000 IRQ disabled)
   */
  disable_dw3000_irq();

  TWR_ENTER_CRITICAL();

  if (dwt_initialise(DWT_DW_INIT) != DWT_SUCCESS) {  /**< set callbacks to NULL
                                                      *   inside
                                                      *   dwt_initialise*/
    return (_ERR_INIT);
  }

  set_dw_spi_fast_rate();

  uint32_t dev_id = dwt_readdevid();

  if ((dev_id == (uint32_t)DWT_DW3000_PDOA_DEV_ID)
      || (dev_id == (uint32_t)DWT_QM33120_PDOA_DEV_ID)) {
    pListenerInfo->dw3000ChipType = AOA;

    diag_printf("Found AOA DW3000 chip. PDoA is available.\r\n");
  } else if (dev_id == (uint32_t)DWT_DW3000_DEV_ID) {
    pListenerInfo->dw3000ChipType = NON_AOA;

    app.pConfig->dwt_config.pdoaMode = DWT_PDOA_M0;

    diag_printf("Found non-AOA DW3000 chip. PDoA is not available.\r\n");
  } else {
    diag_printf("Found unknown chip 0x%08lX. Stop.\r\n",
                (unsigned long int)dev_id);
    return _ERR_DEVID;
  }

  /* Configure DW IC's UWB mode, sets power and antenna delays for TWR mode
   * Configure SPI to fast rate */
  rxtx_listener_configure(&app.pConfig->dwt_config,
                          DWT_FF_DISABLE,          /* No frame filtering for
                                                    *   Listener */
                          app.pConfig->s.antTx_a,
                          app.pConfig->s.antRx_a
                          );

  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);        /**< DEBUG I/O 2&3
                                                              *   : configure
                                                              *   the GPIOs
                                                              *   which control
                                                              *   the LEDs on HW
                                                              */
  dwt_setlnapamode(DWT_PA_ENABLE | DWT_LNA_ENABLE);          /**< DEBUG I/O 5&6
                                                              *   : configure
                                                              *   TX/RX states
                                                              *   to output on
                                                              *   GPIOs
                                                              */

  dwt_setcallbacks(NULL,
                   rx_listener_cb,
                   listener_timeout_cb,
                   listener_error_cb,
                   NULL,
                   NULL,
                   NULL);

  dwt_setinterrupt(DWT_INT_TXFRS_BIT_MASK | DWT_INT_RXFCG_BIT_MASK
                   | (DWT_INT_ARFE_BIT_MASK | DWT_INT_RXFSL_BIT_MASK
                      | DWT_INT_RXSTO_BIT_MASK | DWT_INT_RXPHE_BIT_MASK
                      | DWT_INT_RXFCE_BIT_MASK | DWT_INT_RXFTO_BIT_MASK

                      /*| SYS_STATUS_RXPTO_BIT_MASK */),
                   0,
                   2);

  dwt_configciadiag(DW_CIA_DIAG_LOG_ALL);           /* DW3000 */

  // configure STS KEY/IV
  dwt_configurestskey(&app.pConfig->s.stsKey);
  dwt_configurestsiv(&app.pConfig->s.stsIv);
  // load the configured KEY/IV values
  dwt_configurestsloadiv();

  dwt_configeventcounters(1);

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
  }

  /* End configuration of DW IC */
  rtc_disable_irq();

  TWR_EXIT_CRITICAL();

  return (_NO_ERR);
}

/*
 * @brief
 *     Enable DW3000 IRQ to start
 * */
void listener_process_start(void)
{
  enable_dw3000_irq();

  // start the RTC timer
  rtc_enable_irq();

  diag_printf("Listener Top Application: Started\r\n");
}

/* @brief     app level
 *     RTOS-independent application level function.
 *     deinitialize the pListenerInfo structure.
 *    This must be executed in protected mode.
 *
 * */
void listener_process_terminate(void)
{
  rtc_disable_irq();

  if (psListenerInfo) {
    LSTNR_FREE(psListenerInfo);
    psListenerInfo = NULL;
  }
}

// -----------------------------------------------------------------------------
