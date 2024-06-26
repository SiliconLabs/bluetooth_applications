/**
 * @file      rtls_interface.h
 *
 * @brief     Decawave RTLS CLE / RTLS anchor common definitions / APIs
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef RTLS_INTERFACE_H_
#define RTLS_INTERFACE_H_   1

#ifdef __cplusplus
extern "C" {
#endif

#include "uwb_frames.h"

// Protocol structs and enums :
enum RTLS_CMD {
  RTLS_CMD_PARM_STOP = 0, RTLS_CMD_PARM_START = 1,

  RTLS_CMD_REQ_CFG = 0x42,          // Anchor: request configuration
  RTLS_CMD_SET_CFG_CCP = 0x44,      // CLE: response on RTLS_CMD_REQ_CFG request

  // Anchor -> LE
  RTLS_CMD_DEBUG = 0x37,                // Anchor: Debug Message

  RTLS_CMD_REPORT_TOA = 0x3A,           // v.3 TDOA
  RTLS_CMD_REPORT_TOA_EX = 0x3B,    // v.3 TDOA with extra data (e.g. IMU data)
  RTLS_CMD_REPORT_TX_CS = 0x3E,         // v.3 CCP Tx report
  RTLS_CMD_REPORT_RX_CS = 0x3F,         // v.3 CCP Rx report

  RTLS_CMD_REPORT_TX_CS_V4 = 0x30,          // v.4 CCP Tx report
  RTLS_CMD_REPORT_RX_CS_V4 = 0x31,          // v.4 CCP Rx report
  RTLS_CMD_REPORT_TOA_V4 = 0x32,            // v.4 TDOA with FP
  RTLS_CMD_REPORT_TOA_IMU_V4 = 0x33,        // v.4 TDOA with FP and IMU data

  // LE -> Anchor
  RTLS_COMM_TEST_START_REQ = 0x53,
  RTLS_COMM_TEST_RESULT_REQ = 0x54,
  RTLS_RANGE_MEAS_REQ = 0x55,
  RTLS_INIT_REQ = 0x56,
  RTLS_START_REQ = 0x57,
  RTLS_POWER_TEST_START_REQ = 0x58,
  RTLS_RESET_REQ = 0x59,
  RTLS_SINGLE_TWR_MODE_REQ = 0x5A,
  RTLS_ASYMM_TWR_MODE_REQ = 0x5B,

  RTLS_CFG_IND = 0x80,
  RTLS_COMM_TEST_DONE_IND = 0x81,
  RTLS_COMM_TEST_RESULT_IND = 0x82,
  RTLS_RANGE_MEAS_IND = 0x83,
  RTLS_RANGE_MEAS_IND_FINAL = 0x84,
  RTLS_POWER_TEST_DONE_IND = 0x85,

  RTLS_TEMP_VBAT_IND = 0x87,                    // Temperature and VBAT
  RTLS_LOG_ACCUMULATOR_REQ = 0x88,       // Request Accumulator for CCP or Blink
  RTLS_LOG_ACCUMULATOR_IND = 0x91         // Accumulator Report for CCP or Blink
};

#define RTLS_CMD_SET_CFG_CCP_LEN                        sizeof(cmd_config_t)
#define RTLS_POWER_TEST_START_LEN                       sizeof(cmd_power_test_t)
#define RTLS_COMM_TEST_START_LEN                        sizeof(cmd_comm_test_t)
#define RTLS_COMM_TEST_RESULT_REQ_LEN                   (1)
#define RTLS_RANGE_MEAS_REQ_LEN                         (21)
#define RTLS_SINGLE_TWR_MODE_REQ_LEN                    (17)
#define RTLS_ASYMM_TWR_MODE_REQ_LEN                     (27)
#define RTLS_START_REQ_LEN                              (2)
#define RTLS_LOG_ACC_REQ_LEN                            (4)
#define RTLS_RESET_REQ_LEN                              (18)

#define RTLS_CMD_REPORT_TOA_LEN                         (16)
#define RTLS_CMD_REPORT_TOA_EX_LEN                      (17) // minimum length
                                                             //   of longer TDoA
                                                             //   report, extra
                                                             //   byte,
                                                             //   specifying
                                                             //   extra data
                                                             //   length
#define RTLS_CMD_REPORT_TX_CS_LEN                     (8)
#define RTLS_CMD_REPORT_RX_CS_LEN                     (16)

#define RTLS_CMD_REPORT_TOA_V4_LEN                    (18)
#define RTLS_CMD_REPORT_TX_CS_V4_LEN                  (8)
#define RTLS_CMD_REPORT_RX_CS_V4_LEN                  (18)

#define RTLS_CMD_DEBUG_LEN                            (3)

#define RTLS_POWER_TEST_DONE_IND_LEN                  (1)
#define RTLS_COMM_TEST_DONE_IND_LEN                   (1)
#define RTLS_COMM_TEST_RESULT_IND_LEN                 (5)
#define RTLS_RANGE_MEAS_IND_LEN                       (14)
#define RTLS_TEMP_VBAT_IND_LEN                        (12)

#define RTLS_ACC_LOG_HEADER_LEN                       (15) // 1 byte (MSG ID)
                                                           //   + 1 byte frame
                                                           //   type (CCP or
                                                           //   Blink) + 1
                                                           //   byte seq Num +
                                                           //   4 bytes log
                                                           //   Num + 8 bytes
                                                           //   of source
                                                           //   address (CCP
                                                           //   Master or
                                                           //   Blink Tag)
#define RTLS_ACC_LOG_REG_LEN                          (16) // fpIndex,
                                                           //   maxNoise,
                                                           //   firstPathAmp1,
                                                           //   stdNoise,
                                                           //   firstPathAmp2,
                                                           //   firstPathAmp3,
                                                           //   maxGrowthCIR,
                                                           //   rxPreamCount

#define RTLS_DW_ACCUMULATOR_LEN_16 \
  (992 * 4 + 1)  // 16M PRF is 992*4+1
#define RTLS_DW_ACCUMULATOR_LEN_64 \
  (1016 * 4 + 1) // 64M PRF is 1016*4+1

#define RTLS_LOG_ACCUMULATOR_IND_LEN_16 \
  (RTLS_ACC_LOG_HEADER_LEN              \
   + RTLS_ACC_LOG_REG_LEN               \
   + RTLS_DW_ACCUMULATOR_LEN_16)
#define RTLS_LOG_ACCUMULATOR_IND_LEN_64 \
  (RTLS_ACC_LOG_HEADER_LEN              \
   + RTLS_ACC_LOG_REG_LEN               \
   + RTLS_DW_ACCUMULATOR_LEN_64)
#define RTLS_LOG_ACCUMULATOR_IND_        \
  ((RTLS_LOG_ACCUMULATOR_IND_LEN_16      \
    < RTLS_LOG_ACCUMULATOR_IND_LEN_64) ? \
   RTLS_LOG_ACCUMULATOR_IND_LEN_16       \
   :                                     \
   RTLS_LOG_ACCUMULATOR_IND_LEN_64) // minimumn of 2

#define ACCUM_TYPE_BLINK                                0xA1 // accumulator
                                                             //   reading is
                                                             //   only used to
                                                             //   read blinks
                                                             //   and CCPs
#define ACCUM_TYPE_CCP                                  0xA2 // accumulator
                                                             //   reading is
                                                             //   only used to
                                                             //   read blinks
                                                             //   and CCPs

#define DWT_DIAGNOSTIC_LOG_REV_5                        (5)
#define DWT_DIAGNOSTIC_LOG_V_5                          (5)
#define DWT_SIZEOFDIAGNOSTICDATA_5                      (66)
#define DWT_SIZE_OF_IMUDATA                             (30)

#define DWT_LOG_NUM_SIZE                                (4)

/* The data is framed as follows :
 *<STX><LENlsb><LENmsb><DATA : <FC><XX>.....><CRClsb><CRCmsb><ETX>
 * STX = 0x2
 * LEN is the length of data message(16 bits)
 * CRC is the 16 - bit CRC of the data bytes
 * ETX = 0x3
 * FC = is the function code(API code)
 *
 */
