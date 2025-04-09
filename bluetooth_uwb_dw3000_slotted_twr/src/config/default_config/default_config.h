/**
 * @file      default_config.h
 *
 * @brief     Default config file for NVM initialization
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __DEFAULT_CONFIG_H__H__
#define __DEFAULT_CONFIG_H__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "deca_device_api.h"
#include "tag_list.h"

/* UWB config */
#define DEFAULT_CHANNEL             9
#define DEFAULT_TXPREAMBLENGTH      DWT_PLEN_64
#define DEFAULT_RXPAC               DWT_PAC8
#define DEFAULT_PCODE               9
#define DEFAULT_NSSFD               1  //! SFD type 0 to use standard 8 symbol
                                       //!   SFD, 1 to use non-standard 8
                                       //!   symbol, 2 for non-standard 16
                                       //!   symbol SFD and 3 for 4z 8 symbol
                                       //!   SDF type
#define DEFAULT_DATARATE            DWT_BR_6M8
#define DEFAULT_PHRMODE             DWT_PHRMODE_STD
#define DEFAULT_PHRRATE             DWT_PHRRATE_STD
#define DEFAULT_SFDTO               (64 + 1 + 8 - 8)
#define DEFAULT_STS_MODE            (DWT_STS_MODE_1 | DWT_STS_MODE_SDC) //!< STS
                                                                        //!<   mode
#define DEFAULT_STS_LENGTH          DWT_STS_LEN_256  //!< STS length
#define DEFAULT_PDOA_MODE           DWT_PDOA_M3      //!< pdoa mode: on SP1/3
                                                     //!<   Ipatov_64 +
                                                     //!<   STS_256->PDoA_M3; if
                                                     //!<   Ipatov_64 + STS_64
                                                     //!<   -> PDoA Mode 1

#define DEFAULT_STS_STATIC          1 //! 1 to re-load STS Key & IV after each
                                      //!   Rx & Tx:: TCFM, Listener

/* run-time config */
#define DEFAULT_AUTOSTART_NONE      (0)
#define DEFAULT_NODE_ADDR           0x0001  /**< Addr16    */
#define DEFAULT_PANID               0xDECA  /**< PanID */
#define DEFAULT_UART                1       /**< Use UART I/O instead of USB CDC
                                               */
#define DEFAULT_ANTD                (513.484f * 1e-9 / DWT_TIME_UNITS) /*Total
                                                                        *   antenna
                                                                        *   delay*/
#define DEFAULT_PDOAOFF             0       /**< Phase Differences offset */
#define DEFAULT_RNGOFF              0       /**< Ranging offset */
#define DEFAULT_PDOA_TEMP           0       /**< Linear temperature coefficient
                                             *   for PDOA, mrad. Diff of PDOA
                                             *   when temperature changed +20C
                                             *   deg centigrade */
#define DEFAULT_ACCUM_READING       0       /**< */
#define DEFAULT_DIAG_READING        0       /**< */
#define DEFAULT_PHASECORR_EN        1       /**< enable antenna related phase
                                             *   correction polynomial */
#define DEFAULT_REPORT_LEVEL        1       /**< what to use as output of TWR:
                                             *   1:JSON, 2:Reduced, 3:Minimal */
#define DEFAULT_DEBUG               0       /**< if 1, then the LED_RED used to
                                             *   show an error, if any */

/* Slot configuration */
#define DEFAULT_SLOT_PERIOD_MS      5       /**< Slot period (ms), the best of
                                             *   what the implementation can
                                             *   achieve */
#define DEFAULT_NUM_SLOTS           20      /**< Number of slots: cannot be
                                             *   shorter than
                                             *   MAX_KNOWN_TAG_LIST_SIZE */
#define DEFAULT_SF_PERIOD_MS        100     /**< SuperFrame period: cannot be
                                             *   shorter than
                                             *   NUM_SLOTS*SLOT_PERIOD */

#if (DEFAULT_NUM_SLOTS < MAX_KNOWN_TAG_LIST_SIZE)
#error "error config: DEFAULT_NUM_SLOTS < MAX_KNOWN_TAG_LIST_SIZE"
#endif

#if (DEFAULT_SF_PERIOD_MS < (DEFAULT_NUM_SLOTS * DEFAULT_SLOT_PERIOD_MS))
#error \
  "error config: DEFAULT_SF_PERIOD_MS < (DEFAULT_NUM_SLOTS * DEFAULT_SLOT_PERIOD_MS)"
#endif

/* Below are 2 parameters to be sent to the Tag in the Ranging Config message.
 * Node uses these parameters to setup its receiver during TWR exchange with a
 *   Tag.
 * Tag hardware should be capable of performing the timings below:
 * */
