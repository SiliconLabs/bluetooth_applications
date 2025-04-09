/***************************************************************************//**
 * @file peps_leader.h
 * @brief PEPS Leader functionality
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef PEPS_LEADER__H
#define PEPS_LEADER__H

#include "peps_common.h"

#include "sl_status.h"

#include <stdbool.h>

sl_status_t peps_leader_init(void);
sl_status_t peps_leader_broadcast_connparams_1(
  const peps_cmd_broadcast_connparams_1_t *param);
sl_status_t peps_leader_broadcast_connparams_2(
  const peps_cmd_broadcast_connparams_2_t *param);
sl_status_t peps_leader_broadcast_connparams_3(
  const peps_cmd_broadcast_connparams_3_t *param);
sl_status_t peps_leader_broadcast_connparams_4(
  const peps_cmd_broadcast_connparams_4_t *param);
sl_status_t peps_leader_broadcast_connparams_5(
  const peps_cmd_broadcast_connparams_5_t *param);
sl_status_t peps_leader_read_var2(unsigned char base,
                                  peps_cmd_read_var2_t *param);
sl_status_t peps_leader_write_var2(unsigned char base,
                                   const peps_cmd_write_var2_t *param);
sl_status_t peps_leader_read_rssi(unsigned char base,
                                  int8_t *central_rssi,
                                  int8_t *peripheral_rssi);
sl_status_t peps_leader_write_result(unsigned char base,
                                     float distance,
                                     float angle);
sl_status_t peps_leader_forward_connection_details(uint8_t connection);
sl_status_t peps_leader_read_connparams_state(unsigned char base,
                                              peps_cmd_read_connparams_state_t *param);
sl_status_t peps_leader_broadcast_disconnect(void);
sl_status_t peps_leader_broadcast_latch_rssi(void);

bool peps_leader_fetch_rssi_values(void);
sl_status_t peps_leader_update_location(void);

#endif