#define FRAME_HEADER_LEN                                (6)
#define FRAME_START_IDX                                 (0)
#define FRAME_LENGTH_IDX                                (1)
#define FRAME_DATA_IDX                                  (3)

enum RTLS_DATA {
  RTLS_DATA_ANCHOR_REQ = 0x41,

  RTLS_DATA_ANCHOR = 0x61,
  RTLS_DATA_BLINK = 0x62,
  RTLS_DATA_STATS = 0x63,
  RTLS_DATA_IMU = 0x64,
  RTLS_DATA_BLINK_EXT = 0x65
};

/* Network commands
 * use byte access if in doubt
 */

#define UN16(x) union { uint16_t x ## 16; uint8_t  x[2];}

#define UN32(x) union { uint32_t x ## 32; uint8_t  x[4];}

/* Wire format of messages from Anchor to CLE */
#pragma pack (push, 1)

/* Standard Diagnostics v5 */
struct diag_v5_s {
  // NOTE: diagnostics data format rev 5 (DWT_DIAGNOSTIC_LOG_REV_5)
  uint8_t        header;    // 00 this could be a header (format version number)
  uint8_t        r0F[5];    // 01 register 0xF  - length 5 bytes
  uint8_t        r10[4];    // 06 register 0x10 - length 4 bytes
  uint8_t        r12[8];    // 10 register 0x12 - length 8 bytes
  uint8_t        r13[4];    // 18 register 0x13 - length 4 bytes
  uint8_t        r14[5];    // 22 register 0x14 - length 5 bytes
  uint8_t        r15[14];   // 27 register 0x15 - length 14 bytes (5 TS, 2 FP, 2
                            //   Diag, 5 TSraw)
  uint8_t        r25[16];   // 41 register 0x25 @FP (first path) -> 16 bytes
                            //   starting at FP + 1 dummy
  uint8_t        r2E[2];    // 58 register 0x2E (0x1000) - 2 bytes
  uint8_t        r27[4];    // 60 register 0x27 (0x28)   - 4 bytes
  uint8_t        r2E2[2];   // 64 register 0x2E (0x1002) - 2 bytes
  uint8_t        dummy;
  // 66 total
};

