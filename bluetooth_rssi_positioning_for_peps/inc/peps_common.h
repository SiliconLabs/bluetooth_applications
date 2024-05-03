/***************************************************************************//**
 * @file peps_common.h
 * @brief PEPS-related LIN bus message definitions
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: LicenseRef-MSLA
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement
 * By installing, copying or otherwise using this software, you agree to the
 * terms of the MSLA.
 *
 ******************************************************************************/

#ifndef PEPS_COMMON__H
#define PEPS_COMMON__H

#include "sl_bt_api.h"

#include <stdint.h>

#pragma pack(push, 1)

#ifndef MONACT_PORT
#define MONACT_PORT                              gpioPortA
#endif
#ifndef MONACT_PIN
#define MONACT_PIN                               7
#endif

#define PEPS_ADDR_BROADCAST_CONNPARAMS_1         0x01
typedef struct peps_cmd_broadcast_connparams_1 {
  uint8_t sequence_no;                        // 1 byte
  uint32_t access_address;                    // 4 bytes
  uint8_t central_phy;                        // 1 byte
  uint8_t peripheral_phy;                     // 1 byte
  uint8_t channel_selection_algorithm;        // 1 byte
} peps_cmd_broadcast_connparams_1_t;

#define PEPS_ADDR_BROADCAST_CONNPARAMS_2    0x02
typedef struct peps_cmd_broadcast_connparams_2 {
  uint8_t sequence_no;                        // 1 byte
  uint32_t crc_init;                          // 4 byte
  uint8_t role;                               // 1 byte
} peps_cmd_broadcast_connparams_2_t;

#define PEPS_ADDR_BROADCAST_CONNPARAMS_3    0x03
typedef struct peps_cmd_broadcast_connparams_3 {
  uint8_t sequence_no;                        // 1 byte
  sl_bt_connection_channel_map_t channel_map; // 5 bytes
  uint8_t channel;                            // 1 byte
  uint8_t hop;                                // 1 byte
} peps_cmd_broadcast_connparams_3_t;

#define PEPS_ADDR_BROADCAST_CONNPARAMS_4    0x04
typedef struct peps_cmd_broadcast_connparams_4 {
  uint8_t sequence_no;                        // 1 byte
  uint16_t interval;                          // 2 bytes
  uint16_t supervision_timeout;               // 2 bytes
  uint16_t event_counter;                     // 2 bytes
  uint8_t central_clock_accuracy;             // 1 byte
} peps_cmd_broadcast_connparams_4_t;

#define PEPS_ADDR_BROADCAST_CONNPARAMS_5    0x05
typedef struct peps_cmd_broadcast_connparams_5 {
  uint8_t sequence_no;                        // 1 byte
  uint32_t start_time_us;                     // 4 byte
} peps_cmd_broadcast_connparams_5_t;

#define PEPS_ADDR_BROADCAST_DISCONNECT      0x06
typedef struct peps_cmd_broadcast_disconnect {
  uint8_t dummy;
} peps_cmd_broadcast_disconnect_t;

#define PEPS_ADDR_BROADCAST_LATCH_RSSI      0x07
typedef struct peps_cmd_broadcast_latch_rssi {
  uint8_t dummy;
} peps_cmd_broadcast_latch_rssi_t;

#define PEPS_OFS_READ_VAR1                  0x00
#define PEPS_ADDR_READ_VAR1(base)       ((base) + PEPS_OFS_READ_VAR1)
typedef struct peps_cmd_read_var1 {
  unsigned char data[8];
} peps_cmd_read_var1_t;

#define PEPS_OFS_WRITE_VAR2                 0x01
#define PEPS_ADDR_WRITE_VAR2(base)      ((base) + PEPS_OFS_WRITE_VAR2)
typedef struct peps_cmd_write_var2 {
  unsigned char data[8];
} peps_cmd_write_var2_t;

#define PEPS_OFS_READ_VAR2                  0x02
#define PEPS_ADDR_READ_VAR2(base)       ((base) + PEPS_OFS_READ_VAR2)
typedef struct peps_cmd_read_var2 {
  unsigned char data[8];
} peps_cmd_read_var2_t;

#define PEPS_OFS_READ_RSSI                  0x03
#define PEPS_ADDR_READ_RSSI(base)       ((base) + PEPS_OFS_READ_RSSI)
typedef struct peps_cmd_read_rssi {
  uint8_t status;
  int8_t central_rssi;
  int8_t peripheral_rssi;
} peps_cmd_read_rssi_t;

#define PEPS_OFS_WRITE_RESULT           0x04
#define PEPS_ADDR_WRITE_RESULT(base)    ((base) + PEPS_OFS_WRITE_RESULT)
typedef struct peps_cmd_write_result {
  float distance;
  float angle;
} peps_cmd_write_result_t;

#define PEPS_OFS_READ_CONNPARAMS_STATE  0x05
#define PEPS_ADDR_READ_CONNPARAMS_STATE(base)    ((base) \
                                                  +      \
                                                  PEPS_OFS_READ_CONNPARAMS_STATE)
#define PEPS_ACK_ALL_CONNPARAMS         ((1U << 5) - 1)
typedef struct peps_cmd_read_connparams_state {
  uint32_t ack_bits;                          // 4 bytes
  uint8_t sequence_no;                        // 1 byte
} peps_cmd_read_connparams_state_t;

#pragma pack(pop)

#endif