#define DEFAULT_TAG_POLL_TX_RESPONSE_RX_US  (700)   /**< time when Tag shall be
                                                     *   ready to receive the
                                                     *   node's Response after
                                                     *   end of transmission of
                                                     *   the Poll.
                                                     *    This parameter is
                                                     *   Node's HW/SW dependent,
                                                     *   as the Node's
                                                     *   application is not
                                                     *   designed to meet
                                                     *   shorter timings.
                                                     *    this works in
                                                     *   optimizations -O2 and
                                                     *   -O3 */

#define DEFAULT_TAG_POLL_TX_FINAL_TX_US     (2000)  /**< time when Tag shall
                                                     *   transmit Final's
                                                     *   RMARKER, calculated
                                                     *   from Tag's Poll's
                                                     *   RMARKER. */

/* This configures the delay between end of Tag's Blink_TX and Tag start Rx of
 *   Ranging Config message.
 * From Node's view this is a delay between end of reception of Blink's data and
 *   start of transmission of preamble of Ranging Config.
 * Should be the same for Node and Tag.
 * */
#define DEFAULT_TAG_BLINK_TX_RC_RX_US       (150 * 1000) // Changed to 150 ms to
                                                         //   support
                                                         //   compatibility with
                                                         //   DecaRanging
                                                         //   Application

/* This configures the RX timeout while waiting for Ranging Config message */
#define DEFAULT_TAG_RC_RX_TIMEOUT_US        (500)

/* IP, NETMASK & GATEWAY addresses */
#define IP_ADDR0                            (uint8_t) 192
#define IP_ADDR1                            (uint8_t) 168
#define IP_ADDR2                            (uint8_t) 1
#define IP_ADDR3                            (uint8_t) 100

/*Gateway Address*/
#define GW_ADDR0                            (uint8_t) 192
#define GW_ADDR1                            (uint8_t) 168
#define GW_ADDR2                            (uint8_t) 1
#define GW_ADDR3                            (uint8_t) 1

/*NETMASK*/
#define NETMASK_ADDR0                       (uint8_t) 255
#define NETMASK_ADDR1                       (uint8_t) 255
#define NETMASK_ADDR2                       (uint8_t) 0
#define NETMASK_ADDR3                       (uint8_t) 0

#define DEFAULT_USE_STATIC_IP               (0)

/* NVM PAGE for store configuration */
#define FCONFIG_SIZE                        0x400   /**< can be up to 0x800 */

#define DEFAULT_CONFIG_SIGNATURE            0x0a1b2c3d