typedef struct diag_v5_s diag_v5_t;

struct ccp_rx_v4_s {
  uint8_t type;                         // 0     : type = CCP RX timestamp
  uint8_t seqNum;                       // 1     : CCP Seq Num
  uint8_t masterID[ADDR_BYTE_SIZE_L];   // 2-9   : master anchor ID (that sent
                                        //   the CCP frame)
  uint8_t csRxTime[TS_40B_SIZE];        // 10-14 : CCP Rx time
  UN16(firstPath);                      // 15-16 : raw Firstpath
  uint8_t extLen;                       // 17    : length of ext below
};

typedef struct ccp_rx_v4_s ccp_rx_v4_t;

struct toa_v4_s {
  uint8_t type;                     // 0     : type = TOA report
  uint8_t seqNum;                   // 1     : Blink message sequence number
  uint8_t tagID[ADDR_BYTE_SIZE_L];  // 2-9   : tag ID
  uint8_t rawTOA[TS_40B_SIZE];      // 10-14 : raw timestamp
  UN16(firstPath);                  // 15-16 : raw Firstpath
  uint8_t extLen;                   // 17    : length of ext below
};

typedef struct toa_v4_s toa_v4_t;

/* RTLS_CMD_REPORT_TOA_IMU_V4 */
struct report_toa_imu_v4_s {
  toa_v4_t    toa;
  union {
    uint8_t ext[DWT_SIZE_OF_IMUDATA + sizeof(uint32_t) + sizeof(diag_v5_t)];
    struct {
      uint8_t dataIMU[DWT_SIZE_OF_IMUDATA];      // : IMU data sent from the
                                                 //   tag(2 (ench + exth) + 1
                                                 //   (dwh)+27 (dwp))
      UN32(logNum);
      diag_v5_t diag;
    };
  };
};

typedef struct report_toa_imu_v4_s report_toa_imu_v4_t;

/* RTLS_CMD_REPORT_TOA_V4 */
struct report_toa_v4_s {
  toa_v4_t    toa;
  union {
    uint8_t ext[sizeof(uint32_t) + sizeof(diag_v5_t)];
    struct {
      UN32(logNum);
      diag_v5_t diag;
    };
  };
};

typedef struct report_toa_v4_s report_toa_v4_t;

/* RTLS_CMD_REPORT_RX_CS_V4 */
struct report_ccp_rx_v4_s {
  ccp_rx_v4_t ccp;

  union {
    uint8_t ext[sizeof(uint32_t) + sizeof(diag_v5_t)];
    struct {
      UN32(logNum);
      diag_v5_t diag;
    };
  };
};

typedef struct report_ccp_rx_v4_s report_ccp_rx_v4_t;

/* RTLS_CMD_REPORT_TX_CS_V4 */
struct report_ccp_tx_v4_s {
  uint8_t type;                         // 0         : type = CCP TX timestamp
  uint8_t seqNum;                       // 1         : CCP Seq Num
  uint8_t csTxTime[TS_40B_SIZE];        // 2-6       : CCP Tx time
  uint8_t extLen;                       // 7         : length of ext below
  union {
    uint8_t ext[sizeof(uint32_t)];
    UN32(logNum);
  };
};

typedef struct report_ccp_tx_v4_s report_ccp_tx_v4_t;

