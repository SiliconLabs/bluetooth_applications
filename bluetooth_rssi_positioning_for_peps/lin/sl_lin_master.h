/***************************************************************************//**
 * @file sl_lin_master.h
 * @brief Master-side LIN bus driver
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef SL_LIN_MASTER__H
#define SL_LIN_MASTER__H

#include "sl_lin_common.h"

#include <sl_status.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief      Timeout for the communication on the bus
 *
 * The master is expected to communicate on the bus periodically
 * (in time-division multiplexing), with a constant implementation-defined
 * interval. The specs mention 5 msecs or 10 msecs for an example.
 * The longest supported packet has 124 bits, which translates to 6.2 msecs at
 * a data rate of 20 kbits/second. The spec allows a maximum of 40% of extra
 * transmission time, increasing the width of the window to 8.68 msecs.
 * To be on the safe side, specify a timeout of 9.1 msecs, which gives:
 * 0.42 msecs safety margin after the transmission (8.4 bits,
 * 16128 HF ticks at 38.4 MHz)
 * and 0.9 msecs safety margin before the next one (18 bits,
 * 34560 HF ticks at 38.4 MHz)
 * 9.1 msecs roughly translates to 300 32.768 kHz ticks
 */
#define SL_LIN_TIMEOUT      300

/**
 * @brief      Counters for detected checksum errors
 */
extern volatile uint32_t sl_lin_counter_master_checksum;
extern volatile uint32_t sl_lin_counter_slave1_checksum;
extern volatile uint32_t sl_lin_counter_slave2_checksum;

/**
 * @brief Counters for detected conflicts (transmitting 1 while the bus is 0)
 */
extern volatile uint32_t sl_lin_counter_master_conflict;
extern volatile uint32_t sl_lin_counter_slave1_conflict;
extern volatile uint32_t sl_lin_counter_slave2_conflict;

/**
 * @brief      Counters for detected generic errors (framing)
 */
extern volatile uint32_t sl_lin_counter_master_generic;
extern volatile uint32_t sl_lin_counter_slave1_generic;
extern volatile uint32_t sl_lin_counter_slave2_generic;

/**
 * @brief      Counters for detected timeouts
 */
extern volatile uint32_t sl_lin_counter_master_timeout;

/**
 * @brief      Initialize the peripherals for LIN bus communication
 *
 * @param[in]  baud               The baud rate to configure
 *
 * @note       Shall be the first API function to be called
 */
void sl_lin_master_init(int baud);

/**
 * @brief      Initiate a master->slave transfer
 *
 * @param[in]  frame_id           ID to write to
 * @param[in]  data               Pointer to bytes to write
 * @param[in]  len                Number of bytes to write
 * @param[in]  enhanced_checksum  Shall a LIN 2.x checksum including the
 *                                frame_id be calculated?
 * @param[in]  inject_checksum_error  Shall a checksum error be forced?
 *                                    (for testing purposes)
 * @param[in]  limiter            Communicate in 10ms window if enabled
 *
 * @returns    Status code indicating success of the function call
 *             SL_STATUS_INVALID_PARAMETER  data or len is 0
 *             SL_STATUS_INVALID_RANGE      frame_id is invalid
 *             SL_STATUS_ABORT              conflict detected
 *             SL_STATUS_OK                 everything went well
 *
 * @note       The ID is expected to be a 6-bit unprotected ID in the 0..59
 *             range. Diagnostics and reserved messages are not supported.
 *             The limiter makes sure that no communication starts at least
 *             a window time after the previous communication has started.
 *             It could be enabled to skip the use of a complex communication
 *             manager.
 */
sl_status_t sl_lin_master_transmit(uint8_t frame_id,
                                   const uint8_t *data,
                                   int len,
                                   bool enhanced_checksum,
                                   bool inject_checksum_error,
                                   bool limiter);

/**
 * @brief      Initiate a slave->master transfer
 *
 * @param[in]  frame_id           ID to read from
 * @param[in]  data               Pointer to buffer
 * @param[in]  len                Number of bytes to read
 * @param[in]  enhanced_checksum  Shall a LIN 2.x checksum including the
 *                                frame_id be calculated?
 * @param[in]  limiter            Communicate in 10ms window if enabled
 *                                frame_id be calculated?
 *
 * @returns    Status code indicating success of the function call
 *             SL_STATUS_INVALID_PARAMETER  data or len is 0
 *             SL_STATUS_INVALID_RANGE      frame_id is invalid
 *             SL_STATUS_ABORT              conflict detected
 *             SL_STATUS_IO                 frame/checksum error or
 *                                          RX fifo has overflowed loosing bytes
 *             SL_STATUS_TIMEOUT            no answer arrived in time
 *             SL_STATUS_OK                 everything went well
 *
 * @note       The ID is expected to be a 6-bit unprotected ID in the 0..59
 *             range. Diagnostics and reserved messages are not supported.
 *             The limiter makes sure that no communication starts at least
 *             a window time after the previous communication has started.
 *             It could be enabled to skip the use of a complex communication
 *             manager.
 */
sl_status_t sl_lin_master_request(uint8_t frame_id,
                                  uint8_t *data,
                                  int len,
                                  bool enhanced_checksum,
                                  bool limiter);

/**
 * @brief      Send a "bus sleep" message
 *
 * @param[in]  force              Send the message even if the bus might be
 *                                already sleeping
 *                                (with a side effect of waking it up before)
 *
 * @returns    Check \ref sl_lin_master_transmit().
 *
 * @note       The slave implementation does not require this and just ignores.
 *             The slave devices just go to sleep mode as soon as possible.
 */
sl_status_t sl_lin_master_bus_sleep(bool force);

/**
 * @brief      Generate a wakeup signal on the bus as per 2.6.2 (of 2.2A spec)
 *
 * @note       The slave implementation does not require this and just ignores.
 *             Virtually every BREAK signal is a wakeup.
 */
void sl_lin_master_wakeup_bus(void);

/**
 * @brief      Clean up the counters
 */
void sl_lin_master_reset_counters(void);

#endif
