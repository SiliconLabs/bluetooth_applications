/***************************************************************************//**
 * @file ethernet.c
 * @brief Wiznet Ethernet Function
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

#include "w5x00.h"
#include "dhcp.h"
#include "socket.h"
#include "ethernet.h"

/***************************************************************************//**
 * Ethernet DHCP Init.
 ******************************************************************************/
sl_status_t w5x00_ethernet_dhcp_init(w5x00_ethernet_t *eth,
                                     uint8_t *mac,
                                     uint32_t timeout,
                                     uint32_t response_timeout)
{
  w5x00_ip4_addr_t ip = { 0 };

  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  // Initialise the basic info
  if (!w5x00_init()) {
    return SL_STATUS_FAIL;
  }
  w5x00_set_mac_address(mac);
  w5x00_set_ip_address((uint8_t *)&ip.addr);

  // Now try to get our config info from a DHCP server
  sl_status_t ret = w5x00_dhcp_request_dhcp_lease(&(eth->dhcp),
                                                  mac,
                                                  timeout,
                                                  response_timeout);
  if (ret == SL_STATUS_OK) {
    // We've successfully found a DHCP server and got our configuration
    // info, so set things accordingly
    w5x00_set_ip_address((uint8_t *)&(eth->dhcp.local_ip));
    w5x00_set_gateway_ip((uint8_t *)&(eth->dhcp.gateway_ip));
    w5x00_set_subnet_mask((uint8_t *)&(eth->dhcp.subnet_mask));
    eth->dns_server_address = eth->dhcp.dns_server_ip;
    w5x00_socket_port_rand(w5x00_get_tick_count());
  }
  return ret;
}

/***************************************************************************//**
 * Ethernet Static Init.
 ******************************************************************************/
sl_status_t w5x00_ethernet_static_init(w5x00_ethernet_t *eth,
                                       uint8_t *mac,
                                       w5x00_ip4_addr_t *local_ip,
                                       w5x00_ip4_addr_t *gateway_ip,
                                       w5x00_ip4_addr_t *subnet_mask,
                                       w5x00_ip4_addr_t *dns_server_ip,
                                       uint32_t timeout)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  // Initialise the basic info
  if (!w5x00_init()) {
    return SL_STATUS_FAIL;
  }
  w5x00_set_mac_address(mac);

  w5x00_set_ip_address((uint8_t *)local_ip);
  w5x00_set_gateway_ip((uint8_t *)gateway_ip);
  w5x00_set_subnet_mask((uint8_t *)subnet_mask);
  eth->dns_server_address = *dns_server_ip;
  w5x00_socket_port_rand(w5x00_get_tick_count());

  // Wait for link
  uint32_t last_tick = w5x00_get_tick_ms();
  while (w5x00_get_tick_ms() - last_tick < timeout) {
    if (w5x00_get_link_status() == LINK_ON) {
      return SL_STATUS_OK;
    }
    w5x00_delay_ms(10);
  }
  return SL_STATUS_FAIL;
}

/***************************************************************************//**
 * Ethernet Set DNS server IP.
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_dns_server(w5x00_ethernet_t *eth,
                                          w5x00_ip4_addr_t ip)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  eth->dns_server_address = ip;
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Get Link Status.
 ******************************************************************************/
enum EthernetLinkStatus w5x00_ethernet_link_status(w5x00_ethernet_t *eth)
{
  if (eth == NULL) {
    return EthernetLinkUnknown;
  }

  switch (w5x00_get_link_status()) {
    case UNKNOWN:  return EthernetLinkUnknown;
    case LINK_ON:  return EthernetLinkON;
    case LINK_OFF: return EthernetLinkOFF;
    default:       return EthernetLinkUnknown;
  }
  return EthernetLinkUnknown;
}

/***************************************************************************//**
 * Ethernet Get Hardware Status.
 ******************************************************************************/
enum EthernetHardwareStatus w5x00_ethernet_hardware_status(void)
{
  switch (w5x00_get_chip()) {
    case 51: return EthernetW5100;
    case 52: return EthernetW5200;
    case 55: return EthernetW5500;
    default: return EthernetNoHardware;
  }
}

/***************************************************************************//**
 * Ethernet Maintain DHCP Lease.
 ******************************************************************************/
sl_status_t w5x00_ethernet_maintain(w5x00_ethernet_t *eth)
{
  int rc = W5x00_DHCP_CHECK_NONE;
  // we have a pointer to dhcp, use it
  rc = w5x00_dhcp_check_lease(&eth->dhcp);
  switch (rc) {
  case W5x00_DHCP_CHECK_NONE:
    // nothing done
    break;
  case W5x00_DHCP_CHECK_RENEW_OK:
  case W5x00_DHCP_CHECK_REBIND_OK:
    // we might have got a new IP.
    w5x00_set_ip_address((uint8_t *)&(eth->dhcp.local_ip));
    w5x00_set_gateway_ip((uint8_t *)&(eth->dhcp.gateway_ip));
    w5x00_set_subnet_mask((uint8_t *)&(eth->dhcp.subnet_mask));
    eth->dns_server_address = eth->dhcp.dns_server_ip;
    break;
  default:
    // this is actually an error, it will retry though
    return SL_STATUS_FAIL;
    break;
  }
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Get Local IP.
 ******************************************************************************/
sl_status_t w5x00_ethernet_get_local_ip(w5x00_ethernet_t *eth,
                                        w5x00_ip4_addr_t *ip)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_get_ip_address((uint8_t *)ip);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Set Local IP.
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_local_ip(w5x00_ethernet_t *eth,
                                        w5x00_ip4_addr_t *ip)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_set_ip_address((uint8_t *)ip);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Get Subnet Mask.
 ******************************************************************************/
sl_status_t w5x00_ethernet_get_subnet_mask(w5x00_ethernet_t *eth,
                                           w5x00_ip4_addr_t *subnet)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_get_subnet_mask((uint8_t *)subnet);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Set Subnet Mask.
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_subnet_mask(w5x00_ethernet_t *eth,
                                           w5x00_ip4_addr_t *subnet)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_set_subnet_mask((uint8_t *)subnet);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Get Gateway IP.
 ******************************************************************************/
sl_status_t w5x00_ethernet_get_gateway_ip(w5x00_ethernet_t *eth,
                                          w5x00_ip4_addr_t *gateway)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_get_gateway_ip((uint8_t *)gateway);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Set Gateway IP.
 ******************************************************************************/
sl_status_t w5x00_ethernet_set_gateway_ip(w5x00_ethernet_t *eth,
                                          w5x00_ip4_addr_t *gateway)
{
  if (eth == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_set_gateway_ip((uint8_t *)gateway);
  return SL_STATUS_OK;
}