/* RTLS_TEMP_VBAT_IND */
struct report_temp_vbat_s {
  uint8_t type;                     // 0: type = Battery abnd Voltage Level
                                    //   report
  uint8_t bat;                      // 1             : battery voltage value
  uint8_t temp;                     // 2             : temperature value
  UN16(atemp);                      // 3-4           : ambient Sensor
                                    //   temperature value
  uint8_t apress[3];                // 5-7           : ambient Sensor pressure
                                    //   value
  UN32(logNum);                     // 8-11          : log number
};

typedef struct report_temp_vbat_s report_temp_vbat_t;

/* to be structured:
 *    RTLS_COMM_TEST_RESULT_REQ
 *    RTLS_START_REQ
 *    RTLS_LOG_ACCUMULATOR_REQ
 *    RTLS_RESET_REQ
 */

/* RTLS_CMD_SET_CFG_CCP */
struct cmd_config_s {
  uint8_t command;              // 0
  uint8_t id;                   // 1
  uint8_t master;               // 2
  uint8_t prf_ch;               // 3
  uint8_t datarate;             // 4
  uint8_t code;                 // 5
  uint8_t txPreambLength;       // 6
  uint8_t nsSFD_rxPAC;          // 7
  UN16(delay_rx);               // 8-9
  UN16(delay_tx);               // 10-11
  uint8_t free_12;              // 12
  uint8_t debug_logs;           // 13
  uint8_t eui64_to_follow[8];       // 14-21
  UN32(lag_delay);              // 22-25
};

typedef struct cmd_config_s cmd_config_t;

/* RTLS_SINGLE_TWR_MODE_REQ */
struct cmd_twr_single_s {
  uint8_t command;              // 0     : RTLS_SINGLE_TWR_MODE_REQ
  uint8_t role;                 // 1     : initiator==1 responder==0
  uint8_t use_ant_delay;        // 2
  UN16(delay_tx);               // 3-4
  UN16(delay_rx);               // 5-6
  UN16(response_delay);         // 7-8
  UN16(addr);                   // 9-10
  uint8_t lna_on;               // 11
  UN32(power);                  // 12-15
  uint8_t log_all;              // 16
};

typedef struct cmd_twr_single_s cmd_twr_single_t;

/* RTLS_RANGE_MEAS_REQ */
struct cmd_twr_s {
  uint8_t command;              // 0     : RTLS_RANGE_MEAS_REQ
  uint8_t log_all;              // 1
  uint8_t role;                 // 2
  UN16(num_ranges);             // 3-4
  uint8_t use_ant_delay;        // 5
  UN16(delay_tx);               // 6-7
  UN16(delay_rx);               // 8-9
  UN16(response_delay);         // 10-11
  UN16(initiator_addr);         // 12-13
  UN16(responder_addr);         // 14-15
  uint8_t lna_on;               // 16
  UN32(power);                  // 17-20
};

typedef struct cmd_twr_s cmd_twr_t;

/* RTLS_RANGE_MEAS_REQ */
struct cmd_twr_asymm_s {
  uint8_t command;              // 0     : RTLS_RANGE_MEAS_REQ
  uint8_t log_all;              // 1
  uint8_t role;                 // 2
  UN16(num_ranges);             // 3-4
  uint8_t use_ant_delay;        // 5
  UN16(delay_tx);               // 6-7
  UN16(delay_rx);               // 8-9
  UN16(response_delay);         // 10-11
  UN16(final_delay);            // 12-13
  UN16(report_delay);           // 14-15
  UN16(poll_period);            // 16-17
  UN16(initiator_addr);         // 18-19
  UN16(responder_addr);         // 20-21
  uint8_t lna_on;               // 22
  UN32(power);                  // 23-26
};

typedef struct cmd_twr_asymm_s cmd_twr_asymm_t;

/* RTLS_POWER_TEST_START_REQ */
struct cmd_power_test_s {
  uint8_t command;              // 0
  uint8_t start;                // 1
  UN16(duration);               // 2-3
};

typedef struct cmd_power_test_s cmd_power_test_t;

/* RTLS_COMM_TEST_START_REQ */
struct cmd_comm_test_s {
  uint8_t command;              // 0
  uint8_t transmit[1];          // 1
  UN16(data);                   // 2-3
};

typedef struct cmd_comm_test_s cmd_comm_test_t;

/* */
struct request_configCle_s
{
  uint8_t head;
  uint8_t uid[ADDR_BYTE_SIZE_L];
  char    ver[64 - (ADDR_BYTE_SIZE_L + 1)];   // hardcoded to be 64 bytes long
};

typedef struct request_configCle_s request_configCle_t;

#pragma pack(pop)

#undef UN16
#undef UN32

#ifdef __cplusplus
}
#endif
#endif //RTLS_INTERFACE_H
