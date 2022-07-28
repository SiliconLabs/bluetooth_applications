/***************************************************************************//**
 * @file sntp.h
 * @brief Wiznet Ethernet SNTP Client.
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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
 *******************************************************************************
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

#ifndef SNTP_CLIENT_H_
#define SNTP_CLIENT_H_

#include "ethernet.h"
#include "ethernet_udp.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup SNTP SNTP Client
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup SNTP
 * @brief  Ethernet SNTP client.
 * @details
 * @{
 ******************************************************************************/

/// SNTP object definition
typedef struct
{
  w5x00_ethernet_t *eth;            ///< Ethernet object reference
  w5x00_ethernet_udp_t udp_socket;  ///< UDP socket
  uint32_t epoch_timestamp;         ///< Epoch timestamp
} w5x00_sntp_t;

/***************************************************************************//**
 * @brief
 *    Get unix timestamp. Unix time starts on Jan 1 1970.
 *    In seconds, that's 2208988800:
 * @param[in] sntp
 *    SNTP instance
 * @return
 *    Unix timestamp
 ******************************************************************************/
#define w5x00_sntp_get_unix_timestamp(sntp) \
      (sntp)->epoch_timestamp - 2208988800UL

/***************************************************************************//**
 * @brief
 *    Init SNTP instance
 * @param[in] sntp
 *    SNTP instance
 * @param[in] eth
 *    Ethernet instance
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_sntp_init(w5x00_sntp_t *sntp, w5x00_ethernet_t *eth);

/***************************************************************************//**
 * @brief
 *    Update timestamp from NTP server IP
 * @param[in] sntp
 *    SNTP instance
 * @param[in] server_ip
 *    NTP server IP
 * @param[in] port
 *    NTP server Port. Default value is 123
 * @param[in] timeout
 *    Connection timeout
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_sntp_update_timestamp(w5x00_sntp_t *sntp,
                                        w5x00_ip4_addr_t server_ip,
                                        uint16_t port,
                                        uint16_t timeout);

/***************************************************************************//**
 * @brief
 *    Update timestamp from NTP server host
 * @param[in] sntp
 *    SNTP instance
 * @param[in] host
 *    NTP server host name
 * @param[in] port
 *    NTP server Port. Default value is 123
 * @param[in] timeout
 *    Connection timeout
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_sntp_update_timestamp_from_host(w5x00_sntp_t *sntp,
                                                  const char *host,
                                                  uint16_t port,
                                                  uint16_t timeout);
/** @} (end group SNTP) */
#ifdef __cplusplus
}
#endif
#endif  // SNTP_CLIENT_H_
