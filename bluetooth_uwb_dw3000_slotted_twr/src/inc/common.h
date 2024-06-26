/**
 * @file      common.h
 *
 * @brief     Only common macros and enumerations
 *
 * @author    Decawave Software
 *
 * @attention Copyright 2017-2020 (c) DecaWave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef COMMON_H_
#define COMMON_H_                   1

#ifdef __cplusplus
extern "C" {
#endif

#define TS_40B_SIZE                 (5)

#define PHY_READ_DIG_ENABLE         (0x1)
#define PHY_READ_ACC_ENABLE         (0x2)

#define SPEED_OF_LIGHT              (299702547.0) // in m/s in air

#define CCP_FIXED_REPLY_DELAY_US    150000     /* us */

#ifndef EVENT_BUFSIZE
#define EVENT_BUFSIZE               0x20
#endif

#ifdef UWB_BH_ENABLE

/* With UWB_BH option the max throughput would be smaller, because each instance
 *   has the ETH_REPORT_BUFSIZE buffer */
# define NUM_BH_BUFFERS             (8) // the maximum number of supported
                                        //   downstream anchors: should be 1<<n.
                                        //   at least 1 for Self
                                        // The overal network RAM usage is
                                        //   ETH_REPORT_BUFSIZE*NUM_ETH_BUFFERS

# define ETH_REPORT_BUFSIZE         (0x400) // highest nibble must be 1,2,4,8;
                                            //   all low nibbles eq to 0. see
                                            //   note for NUM_ETH_BUFFERS

# define EVENT_RX_BUFSIZE           (0x04)  // highest nibble must be 1,2,4,8;
                                            //   all low nibbles eq to 0

# define EVENT_TX_BUFSIZE           (0x02)  // highest nibble must be 1,2,4,8;
                                            //   all low nibbles eq to 0
#else
# define NUM_BH_BUFFERS             (1)   // must be at least 1 for Self, i.e.
                                          //   CtrlServer #0
                                          // The overal network RAM usage is
                                          // ETH_REPORT_BUFSIZE*NUM_ETH_BUFFERS

# define ETH_REPORT_BUFSIZE         (0x400) // highest nibble must be 1,2,4,8;
                                            //   all low nibbles eq to 0

# define EVENT_RX_BUFSIZE           (0x20)  // highest nibble must be 1,2,4,8;
                                            //   all low nibbles eq to 0

# define EVENT_TX_BUFSIZE           (0x04)  // highest nibble must be 1,2,4,8;
                                            //   all low nibbles eq to 0
#endif

// -----------------------------------------------------------------------------
// NRF specific RTC config

/* WKUP timer counts Super Frame period.
 * The WKUP timer resolution is (30517.5) counts in 1 ns.
 */
#define WKUP_RESOLUTION_NS           (1e9f / 32768.0f)

/* WKUP timer used at Ranging phase.
 * It is counts the Super Frame period, received in the Ranging Config message
 * in Tag's local time domain.
 * */
#define WKUP_RESOLUTION_US           (WKUP_RESOLUTION_NS / 1e3f)

/* RTC WKUP timer counts Super Frame period.
 * The RTC WKUP timer prescaler is configured as each Tick count is 30.517 us.
 */
#define RTC_WKUP_PRESCALER           (0)

/* RTC WKUP timer counts Super Frame period.
 * The RTC WKUP timer is 32 bit counter. Counter oveflows at 2^32 - 16777216
 */
#define RTC_WKUP_CNT_OVFLW           (UINT32_MAX)
#define RTC_WKUP_CNT_OVFLW_MASK      (RTC_WKUP_CNT_OVFLW - 1)

// -----------------------------------------------------------------------------

#define TX_BUF_INC(x) (x = (x + 1) & (EVENT_TX_BUFSIZE - 1))
#define RX_BUF_INC(x) (x = (x + 1) & (EVENT_RX_BUFSIZE - 1))

/**/
#define APP_EV_TAG_BLINK_RX            (phyevent->rx[tail].msg.blinkMsg. \
                                        frameCtrl[0] == HEAD_MSG_BLINK)
#define APP_EV_CLOCK_SYNC_RX           ((phyevent->rx[tail].msg.ccpMsg. \
                                         messageData[0]                 \
                                         == RTLS_MSG_ANCH_CLK_SYNC) &&  \
                                        (phyevent->rx[tail].rxDataLen   \
                                         == sizeof(ccp_msg_t)))
#define APP_EV_DATA_RX                 (phyevent->rx[tail].msg.bcastMsg. \
                                        messageData[0] == APP_MSG_DATA)
#define APP_EV_UWB_BH_DWNSTREAM_RX     ((phyevent->rx[tail].msg.bcastMsg. \
                                         messageData[0]                   \
                                         == APP_MSG_UWB_BH_DWNSTREAM) &&  \
                                        (phyevent->rx[tail].rxDataLen     \
                                         == sizeof(uwb_bh_dwnstream_msg_t)))
#define APP_EV_UWB_BH_UPSTREAM_RX      ((phyevent->rx[tail].msg.bcastMsg. \
                                         messageData[0]                   \
                                         == APP_MSG_UWB_BH_UPSTREAM) &&   \
                                        (phyevent->rx[tail].rxDataLen     \
                                         == sizeof(uwb_bh_upstream_msg_t)))

// -----------------------------------------------------------------------------
// common macros

#ifndef SWAP
#define SWAP(a, b)   { a ^= b; b ^= a; a ^= b; }
#endif /* SWAP */

#ifndef MIN
#define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef TRUE
#define TRUE                  1
#endif /* TRUE */

#ifndef FALSE
#define FALSE                 0
#endif /* FALSE */

#define MASK_40BIT            (0x00FFFFFFFFFFULL)  // DW1000 counter is 40 bits
#define MASK_TXDTS            (0x00FFFFFFFE00ULL)  // The TX timestamp will snap
                                                   //   to 8 ns resolution -
                                                   //   mask lower 9 bits.

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H_ */
