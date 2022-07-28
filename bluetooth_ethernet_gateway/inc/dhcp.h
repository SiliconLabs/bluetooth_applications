/***************************************************************************//**
 * @file dhcp.h
 * @brief Wiznet Ethernet DHCP Function Prototypes.
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

#ifndef DHCP_H_
#define DHCP_H_

#include "ethernet_udp.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup DHCP DHCP Client
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup DHCP
 * @brief  Ethernet DHCP client function.
 * @details
 * @{
 ******************************************************************************/

enum W5x00_DHCP_CHECK {
  W5x00_DHCP_CHECK_NONE = 0,
  W5x00_DHCP_CHECK_RENEW_FAIL = 1,
  W5x00_DHCP_CHECK_RENEW_OK = 2,
  W5x00_DHCP_CHECK_REBIND_FAIL = 3,
  W5x00_DHCP_CHECK_REBIND_OK = 4
};

/***************************************************************************//**
 * @brief
 *    DHCP client object definition
 ******************************************************************************/
typedef struct {
  w5x00_ip4_addr_t  server_ip;            /// Server IP address
  uint8_t  mac_addr[6];                   /// MAC address

  w5x00_ip4_addr_t  local_ip;             /// Local IP
  w5x00_ip4_addr_t  subnet_mask;          /// Subnet mask
  w5x00_ip4_addr_t  gateway_ip;           /// Gateway IP
  w5x00_ip4_addr_t  dns_server_ip;        /// DNS Server IP

  uint32_t initial_transaction_id;        /// Initial Transaction ID
  uint32_t transaction_id;                /// Transaction ID

  uint32_t lease_time;                    /// Lease time
  uint32_t dhcp_T1, dhcp_T2;
  uint32_t renew_in_sec;                  /// Renewal Time Value (T1)
  uint32_t rebind_in_sec;                 /// Rebinding Time Value (T2)
  unsigned long timeout;                  /// Connection timeout
  unsigned long response_timeout;         /// Response timeout
  unsigned long last_check_lease_ms;
  uint8_t dhcp_state;
  w5x00_ethernet_udp_t udp_socket;        /// UDP socket to request DHCP configuration
} w5x00_dhcp_t;

/***************************************************************************//**
 * @brief
 *    Request new DHCP lease
 * @param[in] dhcp
 *    DHCP instance
 * @param[in] mac
 *    MAC address of the ethernet device
 * @param[in] timeout
 *    Connection timeout
 * @param[in] response_timeout
 *    Server response timeout
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_dhcp_request_dhcp_lease(w5x00_dhcp_t *dhcp,
                                          uint8_t *mac,
                                          unsigned long timeout,
                                          unsigned long response_timeout);

/***************************************************************************//**
 * @brief
 *    This function check and renew DHCP lease when it expires
 * @param[in] dhcp
 *    DHCP instance
 * @return
 *    return a #W5x00_DHCP_CHECK enum value
 ******************************************************************************/
enum W5x00_DHCP_CHECK w5x00_dhcp_check_lease(w5x00_dhcp_t *dhcp);

/** @} (end group DHCP) */
#ifdef __cplusplus
}
#endif
#endif // DHCP_H_
