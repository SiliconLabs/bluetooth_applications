/*
 * @file      common_n.c
 * @brief     Decawave Application Layer
 *            Common functions for twr tag and node
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include "app.h"
#include "common_n.h"
#include "port_common.h"

#pragma GCC diagnostic ignored "-Wunused-function"

/*
 * @brief This fn() sets the STS_ND (SP3) and loads the STS key and IV
 *
 *  dwt_configure(pdwCfg) should be executed thereafter
 * */
static void
set_cfg_sp3(dwt_config_t      *pdwCfg,
            dwt_sts_cp_key_t *key,
            dwt_sts_cp_iv_t     *iv)
{
  (void) pdwCfg;
  pdwCfg->stsMode = DWT_STS_MODE_ND;
  pdwCfg->sfdType = 3;

  /*
   * Set STS encryption key and IV (nonce)
   */
  dwt_configurestskey(key);                     // this can be done only 1 time
                                                //   in the application
  dwt_configurestsiv(iv);                       // the IV should be synchronised
  dwt_configurestsloadiv();
  // END STS
}

/*
 * @brief This fn() sets the STS1 (SP1) and loads the STS key and IV
 *
 *  dwt_configure(pdwCfg) should be executed thereafter
 * */
static void
set_cfg_sp1(dwt_config_t    *pdwCfg,
            dwt_sts_cp_key_t *key,
            dwt_sts_cp_iv_t  *iv)
{
  (void) pdwCfg;
  pdwCfg->stsMode = DWT_STS_MODE_1;
  pdwCfg->sfdType = 3;

  /*
   * Set STS encryption key and IV (nonce)
   */
  dwt_configurestskey(key);                     // this can be done only 1 time
                                                //   in the application
  dwt_configurestsiv(iv);                       // the IV should be synchronised
  dwt_configurestsloadiv();
  // END STS
}

/*
 * @brief This fn() revert back SP0 frame mode
 *        dwt_configure(pdwCfg) should be executed thereafter
 * */
static void
set_cfg_sp0A(dwt_config_t *pdwCfg)
{
  (void) pdwCfg;
  pdwCfg->stsMode = DWT_STS_MODE_OFF;
  pdwCfg->sfdType = 0;
}

static void
set_cfg_sp0Z(dwt_config_t *pdwCfg)
{
  (void) pdwCfg;
  pdwCfg->stsMode = DWT_STS_MODE_OFF;
  pdwCfg->sfdType = 3;
}

/*
 * @brief   configure tan/node application: frame filtering, PANID, address,
 *   antenna delays
 *
 *
 * @note
 * */
void
tn_app_config(rxtx_configure_t *p)
{
  /* set antenna delays */
  dwt_setrxantennadelay(p->rxAntDelay);
  dwt_settxantennadelay(p->txAntDelay);

  dwt_setrxaftertxdelay(0);      /**< no any delays set by default : part of
                                  *   config of receiver on Tx sending
                                  */
  dwt_setrxtimeout(0);           /**< no any delays set by default : part of
                                  *   config of receiver on Tx sending
                                  */

  dwt_configureframefilter(p->frameFilter, p->frameFilterMode);

  dwt_setpanid(p->panId);

  dwt_setaddress16(p->shortadd);
}

/*
 * @brief   ISR level (need to be protected if called from APP level)
 *          low-level configuration for DW1000
 *
 *          if called from app, shall be performed with DW IRQ off &
 *          TAG_ENTER_CRITICAL(); / TAG_EXIT_CRITICAL();
 *
 *
 * @note
 * */
void
rxtx_configure(rxtx_configure_t *p)
{
  if (dwt_configure(p->pdwCfg)) {   /**< Configure the Physical Channel
                                     *   parameters (PLEN, PRF, etc) */
    error_handler(1, _ERR_INIT);
  }

  /* configure power */
  dwt_configuretxrf(&app.pConfig->s.txConfig);

  tn_app_config(p);
}

/**
 * @brief   ISR level (need to be protected if called from APP level)
 *          Transmit packet
 * */
error_e
tx_start(tx_pckt_t *pTxPckt)
{
  error_e ret = _NO_ERR;

  if (pTxPckt->psduLen) {
    dwt_writetxdata(pTxPckt->psduLen, (uint8_t *) &pTxPckt->msg.stdMsg, 0);
    dwt_writetxfctrl(pTxPckt->psduLen, 0, 1);
  }

  // Setup for delayed Transmit time (units are 4ns)
  if (pTxPckt->txFlag
      & (DWT_START_TX_DELAYED | DWT_START_TX_DLY_REF | DWT_START_TX_DLY_RS
         | DWT_START_TX_DLY_TS)) {
    dwt_setdelayedtrxtime(pTxPckt->delayedTxTimeH_sy);
  }

  // Setup for delayed Receive after Tx (units are sy = 1.0256 us)
  if (pTxPckt->txFlag & DWT_RESPONSE_EXPECTED) {
    dwt_setrxaftertxdelay(pTxPckt->delayedRxTime_sy);
    dwt_setrxtimeout(pTxPckt->delayedRxTimeout_sy);
  }

  // Begin delayed TX of frame
  if (dwt_starttx(pTxPckt->txFlag) != DWT_SUCCESS) {
    ret = _ERR_DelayedTX_Late;
  }

  return (ret);
}