/* Default configuration initialization */
#define DEFAULT_CONFIG                                                        \
        {                                                                     \
          .dwt_config.chan = DEFAULT_CHANNEL,                                 \
          .dwt_config.txPreambLength = DEFAULT_TXPREAMBLENGTH,                \
          .dwt_config.rxPAC = DEFAULT_RXPAC,                                  \
          .dwt_config.txCode = DEFAULT_PCODE,                                 \
          .dwt_config.rxCode = DEFAULT_PCODE,                                 \
          .dwt_config.sfdType = DEFAULT_NSSFD,                                \
          .dwt_config.dataRate = DEFAULT_DATARATE,                            \
          .dwt_config.phrMode = DEFAULT_PHRMODE,                              \
          .dwt_config.phrRate = DEFAULT_PHRRATE,                              \
          .dwt_config.sfdTO = DEFAULT_SFDTO,                                  \
          .dwt_config.stsMode = DEFAULT_STS_MODE,                             \
          .dwt_config.stsLength = DEFAULT_STS_LENGTH,                         \
          .dwt_config.pdoaMode = DEFAULT_PDOA_MODE,                           \
                                                                              \
          .v.ver0 = 0xFFFFFFFF,                                               \
          .v.ver1 = 0xFFFFFFFF,                                               \
          .v.ver2 = 0xFFFFFFFF,                                               \
          .v.ver3 = 0xFFFFFFFF,                                               \
          .static_config.addr1 = IP_ADDR0,                                    \
          .static_config.addr2 = IP_ADDR1,                                    \
          .static_config.addr3 = IP_ADDR2,                                    \
          .static_config.addr4 = IP_ADDR3,                                    \
          .static_config.gw_addr1 = GW_ADDR0,                                 \
          .static_config.gw_addr2 = GW_ADDR1,                                 \
          .static_config.gw_addr3 = GW_ADDR2,                                 \
          .static_config.gw_addr4 = GW_ADDR3,                                 \
          .static_config.nm_addr1 = NETMASK_ADDR0,                            \
          .static_config.nm_addr2 = NETMASK_ADDR1,                            \
          .static_config.nm_addr3 = NETMASK_ADDR2,                            \
          .static_config.nm_addr4 = NETMASK_ADDR3,                            \
          .static_config.use_static_ip = DEFAULT_USE_STATIC_IP,               \
                                                                              \
          .s.sfConfig.slotPeriod = DEFAULT_SLOT_PERIOD_MS,                    \
          .s.sfConfig.numSlots = DEFAULT_NUM_SLOTS,                           \
          .s.sfConfig.sfPeriod_ms = DEFAULT_SF_PERIOD_MS,                     \
          .s.sfConfig.tag_replyDly_us = DEFAULT_TAG_POLL_TX_RESPONSE_RX_US,   \
          .s.sfConfig.tag_pollTxFinalTx_us = DEFAULT_TAG_POLL_TX_FINAL_TX_US, \
                                                                              \
          .s.txConfig.PGdly = 0x34,                                           \
          .s.txConfig.power = 0xfdfdfdfdUL,                                   \
          .s.txConfig.PGcount = 0,                                            \
                                                                              \
          .s.addr = DEFAULT_NODE_ADDR,                                        \
          .s.panID = DEFAULT_PANID,                                           \
          .s.uartEn = DEFAULT_UART,                                           \
          .s.pdoaOffset_deg = DEFAULT_PDOAOFF,                                \
          .s.rngOffset_mm = DEFAULT_RNGOFF,                                   \
          .s.pdoa_temp_coeff_mrad = DEFAULT_PDOA_TEMP,                        \
          .s.accEn = DEFAULT_ACCUM_READING,                                   \
          .s.diagEn = DEFAULT_DIAG_READING,                                   \
          .s.phaseCorrEn = DEFAULT_PHASECORR_EN,                              \
          .s.reportLevel = DEFAULT_REPORT_LEVEL,                              \
          .s.faultyRanges = 5,                                                \
          .s.debugEn = DEFAULT_DEBUG,                                         \
          .s.rcDelay_us = DEFAULT_TAG_BLINK_TX_RC_RX_US,                      \
          .s.rcRxTo_us = DEFAULT_TAG_RC_RX_TIMEOUT_US,                        \
                                                                              \
          .s.antRx_a = (uint16_t)(0.5 * DEFAULT_ANTD),                        \
          .s.antTx_a = (uint16_t)(0.5 * DEFAULT_ANTD),                        \
          .s.antRx_b = (uint16_t)(0.5 * DEFAULT_ANTD),                        \
          .s.fixed_pos_x_mm = (uint16_t)(0),                                  \
          .s.fixed_pos_y_mm = (uint16_t)(0),                                  \
          .s.fixed_pos_z_mm = (uint16_t)(1900),                               \
                                                                              \
          .s.default_event = 0L,                                              \
          .s.lotId = 0UL,                                                     \
          .s.partId = 0UL,                                                    \
                                                                              \
          .s.stsKey.key0 = 0x14EB220FUL,                                      \
          .s.stsKey.key1 = 0xF86050A8UL,                                      \
          .s.stsKey.key2 = 0xD1D336AAUL,                                      \
          .s.stsKey.key3 = 0x14148674UL,                                      \
          .s.stsIv.iv0 = 0x1F9A3DE4UL,                                        \
          .s.stsIv.iv1 = 0xD37EC3CAUL,                                        \
          .s.stsIv.iv2 = 0xC44FA8FBUL,                                        \
          .s.stsIv.iv3 = 0x362EEB34UL,                                        \
          .s.stsStatic = DEFAULT_STS_STATIC,                                  \
          .s.xtalTrim = (DEFAULT_XTAL_TRIM),                                  \
          .signature = DEFAULT_CONFIG_SIGNATURE                               \
        }

struct sfConfig_s
{
  uint16_t    slotPeriod;                /**< Slot period (time for one tag to
                                          *   range) */
  uint16_t    numSlots;                  /**< Number of slots used in the
                                          *   SuperFrame */
  uint16_t    sfPeriod_ms;               /**< SuperFrame period in ms */
  uint16_t    tag_replyDly_us;           /**< wait4response delay after end of
                                          *   tag's Poll transmission in us */
  uint16_t    tag_pollTxFinalTx_us;      /**< PollTxToFinalTx period, us (i.e.
                                          *   period from Poll's RMARKER to
                                          *   Final's RMARKER)*/
};

typedef struct sfConfig_s sfConfig_t;

