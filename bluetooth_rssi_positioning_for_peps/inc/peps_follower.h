/***************************************************************************//**
 * @file peps_follower.h
 * @brief PEPS Follower functionality
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

#ifndef PEPS_FOLLOWER__H
#define PEPS_FOLLOWER__H

#include "peps_common.h"

#include "sl_status.h"

sl_status_t peps_follower_init(void);
sl_status_t peps_follower_update_rssi(int8_t central_rssi,
                                      int8_t peripheral_rssi);
void peps_follower_sniffer_stopped(void);
void peps_follower_tick(void);

#endif
