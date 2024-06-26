/**
 * @file    tag.h
 *
 * @brief   tag bare implementation
 *
 * @attention
 *
 * Copyright 2016-2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author
 */

#ifndef __TAG__H__
#define __TAG__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "uwb_frames.h"
#include "msg_time.h"

#include "port.h"
#include "port_common.h"

#define DW_TAG_NOT_SLEEPING            (0)

#define BLINK_PERIOD_MS                (500) /* range init phase - Blink send
                                              *   period, ms */

#define DWT_DIAGNOSTIC_LOG_REV_5       (5)

/* Rx Events circular buffer.
 * 0x02, 0x04, 0x08, 0x10, etc.
 * The size of the buffer at any given time should be < 2 */
#define EVENT_BUF_TAG_SIZE             (0x02)

#define POLL_ENTER_CRITICAL()     vPortEnterCritical()
#define POLL_EXIT_CRITICAL()      vPortExitCritical()
// -----------------------------------------------------------------------------
// Struct & Typedefs

/* RxPckt */
struct rx_pckt_t_s
{
  int16_t        rxDataLen;

  union {
    std_msg_t           stdMsg;
    std_msg_ss_t        ssMsg;
    std_msg_ls_t        lsMsg;
    twr_msg_t           twrMsg;
    blink_msg_t         blinkMsg;
    rng_cfg_msg_t       rngCfgMsg;
    rng_cfg_upd_msg_t   rngCfgUpdMsg;
    resp_pdoa_msg_t      respExtMsg;
  } msg;

  uint8_t     timeStamp[TS_40B_SIZE];     /* Full TimeStamp */
  uint32_t    rtcTimeStamp;               /* MCU RTC timestamp */
  uint16_t    firstPath;                    /* First path (raw 10.6) */
  int16_t     clock_offset;

#if (DIAG_READ_SUPPORT == 1)
  diag_v5_t   diagnostics;                  /* 66 bytes*/
#endif
};

typedef struct rx_pckt_t_s rx_pckt_t_t;

/* This structure holds application parameters:
 * eui64
 * txAntennaDelay
 * rxAntennaDelay
 * timestamps for every phase's IRQ:
 *             initiator: blinkTx_ts, pollTx_ts, respRX_ts, finalTx_ts,
 *   (reportRx_ts)
 *             responder: blinkRx_ts, pollRx_ts, respTx_ts, finalRx_ts,
 *   (reportTx_ts)
 *
 * */
struct tag_info_s
{
  /* Unique long Address, used at the discovery phase before Range Init
   *   reception */
  union    {
    uint8_t  euiLong[8];
    uint64_t eui64;
  };

  /* circular Buffer of received Rx packets :
   * uses in transferring of the data from ISR to APP level.
   * */
  struct {
    rx_pckt_t_t   buf[EVENT_BUF_TAG_SIZE];
    uint16_t    head;
    uint16_t    tail;
  } rxPcktBuf;

  /* ranging variables */
  struct {
    /* MAC sequence number, increases on every tx_start */
    uint8_t        seqNum;

    /* Discovery phase : Tx time structures for DW_TX_IRQ callback */
    struct {
      uint8_t     blinkTx_ts[TS_40B_SIZE];         /**< tag: blinkTx_ts,
                                                    *   blinkRtcTimeStamp */
      uint32_t blinkRtcTimeStamp;               /**< handles the MCU RTC time at
                                                 *   the DW_IRQ */
    };

    /* Ranging phase : Tx time structures for DW_TX_IRQ callback */
    struct {
      uint8_t  pollTx_ts[TS_40B_SIZE];          /**< tag:  pollTx_ts,
                                                 *   pollRtcTimeStamp */
      uint32_t pollRtcTimeStamp;                /**< handles the MCU RTC time at
                                                 *   the DW_IRQ */

      uint8_t  finalTx_ts[TS_40B_SIZE];         /**< tag:  finalTx_ts,
                                                 *   finalRtcTimeStamp */
      uint32_t finalRtcTimeStamp;               /**< handles the MCU RTC time at
                                                 *   the DW_IRQ */
    };

