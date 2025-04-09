/**
 * @file    node.h
 *
 * @brief   Decawave
 *             bare implementation layer
 *
 * @author Decawave
 *
 * @attention Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __NODE__H__
#define __NODE__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "uwb_frames.h"
#include "msg_time.h"
#include "tag_list.h"

#include "port_dw3000.h"
#include "port_common.h"
#include "common_n.h"

// -----------------------------------------------------------------------------
// Definitions
#define FULL_ACC_LEN      (1016)
#define ACC_OFFSET        (300)

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

/* WKUP timer counts Super Frame period.
 * The WKUP timer resolution is (61035) counts in 1 ns.
 */
#define SPEED_OF_LIGHT                  (299702547.0) // in m/s in the air
#define DWT_DIAGNOSTIC_LOG_REV_5        (5)
#define INVALID_TOF                     (0xABCDFFFF)

/* Bitmask for result_t flag's.
 * Note, byte_0 is bitmask from a tag;
 * byte_1 is other flags:
 * */
#define RES_FLAG_PDOA_OFFSET_ZERO_BIT   (1 << 15) /**< This bit will be set to
                                                   *   the report when the
                                                   *   "pdoa_offset" is zero
                                                   */
#define RES_FLAG_RANGE_OFFSET_ZERO_BIT  (1 << 14) /**< This bit will be set to
                                                   *   the report when the
                                                   *   "rng_offset" is zero
                                                   */

// -----------------------------------------------------------------------------

/*
 * Rx Events circular buffer : used to transfer RxPckt from ISR to APP
 * 0x02, 0x04, 0x08, 0x10, etc.
 * As per design, the amount of RxPckt in the buffer at any given time shall not
 *   be more than 1.
 * */
#define EVENT_BUF_NODE_SIZE             (0x02)

// -----------------------------------------------------------------------------

/*
 * RX_MAIL_QUEUE_SIZE : used to transfer RxPckt from rxTask() to calkTask()
 * it shall be smaller than bare-metal circular buffer EVENT_BUF_SIZE
 * See note to osMailQDef(rxPcktPool_q, RX_MAIL_QUEUE_SIZE, rx_mail_t);
 * */
#define RX_MAIL_QUEUE_SIZE              (0x01)

// -----------------------------------------------------------------------------
// Typedefs

/* RxPckt is the structure is for the current reception */
struct rx_pckt_s
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
  } msg;

  tag_addr_slot_t        *tag;                  /* the tag, to which the current
                                                 *   range exchange is
                                                 *   performing */

  uint8_t     timeStamp[TS_40B_SIZE];           /* TimeStamp of current RX:
                                                 *   blinkRx, pollRx, finalRx */
  uint32_t    rtcTimeStamp;                     /* MCU current Rx RTC timestamp:
                                                 *   make sense for the Poll
                                                 *   message reception */

  uint8_t     temperature;                      /* raw reading of temperature:
                                                 *   valid at reception of Final
                                                 */

  pdoa_t      PDOA;

  /* Below is Decawave's diagnostics information */
  uint32_t    status;

  dwt_rxdiag_t    diag_dw3000;                  /*TODO: review diagnostics */

  uint8_t     acc[(FULL_ACC_LEN - ACC_OFFSET) * 4 + 1];/* TODO:
                                                        * review accumulator
                                                        * reading /
                                                        * do we need it
                                                        * in this app ? /
                                                        * Will be started
                                                        * with offset
                                                        * ACC_OFFSET :
                                                        * for ACC_OFFSET = 0
                                                        * 2x ((1016-0)*4 + 1)
                                                        * = 2x 4065 = 8130 bytes
                                                        * for raw ACC
                                                        *
                                                        * for ACC_OFFSET = 300
                                                        * 2x ((1016-300)*4 + 1)
                                                        * = 2x 2865 = 5730 bytes
                                                        * for raw ACC
                                                        */
};

typedef struct rx_pckt_s rx_pckt_t;

/* Mail from RxTask to CalcTask*/
struct rx_mail_s
{
  tag_addr_slot_t *tag;         /* the tag, to which the current range was
                                 *   performed */

  result_t    res;

  uint8_t     tagPollTx_ts[TS_40B_SIZE];
  uint8_t     tagRespRx_ts[TS_40B_SIZE];
  uint8_t     tagFinalTx_ts[TS_40B_SIZE];

  uint8_t     nodePollRx_ts[TS_40B_SIZE];
  uint8_t     nodeRespTx_ts[TS_40B_SIZE];
  uint8_t     nodeFinalRx_ts[TS_40B_SIZE];