/* holds a run-time parameters */
struct run_s
{
  sfConfig_t  sfConfig;         /**< System configuration: SuperFrame
                                 *   description */
  dwt_txconfig_t  txConfig;
  uint16_t    addr;             /**< Node's address */
  uint16_t    panID;            /**< System PanID */
  uint8_t     uartEn;           /**< Use UART to output data simultaneously to
                                 *   USB */
  int16_t     pdoaOffset_deg;   /**< Calibration: the Phase Differences offset
                                   */
  int16_t     rngOffset_mm;     /**< Calibration: the Ranging offset */
  int16_t     pdoa_temp_coeff_mrad;   /**< TODO: reserved for future */
  uint8_t     accEn;            /**< Enable CIR reading & reporting */
  uint8_t     diagEn;           /**< Enable Diagnostics reading & reporting */
  uint8_t     phaseCorrEn;      /**< Use correction curve to use in calculation
                                 *   PDOA->X-Y */
  uint8_t     reportLevel;      /**< 0 - no output, 1-JSON, 2-limited */
  uint8_t     faultyRanges;     /**< Tag config: number of faulty ranges after
                                 *   that Tag return back to a Discovery phase
                                   */
  uint8_t     debugEn;          /**< Enable Red "error" Led and error_handler()
                                   */
  uint32_t    rcDelay_us;       /**< Node&Tag delay between end reception of UWB
                                 *   blink and start transmission of UWB Ranging
                                 *   Config message */
  uint16_t    rcRxTo_us;        /**< Tag's Receiver timeout only to save power
                                 *   on non-reception of the Ranging Config */

  uint16_t    antRx_a;          /**< antenna delay values for the left port */
  uint16_t    antTx_a;          /**< antenna delay values for the left port */
  uint16_t    antRx_b;          /**< antenna delay values for the right port  */
  uint16_t    fixed_pos_x_mm;   /**< max 65_000 mm */
  uint16_t    fixed_pos_y_mm;   /**< max 65_000 mm */
  uint16_t    fixed_pos_z_mm;   /**< max 65_000 mm */

  int         default_event;    /**< this even will be generated after the
                                 *   Reset: default is set in main(), also see
                                 *   the "save" command */

  uint32_t    lotId;            // workaround for ES0 chip variant:: ES0 has no
                                //   lotId
  uint32_t    partId;           // workaround for ES0 chip variant:: ES0 has no
                                //   partId

  dwt_sts_cp_key_t    stsKey;           /**< AES Key to be used to set the STS
                                           */
  dwt_sts_cp_iv_t     stsIv;            /**< AES IV to be used to set the
                                         *   initial IV */
  int                 stsStatic;   /**< Configuration to STS/IV : value 0 =
                                    *   dynamic STS, 1 = fixed STS*/
  uint8_t             xtalTrim;
} __attribute__((__packed__));

typedef struct run_s run_t;

struct ver_num_s
{
  uint32_t ver0;
  uint32_t ver1;
  uint32_t ver2;
  uint32_t ver3;
} __attribute__((__packed__));

typedef struct ver_num_s ver_num_t;

struct static_ip_s
{
  uint8_t addr1;
  uint8_t addr2;
  uint8_t addr3;
  uint8_t addr4;

  uint8_t gw_addr1;
  uint8_t gw_addr2;
  uint8_t gw_addr3;
  uint8_t gw_addr4;

  uint8_t nm_addr1;
  uint8_t nm_addr2;
  uint8_t nm_addr3;
  uint8_t nm_addr4;

  uint8_t use_static_ip;
} __attribute__((__packed__));

typedef struct static_ip_s static_ip_t;

/* The structure, holding the changeable configuration of the application
 * */
struct param_block_s
{
  dwt_config_t    dwt_config;       /**< Standard Decawave driver config */
  tag_addr_slot_t knownTagList[MAX_KNOWN_TAG_LIST_SIZE];
  ver_num_t       v;                /**< App version */
  static_ip_t     static_config;    /**< Static IP config */
  run_t           s;                /**< Run-time parameters */
  uint32_t        signature;
  uint8_t         free[FCONFIG_SIZE
                       - sizeof(dwt_config_t)
                       - (sizeof(tag_addr_slot_t) * MAX_KNOWN_TAG_LIST_SIZE)
                       - sizeof(ver_num_t) - sizeof(static_ip_t)
                       - sizeof(run_t)
                       - sizeof(uint32_t)];
} __attribute__((__packed__));

typedef struct param_block_s param_block_t;

#ifdef __cplusplus
}
#endif

#endif /* __DEFAULT_CONFIG__H__ */
