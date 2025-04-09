/***************************************************************************//**
 * @file dweet_http_client.c
 * @brief Update environment temperature and humidity to dweet cloud
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ********************************************************************************
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
 * # Experimental Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include <string.h>
#include "sl_status.h"
#include "w5x00.h"
#include "w5x00_utils.h"
#include "ethernet.h"
#include "dns.h"
#include "ethernet_client.h"
#include "app_log.h"
#include "dweet_http_client.h"
#include "sl_spidrv_instances.h"

#define abs(n)      ((n) < 0 ? -(n):(n))

#define app_log_print_ip(ip)                             \
  app_log("%d.%d.%d.%d", w5x00_ip4_addr_get_byte(ip, 0), \
          w5x00_ip4_addr_get_byte(ip, 1),                \
          w5x00_ip4_addr_get_byte(ip, 2),                \
          w5x00_ip4_addr_get_byte(ip, 3))

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static w5x00_ethernet_t eth;
static w5x00_ethernet_client_t client;
static w5x00_ip4_addr_t remote_ip;
static const w5x00_ip4_addr_t dns_server1 = { WIZNET_IP4_DATA(1, 1, 1, 1) };
static char message_buffer[1024];

sl_status_t dweet_http_client_init(void)
{
  sl_status_t status;
  w5x00_ip4_addr_t local_ip, gateway_ip, subnet_mask;
  w5x00_dns_t dns;

  w5x00_init((mikroe_spi_handle_t)sl_spidrv_mikroe_handle);
  status = w5x00_ethernet_dhcp_init(&eth, mac, 30000, 10000);
  app_log("DHCP configuration: %s\r\n",
          SL_STATUS_OK == status ? "success":"failed");

  if (SL_STATUS_OK != status) {
    enum EthernetLinkStatus link_status;

    link_status = w5x00_ethernet_link_status(&eth);
    if (EthernetLinkON == link_status) {
      app_log("Ethernet link status is on\r\n");
    } else if (EthernetLinkOFF == link_status) {
      app_log("Ethernet link status is off\r\n");
    }
    return status;
  }

  w5x00_ethernet_set_dns_server(&eth, dns_server1);

  memset(&local_ip, 0, sizeof(local_ip));
  w5x00_ethernet_get_local_ip(&eth, &local_ip);
  app_log_info("local ip:    ");
  app_log_print_ip(&local_ip);
  app_log("\r\n");

  memset(&gateway_ip, 0, sizeof(gateway_ip));
  w5x00_ethernet_get_gateway_ip(&eth, &gateway_ip);
  app_log_info("gateway:     ");
  app_log_print_ip(&gateway_ip);
  app_log("\r\n");

  memset(&subnet_mask, 0, sizeof(subnet_mask));
  w5x00_ethernet_get_subnet_mask(&eth, &subnet_mask);
  app_log_info("subnet mask: ");
  app_log_print_ip(&subnet_mask);
  app_log("\r\n");

  app_log_info("dns:         ");
  app_log_print_ip(&eth.dns_server_address);
  app_log("\r\n");

  w5x00_dns_init(&dns, eth.dns_server_address);
  status = w5x00_dns_get_host_by_name(&dns,
                                      "dweet.io",
                                      &remote_ip,
                                      10000);

  return status;
}

sl_status_t dweet_http_client_update_rht(const char *device_name,
                                         float rh,
                                         float t)
{
  sl_status_t status;
  int length;

  w5x00_ethernet_client_init(&client, &eth, 10000);

  status = w5x00_ethernet_client_connect(&client,
                                         remote_ip,
                                         80);
  if (SL_STATUS_OK != status) {
    return status;
  }

  length = snprintf(message_buffer,
                    sizeof(message_buffer),
                    "GET http://dweet.io/dweet/for/%s?temperature=%.2f&humidity=%.2f HTTP/1.1\r\nHost: dweet.io\r\nAccept: text/html\r\n\r\n",
                    device_name,
                    t,
                    rh);
  while (w5x00_ethernet_client_available_for_write(&client) < length) {}

  if (length != w5x00_ethernet_client_write(&client,
                                            (uint8_t *)message_buffer,
                                            length)) {
    status = SL_STATUS_FAIL;
    goto stop;
  }

  length = 0;
  while (length <= 0
         && w5x00_ethernet_client_connected(&client)) {
    length = w5x00_ethernet_client_read(&client,
                                        (uint8_t *)message_buffer,
                                        sizeof(message_buffer) - 1);
  }
  stop:
  w5x00_ethernet_client_stop(&client);
  return status;
}
