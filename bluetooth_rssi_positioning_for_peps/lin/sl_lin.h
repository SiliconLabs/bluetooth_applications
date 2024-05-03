/***************************************************************************//**
 * @file sl_lin.h
 * @brief Slave-side LIN bus driver
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

#ifndef SL_LIN__H
#define SL_LIN__H

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
 * @brief      Sigature of the callback to be called on events
 *
 * @param[in]  frame_id           ID an event happened on
 * @param[in]  writable           Is this a write operation?
 * @param[in]  data               Data received/transmitted
 * @param[in]  len                Number of bytes in data
 * @param[in]  success            Was the operation successful?
 *
 * @note       The ID is a 6-bit unprotected ID in the 0..59 range.
 *             The same callback might be registered for multiple readable
 *             or writable endpoints. The data is only expected to be valid
 *             during the scope of the callback's invocation, so the data shall
 *             be copied elsewhere for later use if required, possibly a FIFO,
 *             which is processed from the main loop. Also the callback shall
 *             return as soon as possible to not disturb the communciation
 *             handling.
 */
typedef void (*sl_lin_callback_t)(uint8_t frame_id,
                                  bool writable,
                                  uint8_t *data,
                                  int len,
                                  bool success);

/**
 * @brief      Initialize the peripherals for LIN bus communication
 */
void sl_lin_slave_init(void);

/**
 * @brief      Register an ID for master->slave communication
 *
 * @param[in]  frame_id           ID to register
 * @param[in]  len                Number of bytes to expect
 * @param[in]  callback           Callback to call on a packet reception
 * @param[in]  enhanced_checksum  Shall a LIN 2.x checksum including the
 *                                frame_id be calculated?
 *
 * @returns    Status code indicating success of the function call
 *             SL_STATUS_INVALID_RANGE  frame_id or len is invalid
 *             SL_STATUS_ALREADY_EXISTS frame_id is already registered
 *             SL_STATUS_OK             everything went well
 *
 * @note       The ID is expected to be a 6-bit unprotected ID in the 0..59
 *             range. Diagnostics and reserved messages are not supported.
 *             The master might send a "bus sleep" message, which reuses a
 *             diagnostics message ID, and it's not supported (ignored).
 *             The device goes to sleep after processing the message as soon as
 *             possible anyway, so this should not cause a problem.
 *             It's safe to call this function anytime.
 */
sl_status_t sl_lin_slave_register_writable_endpoint(uint8_t frame_id,
                                                    int len,
                                                    sl_lin_callback_t callback,
                                                    bool enhanced_checksum);

/**
 * @brief      Register an ID for slave->master communication
 *
 * @param[in]  frame_id           ID to register
 * @param[in]  len                Number of bytes to transmit
 * @param[in]  callback           Callback to call on a packet transmission
 * @param[in]  initdata           Points to initial data to be set
 *                                NULL = all zeros
 * @param[in]  enhanced_checksum  Shall a LIN 2.x checksum including the
 *                                frame_id be calculated?
 *
 * @returns    Status code indicating success of the function call
 *             SL_STATUS_INVALID_RANGE  frame_id or len is invalid
 *             SL_STATUS_ALREADY_EXISTS frame_id is already registered
 *             SL_STATUS_OK             everything went well
 *
 * @note       The ID is expected to be a 6-bit unprotected ID in the 0..59
 *             range. Diagnostics and reserved messages are not supported.
 *             The data to be sent can be updated later by the
 *             \ref sl_lin_slave_update_readable_endpoint() call.
 *             It's safe to call this function anytime.
 */
sl_status_t sl_lin_slave_register_readable_endpoint(uint8_t frame_id,
                                                    int len,
                                                    sl_lin_callback_t callback,
                                                    const uint8_t *initdata,
                                                    bool enhanced_checksum);

/**
 * @brief      Unregister an ID
 *
 * @param[in]  frame_id           ID to unregister
 *
 * @returns    Status code indicating success of the function call
 *             SL_STATUS_INVALID_RANGE  frame_id is invalid
 *             SL_STATUS_EMPTY          frame_id was not registered
 *             SL_STATUS_OK             everything went well
 *
 * @note       The ID is expected to be a 6-bit unprotected ID in the 0..59
 *             range. Diagnostics and reserved messages are not supported.
 *             It's safe to call this function anytime.
 */
sl_status_t sl_lin_slave_unregister_endpoint(uint8_t frame_id);

/**
 * @brief      Update data for an ID registered for slave->master communication
 *
 * @param[in]  frame_id           ID to update
 * @param[in]  data               Points to the data to be set
 *
 * @returns    Status code indicating success of the function call
 *             SL_STATUS_INVALID_RANGE  frame_id is invalid
 *             SL_STATUS_NULL_POINTER   data is NULL
 *             SL_STATUS_INVALID_KEY    frame_id is not registered as readable
 *             SL_STATUS_OK             everything went well
 *
 * @note       The ID is expected to be a 6-bit unprotected ID in the 0..59
 *             range. Diagnostics and reserved messages are not supported.
 *             The data to be sent can be updated later by the
 *             \ref sl_lin_slave_update_readable_endpoint() call.
 *             Changing the length is only possible via unregistering and
 *             re-registering.
 *             It's safe to call this function anytime.
 */
sl_status_t sl_lin_slave_update_readable_endpoint(uint8_t frame_id,
                                                  const uint8_t *data);

/**
 * @brief      Damage a readable endpoint's checksum for testing purposes
 *
 * @param[in]  frame_id           ID of packet to damage the checksum
 *
 * @returns    Status code indicating success of the function call
 *             SL_STATUS_INVALID_RANGE  frame_id is invalid
 *             SL_STATUS_INVALID_KEY    frame_id is not registered as readable
 *             SL_STATUS_OK             everything went well
 *
 * @note       The ID is expected to be a 6-bit unprotected ID in the 0..59
 *             range. Diagnostics and reserved messages are not supported.
 */
sl_status_t sl_lin_slave_inject_checksum_error(uint8_t frame_id);

/**
 * @brief      Generate a wakeup signal on the bus as per 2.6.2 (of 2.2A spec)
 *
 * @note       The break field detection and retrying of the wakeup signal as
 *             shown in figure 2.17 is not implemented by the driver, and shall
 *             be implemented by the application, if required.
 */
void sl_lin_slave_wakeup_bus(void);

#endif