  /* Below is Decawave's diagnostics information */
  dwt_rxdiag_t    diag_dw3000;
  uint8_t     acc[(FULL_ACC_LEN - ACC_OFFSET) * 4 + 1];/* TODO:
                                                        * review accumulator
                                                        * reading /
                                                        * do we need it
                                                        * in this app ? /
                                                        * Will be started
                                                        * with offset
                                                        * ACC_OFFSET :
                                                        * for ACC_OFFSET = 0
                                                        * 2x ((1016-0)*4 + 1)
                                                        * = 2x 4065 = 8130 bytes
                                                        * for raw ACC
                                                        *
                                                        * for ACC_OFFSET = 300
                                                        * 2x ((1016-300)*4 + 1)
                                                        * = 2x 2865 = 5730 bytes
                                                        * for raw ACC
                                                        */
};

typedef struct rx_mail_s rx_mail_t;

/* This structure holds Node's TWR application parameters */
struct node_info_s
{
  /* Unique short Address, uses at the ranging phase
   * valid for low-endian compiler.
   * */
  union    {
    uint8_t     euiShort[2];
    uint16_t    eui16;
  };

  /* circular Buffer of received Rx packets :
   * uses in transferring of the data from ISR to APP level.
   * */
  struct {
    rx_pckt_t   buf[EVENT_BUF_NODE_SIZE];
    uint16_t    head;
    uint16_t    tail;
  } rxPcktBuf;

  /* ranging run-time variables */
  struct {
    /* MAC sequence number, increases on every tx_start */
    uint8_t        seqNum;

    /* Node's Discovery phase : Tx time structures for DW_TX_IRQ callback */
    struct {
      uint8_t  rangeInitTx_ts[TS_40B_SIZE];         /**< node: rangeInitTx_ts,
                                                     *   rangeInitRtcTimeStamp
                                                     */
      uint32_t rangeInitRtcTimeStamp;               /**< handles the MCU RTC
                                                     *   time at the DW_IRQ
                                                     */
    };

    /* Node's Ranging phase : Tx time structures for DW_TX_IRQ callback */
    struct {
      uint8_t  nodeRespTx_ts[TS_40B_SIZE];          /**< node: nodeRespTx_ts,
                                                     *   respRtcTimeStamp */
      uint32_t respRtcTimeStamp;                    /**< handles the MCU RTC
                                                     *   time at the DW_IRQ
                                                     */
    };

    /* Node's Ranging phase : Rx time structures for DW_TX_IRQ callback */
    struct {
      uint8_t  nodePollRx_ts[TS_40B_SIZE];          /**< temporary for current
                                                     *   range exchange:
                                                     *   received time of poll
                                                     *   message */
      uint32_t pollRtcTimeStamp;                    /**< temporary for current
                                                     *   range exchange:
                                                     *   received time of poll
                                                     *   message RTC time */
    };

    /* Node's Ranging phase : DW3000 PDOA debug */
    pdoa_t      pollPDOA;       /**< DW3000 PDOA result from Poll message */
    pdoa_t      finalPDOA;      /**< DW3000 PDOA result from Poll message */

    /* node is ranging to a single tag in a range exchange sequence */
    tag_addr_slot_t *pDestTag;                  /**< tag, with whom the current
                                                 *   range is performing */

    /* Application DW_TX_IRQ source indicator */
    tx_states_e        txState;
  };

  /* pre-calculated times for different messages */
  struct {
    msg_time_t    blink;
    msg_time_t    ranging_config;
    msg_time_t    poll;
    msg_time_t    response;
    msg_time_t    final;
  } msg_time;

  uint16_t    panID;
  sfConfig_t  *pSfConfig;               // superFrame configuration

  uint64_t    newTag_addr64;            // new discovered tag address

  result_t    result[MAX_KNOWN_TAG_LIST_SIZE];   /* range/X/Y/Xtal_offset result
                                                  *   back to the tag */

  volatile uint32_t gRtcSFrameZeroCnt;  // SuperFrame Cnt RTC timestamp

  uint32_t    lateTxCount;              // indicate that Delayed Tx timings
                                        //   failed

  struct {
    unsigned int imuOn         : 1;
    unsigned int stationary    : 1;
  };

  uint8_t      Tmeas;              // copy of Temperature calibration value for
                                   //   DW chip

  dw3000type_e dw3000ChipType;
};

typedef struct node_info_s node_info_t;

// -----------------------------------------------------------------------------
// exported functions prototypes
//
extern node_info_t * getNodeInfoPtr(void);

// -----------------------------------------------------------------------------
// exported functions prototypes
//

/* responder (node) */
error_e twr_responder_algorithm_rx(rx_pckt_t *pRxPckt, node_info_t *pNodeInfo);
void    twr_configure_rtc_wakeup_ms(uint32_t     period);
void    synchronize_DW1000clocks(void);

error_e node_process_init(void);
void    node_process_start(void);
void    node_process_terminate(void);

error_e tof2range(param_block_t *, float *, int32_t);
void    pdoa2XY(result_t *, uint8_t);

// -----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* __NODE__H__ */
