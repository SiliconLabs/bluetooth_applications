/**
 * @file    listener.h
 *
 * @brief   Decawave
 *             bare implementation layer
 *
 * @author Decawave
 *
 * @attention Copyright 2020 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __LISTENER__H__
#define __LISTENER__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "uwb_frames.h"
#include "port_dw3000.h"
#include "port_common.h"

// -----------------------------------------------------------------------------

/*
 * Rx Events circular buffer : used to transfer RxPckt from ISR to APP
 * 0x02, 0x04, 0x08, 0x10, etc.
 * As per design, the amount of RxPckt in the buffer at any given time shall not
 *   be more than 1.
 * */
#define EVENT_BUF_L_SIZE      (0x10)

// -----------------------------------------------------------------------------

/* RxPckt is the structure is for the current reception */
struct rx_listener_pckt_s
{
  int16_t        rxDataLen;

  union {
    std_msg_t           stdMsg;
    twr_msg_t           twrMsg;
    blink_msg_t         blinkMsg;
    rng_cfg_msg_t       rngCfgMsg;
    poll_msg_t          pollMsg;
    resp_pdoa_msg_t     respMsg;
    final_msg_accel_t   finalMsg;
    uint8_t             data[STANDARD_FRAME_SIZE];
  } msg;

  uint8_t     timeStamp[TS_40B_SIZE];     /* Full TimeStamp */

  /* Below is Decawave's diagnostics information */
  uint32_t    status;
  int16_t     clock_offset;
};

typedef struct rx_listener_pckt_s rx_listener_pckt_t;

/* This structure holds Listener's application parameters */
struct listener_info_s
{
  /* circular Buffer of received Rx packets :
   * uses in transferring of the data from ISR to APP level.
   * */
  struct {
    rx_listener_pckt_t  buf[EVENT_BUF_L_SIZE];
    uint16_t            head;
    uint16_t            tail;
  } rxPcktBuf;

  sfConfig_t  *pSfConfig;               // superFrame configuration
  dw3000type_e dw3000ChipType;
};

typedef struct listener_info_s listener_info_t;

// -----------------------------------------------------------------------------
// exported functions prototypes
//
extern listener_info_t * getListenerInfoPtr(void);

// -----------------------------------------------------------------------------
// exported functions prototypes
//

/* responder (Listener) */

error_e listener_process_init(void);
void    listener_process_start(void);
void    listener_process_terminate(void);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* __LISTENER__H__ */
