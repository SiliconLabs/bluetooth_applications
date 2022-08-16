/***************************************************************************//**
 * @file ethernet.h
 * @brief Wiznet Ethernet Function Prototypes
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

#ifndef ETHERNET_H_
#define ETHERNET_H_

#include "w5x00_utils.h"
#include "dhcp.h"

/***************************************************************************//**
 * @addtogroup Ethernet
 * @brief  Ethernet interface configuration.
 * @details
 * @{
 ******************************************************************************/

enum EthernetLinkStatus {
  EthernetLinkUnknown,  /*!< Ethernet link unknown status */
  EthernetLinkON,       /*!< Ethernet link unknown status=on */
  EthernetLinkOFF       /*!< Ethernet link unknown status=off */
};

enum EthernetHardwareStatus {
  EthernetNoHardware,   /*!< Ethernet chip is not detected */
  EthernetW5100,        /*!< Ethernet chip is W5100 */
  EthernetW5200,        /*!< Ethernet chip is W5200 */
  EthernetW5500         /*!< Ethernet chip is W5500 */
};

/***************************************************************************//**
 * @brief
 *    Ethernet object definition
 ******************************************************************************/
typedef struct {
  w5x00_ip4_addr_t dns_server_address;  /// DNS server ip address
  w5x00_dhcp_t dhcp;                    /// DHCP client object
} w5x00_ethernet_t;

/***************************************************************************//**
 * @brief
 *    Initialize ethernet interface with DHCP
 * @param[in] eth
 *    Ethernet instance
 * @param[in] mac
 *    MAC address
 * @param[in] timeout
 *    Connection timeout
 * @param response_timeout
 *    Server response timeout
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_dhcp_init(w5x00_ethernet_t *eth,
                                     uint8_t *mac,
                                     uint32_t timeout,
                                     uint32_t response_timeout);

/***************************************************************************//**
 * @brief
 *    Initialize ethernet interface with static address
 * @param[in] eth
 *    Ethernet instance
 * @param[in] mac
 *    MAC address
 * @param[in] local_ip
 * @param[in] gateway_ip
 * @param[in] subnet_mask
 * @param[in] dns_server_ip
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_static_init(w5x00_ethernet_t *eth,
                                       uint8_t *mac,
                                       w5x00_ip4_addr_t *local_ip,
                                       w5x00_ip4_addr_t *gateway_ip,
                                       w5x00_ip4_addr_t *subnet_mask,
                                       w5x00_ip4_addr_t *dns_server_ip,
                                       uint32_t timeout);

/***************************************************************************//**
 * @brief
 *    Set DNS server ip
 * @param eth
 *    Ethernet instance
 * @param ip
 *    DNS server IP
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_dns_server(w5x00_ethernet_t *eth,
                                          w5x00_ip4_addr_t ip);

/***************************************************************************//**
 * @brief
 *    Get ethernet link status
 * @param[in] eth
 *    Ethernet instance
 * @return
 *    returns an #EthernetLinkStatus value enum
 ******************************************************************************/
enum EthernetLinkStatus w5x00_ethernet_link_status(w5x00_ethernet_t *eth);

/***************************************************************************//**
 * @brief
 *    Get ethernet link status
 * @return
 *    returns an #EthernetHardwareStatus value enum
 ******************************************************************************/
enum EthernetHardwareStatus w5x00_ethernet_hardware_status(void);

/***************************************************************************//**
 * @brief
 *    Maintain DHCP lease
 * @param[in] eth
 *    Ethernet instance
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_maintain(w5x00_ethernet_t *eth);

/***************************************************************************//**
 * @brief
 *    Get local ip
 * @param[in] eth
 *    Ethernet instance
 * @param[out] ip
 *    Local ip
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_get_local_ip(w5x00_ethernet_t *eth,
                                        w5x00_ip4_addr_t *ip);

/***************************************************************************//**
 * @brief
 *    Set local ip
 * @param[in] eth
 *    Ethernet instance
 * @param[in] ip
 *    Local ip
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_local_ip(w5x00_ethernet_t *eth,
                                        w5x00_ip4_addr_t *ip);

/***************************************************************************//**
 * @brief
 *    Get subnet mask
 * @param[in] eth
 *    Ethernet instance
 * @param[out] subnet
 *    Subnet mask
 * @return
 *    Subnet mask
 ******************************************************************************/
sl_status_t w5x00_ethernet_get_subnet_mask(w5x00_ethernet_t *eth,
                                           w5x00_ip4_addr_t *subnet);

/***************************************************************************//**
 * @brief
 *    Set subnet mask
 * @param[in] eth
 *    Ethernet instance
 * @param[in] subnet
 *    Subnet mask
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_subnet_mask(w5x00_ethernet_t *eth,
                                           w5x00_ip4_addr_t *subnet);

/***************************************************************************//**
 * @brief
 *    Get gateway ip
 * @param[in] eth
 *    Ethernet instance
 * @param[out] gateway
 *    Gateway ip
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_get_gateway_ip(w5x00_ethernet_t *eth,
                                          w5x00_ip4_addr_t *gateway);

/***************************************************************************//**
 * @brief
 *    Set gateway ip
 * @param[in] eth
 *    Ethernet instance
 * @param[in] ip
 *    Gateway ip
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_gateway_ip(w5x00_ethernet_t *eth,
                                          w5x00_ip4_addr_t *gateway);

/** @} (end group Ethernet) */

#endif // ETHERNET_H_