    /* Application DW_TX_IRQ source indicator */
    tx_states_e     txState;
  };

  /* pre-calculated times for different messages */
  struct {
    msg_time_t    ranging_config;
    msg_time_t    poll;
    msg_time_t    response;
    msg_time_t    final;
  } msg_time;

  uint32_t        gRtcSFrameZeroCnt;          // Local SuperFrame start,
                                              //   Timestamp

  /* Environment - configured from Range init structure.
   * slotCorr_us is used to adjust slot every reception as part of Response
   */
  struct    env
  {
    uint8_t     version;

    mac_header_ss_t twr_mac_header;

    uint16_t    panID;
    uint8_t     tagAddr[ADDR_BYTE_SIZE_S];
    uint8_t     nodeAddr[ADDR_BYTE_SIZE_S];

    uint32_t    sframePeriod_ns;            // Superframe Period, ns
    uint64_t    pollTx2FinalTxDelay64;      // This is delay used in TWR between
                                            //   Poll and Final sending: from
                                            //   Ranging Config message

    uint16_t    responseRxTo_sy;     // pre-calculated Rx timeout for Response
                                     //   Msg
    uint32_t    delayRx_sy;          // Rx timeout from Ranging Config for
                                     //   Response Msg to the Node

    int32_t     slotCorr_ns;         // Slot correction from current reception,
                                     //   ns

    uint16_t    pollMultFast;        // multiplier for fast ranging in
                                     //   Superframe durations
    uint16_t    pollMultSlow;        // multiplier for slow ranging in
                                     //   Superframe durations

    union {
      uint16_t     mode;             // additional service: IMU on/off, etc.TBD
      bool        imuOn : 1;         // tag shall use IMU to slow down its
                                     //   ranging
    };
  } env;

  union {
    struct acc {
      uint8_t     acc_x[2];
      uint8_t     acc_y[2];
      uint8_t     acc_z[2];
    } acc;

    struct pos {
      uint8_t     pos_x[2];
      uint8_t     pos_y[2];
      uint8_t     pos_z[2];
    } pos;
  };

  bool        stationary_imu    : 1;   // IMU report that the Tag is stationary
  bool        stationary        : 1;   // IMU report that the Tag is stationary

  /* The number of range sequence, increases on every poll */
  uint16_t    rangeNum;

  /* Tag's crystal clock offset trimming */
  int16_t     clkOffset_pphm;       //
  uint8_t     xtaltrim;             // Tag crystal trim value

  volatile
  uint16_t    faultyRangesCnt;

  uint16_t    lateTX;               // used for Debug to count any lateTX

  enum
  {
    BLINKING_MODE,
    RANGING_MODE
  }            mode;

  dw3000type_e dw3000ChipType;
};

typedef struct tag_info_s tag_info_t;

// -----------------------------------------------------------------------------
// exported functions prototypes

/* initiator (tag) */
tag_info_t * getTagInfoPtr(void);

error_e tag_process_init(void);
void    tag_process_start(void);
void    tag_process_terminate(void);

error_e twr_initiator_algorithm_rx(rx_pckt_t_t *prxPckt, tag_info_t *ptwrInfo);

error_e tag_wakeup_dw3000_blink_poll(tag_info_t *ptwrInfo);
error_e initiator_send_blink(tag_info_t *ptwrInfo);
error_e initiator_send_poll(tag_info_t *ptwrInfo);
error_e initiator_received_ranging_config(rx_pckt_t_t *prxPckt,
                                          tag_info_t *ptwrInfo);
error_e initiator_received_response(rx_pckt_t_t *prxPckt, tag_info_t *ptwrInfo);

void twr_configure_rtc_wakeup_ns(uint32_t ns);
void trim_tag_proc(tag_info_t *pTwrInfo);

#ifdef __cplusplus
}
#endif

#endif /* __TAG__H__ */
