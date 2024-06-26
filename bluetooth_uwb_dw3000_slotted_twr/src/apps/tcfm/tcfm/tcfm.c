/**
 * @file     tcfm.c
 * @brief    process to run Test Continuous Frame Mode
 *
 *           This test application will send a number of packets (e.g. 200) and
 *   then stop
 *           The payload and the inter-packet period can also be varied
 *           command: "TCFM N D P", where N is number of packets to TX, D is
 *   inter packet period (in ms), P is payload in bytes
 *
 *
 *    measure the power:
 *    Spectrum Analyser set:
 *    FREQ to be channel default e.g. 6489.6 MHz for channel 5, 7987.2 MHz for
 *   channel 9
 *    SPAN to 1GHz
 *    SWEEP TIME 1s
 *    RBW and VBW 1MHz
 *    measure channel power
 *    measure peak power
 *
 *
 * @author   Decawave
 *
 * @attention
 *             Copyright 2017-2020 (c) Decawave Ltd, Dublin, Ireland.
 *          All rights reserved.
 * */
#include <stdlib.h>
#include <string.h>
#include "tcfm.h"

#include "msg_time.h"
#include "port.h"
#include "port_common.h"
#include "uwb_frames.h"

#include "deca_device_api.h"

#define DW_MS_PERIOD 249600    /* 1 ms in 4ns units to program into DX_TIME_ID
                                */

struct tcfm_app_s
{
  uint8_t payload[127];
  uint16_t msg_len;
  uint16_t msg_count;
  uint16_t nframes;
  uint8_t fixed_sts;
};

static struct tcfm_app_s *pTcfmMsg = NULL;

/* IMPLEMENTATION */

/* @brief   ISR level
 *          TCFM application TX callback
 *          to be called from dwt_isr() as an TX call-back
 * */
void tcfm_tx_cb(const dwt_cb_data_t *rxd)
{
  (void) rxd;
  if (!pTcfmMsg) {
    return;
  }

  if (pTcfmMsg->msg_count >= pTcfmMsg->nframes) {
    // we have transmitted required number of messages - stop the application

    // FreeRTOS specific implementation of how-to set the Event from the ISR
    BaseType_t xHigherPriorityTaskWoken, xResult;

    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(app.xStartTaskEvent, Ev_Stop_All,
                                        &xHigherPriorityTaskWoken);

    // Was the message posted successfully?
    if (xResult == pdPASS) {
      // If xHigherPriorityTaskWoken is now set to pdTRUE then a context
      // switch should be requested.  The macro used is port specific and
      // will be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() -
      // refer to the documentation page for the port being used.
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  } else {
    pTcfmMsg->msg_count++;

    pTcfmMsg->payload[1] = pTcfmMsg->msg_count;         // packet counter 16-bit
    pTcfmMsg->payload[2] = pTcfmMsg->msg_count >> 8;

    dwt_writetxdata(pTcfmMsg->msg_len, (uint8_t *)pTcfmMsg->payload, 0);

    dwt_writetxfctrl(pTcfmMsg->msg_len, 0, 0);

    // the configured delay in between TX packets was set in the init
    if (pTcfmMsg->fixed_sts) {
      // re-load the initial cp_iv value to keep STS the same for each frame
      dwt_configurestsloadiv();
    }
    dwt_starttx(DWT_START_TX_DLY_TS);
  }
}

/*
 * @brief     init function initialises all run-time environment allocated by
 *   the process
 *             it will be executed once
 *
 * */
error_e tcfm_process_init(tcfm_info_t *info)
{
  error_e ret = _NO_ERR;

  pTcfmMsg = malloc(sizeof(struct tcfm_app_s));

  if (!pTcfmMsg) {
    return _ERR_Cannot_Alloc_Memory;
  }

  // define some test data for the tx buffer
  const uint8_t msg_data[] = "The quick brown fox jumps over the lazy dog";

  // configure device settings based on the settings stored in app.pConfig
  // it will not return if the init will fail
  tcXm_configure_test_mode();

  dwt_setcallbacks(tcfm_tx_cb, NULL, NULL, NULL, NULL, NULL, NULL);

  dwt_setinterrupt(DWT_INT_TXFRS_BIT_MASK, 0, DWT_ENABLE_INT_ONLY);

  init_dw3000_irq();              /**< manually init EXTI DW3000 lines IRQs */

  pTcfmMsg->fixed_sts = app.pConfig->s.stsStatic;   // value 0 = dynamic STS, 1
                                                    //   = fixed STS

  // configure STS KEY/IV
  dwt_configurestskey(&app.pConfig->s.stsKey);
  dwt_configurestsiv(&app.pConfig->s.stsIv);
  // load the configured KEY/IV values
  dwt_configurestsloadiv();

  /* Setup Tx packet*/
  pTcfmMsg->msg_len = (uint16_t) info->bytes;      // overall message length
  pTcfmMsg->msg_count = 1;
  pTcfmMsg->nframes = (uint16_t) info->nframes;

  memcpy(pTcfmMsg->payload, msg_data, info->bytes);

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

  if (info->bytes == 5) { // Special interop case
    pTcfmMsg->payload[0] = 0x10;      //
    pTcfmMsg->payload[1] = pTcfmMsg->msg_count;         // packet counter 16-bit
    pTcfmMsg->payload[2] = pTcfmMsg->msg_count >> 8;
  }

  dwt_writetxdata(pTcfmMsg->msg_len, (uint8_t *)pTcfmMsg->payload, 0);

  dwt_writetxfctrl(pTcfmMsg->msg_len, 0, 0);

  // If the length of the packet is > period_ms, the packets will be sent
  //   back-to-back
  dwt_setdelayedtrxtime(DW_MS_PERIOD * info->period_ms);

  return ret;
}

/*
 * @brief     run function implements continuous process functionality
 * */
void tcfm_process_run(void)
{
  /*do nothing*/
}

/*
 * @brief     stop function implements stop functionality if any
 *             which will be executed on reception of Stop command
 * */
void tcfm_process_terminate(void)
{
  port_stop_all_UWB();

  if (pTcfmMsg) {
    free(pTcfmMsg);
    pTcfmMsg = NULL;
  }
}
