/***************************************************************************//**
 * @file dns.h
 * @brief Wiznet Ethernet DNS Client Function Prototypes.
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

#ifndef DNS_H_
#define DNS_H_

#include "ethernet.h"
#include "ethernet_udp.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup DNS DNS Client
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup DNS
 * @brief  Ethernet DNS client function.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    DNS client object definition
 ******************************************************************************/
typedef struct
{
  w5x00_ip4_addr_t dns_server;      /// DNS server ip address
  uint16_t request_id;              /// Request ID
  w5x00_ethernet_udp_t udp_socket;  /// UDP socket
} w5x00_dns_t;

/***************************************************************************//**
 * @brief
 *    Init DNS instance
 * @param[in] dns
 *    DNS instance
 * @param[in] a_dns_server
 *    DNS Server IP
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_dns_init(w5x00_dns_t *dns,
                           const w5x00_ip4_addr_t a_dns_server);

/***************************************************************************//**
 * @brief
 *    DNS Get host IP by name
 * @param[in] dns
 *    DNS instance
 * @param[in] a_hostname
 *    hostname
 * @param[out] a_result
 *    IP address of the hostname
 * @param[in] timeout
 *    Connection timeout
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_dns_get_host_by_name(w5x00_dns_t *dns,
                                       const char *a_hostname,
                                       w5x00_ip4_addr_t *a_result,
                                       uint16_t timeout);
/** @} (end group DNS) */
#ifdef __cplusplus
}
#endif
#endif // DNS_H_
