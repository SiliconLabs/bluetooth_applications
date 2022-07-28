/***************************************************************************//**
 * @file sntp.c
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
#include <stddef.h>
#include <string.h>

#include "ethernet.h"
#include "w5x00.h"
#include "dns.h"
#include "sntp.h"

#define TAG                      "SNTP"

#define NTP_PACKET_SIZE          48 // NTP time stamp is in the first 48 bytes of the message

#define SOCKET_NONE              255
// Port number that DNS servers listen on
#define SNTP_DEFAULT_PORT        123

// Possible return codes from ProcessResponse
#define SUCCESS                  1
#define TIMED_OUT                -1
#define INVALID_SERVER           -2
#define TRUNCATED                -3
#define INVALID_RESPONSE         -4

static int process_response(w5x00_ethernet_udp_t *udp_socket,
                            uint16_t timeout,
                            uint8_t *packet_buffer,
                            int size);

/***************************************************************************//**
 * SNTP Begin.
 ******************************************************************************/
sl_status_t w5x00_sntp_init(w5x00_sntp_t *sntp, w5x00_ethernet_t *eth)
{
  if ((sntp == NULL) || (eth == NULL)) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  sntp->eth = eth;
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * SNTP Update Timestamp From Host.
 ******************************************************************************/
sl_status_t w5x00_sntp_update_timestamp_from_host(w5x00_sntp_t *sntp,
                                                  const char *host,
                                                  uint16_t port,
                                                  uint16_t timeout)
{
  w5x00_dns_t dns_client; // Look up the host first
  w5x00_ip4_addr_t remote_addr;

  if ((sntp == NULL) || (host == NULL)) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_dns_init(&dns_client, sntp->eth->dns_server_address);
  if (SL_STATUS_OK != w5x00_dns_get_host_by_name(&dns_client,
                                                 host,
                                                 &remote_addr,
                                                 timeout)) {
    return SL_STATUS_FAIL;
  }
  return w5x00_sntp_update_timestamp(sntp, remote_addr, port, timeout);
}

/***************************************************************************//**
 * SNTP Update Timestamp From IP.
 ******************************************************************************/
sl_status_t w5x00_sntp_update_timestamp(w5x00_sntp_t *sntp,
                                        w5x00_ip4_addr_t server_ip,
                                        uint16_t port,
                                        uint16_t timeout)
{
  sl_status_t ret;
  int status = TIMED_OUT;
  size_t len;
  // buffer to hold incoming and outgoing packets
  uint8_t packet_buffer[NTP_PACKET_SIZE];

  if (sntp == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  // Find a socket to use
  ret = w5x00_ethernet_udp_begin(&sntp->udp_socket,
                                  1024 + (w5x00_get_tick_count() & 0xF));
  if (ret == SL_STATUS_OK) {
    w5x00_log_info(TAG, "Start sending request to NTP server: ");
    w5x00_log_print_ip(&server_ip);
    w5x00_log_printf("\r\n");
    // Send DNS request
    ret = w5x00_ethernet_udp_begin_packet(&sntp->udp_socket,
                                          &server_ip,
                                          port);
    if (ret == SL_STATUS_OK) {
      memset(packet_buffer, 0, NTP_PACKET_SIZE);
      // Initialize values needed to form NTP request
      // (see URL above for details on the packets)
      packet_buffer[0] = 0b11100011;   // LI, Version, Mode
      packet_buffer[1] = 0;     // Stratum, or type of clock
      packet_buffer[2] = 6;     // Polling Interval
      packet_buffer[3] = 0xEC;  // Peer Clock Precision
      // 8 bytes of zero for Root Delay & Root Dispersion
      packet_buffer[12] = 49;
      packet_buffer[13] = 0x4E;
      packet_buffer[14] = 49;
      packet_buffer[15] = 52;
      len = w5x00_ethernet_udp_write(&sntp->udp_socket,
                                     packet_buffer,
                                     sizeof(packet_buffer));
      if (len != 0) {
        // And finally send the request
        ret = w5x00_ethernet_udp_end_packet(&sntp->udp_socket);
        if (ret == SL_STATUS_OK) {
          // Now wait for a response
          int wait_retries = 0;
          while ((wait_retries < 3) && (status == TIMED_OUT)) {
            // Wait for a response packet
            status = process_response(&sntp->udp_socket,
                                      timeout,
                                      packet_buffer,
                                      NTP_PACKET_SIZE);
            wait_retries++;
          }
        }
      }
    } else {
      w5x00_log_error(TAG, "Couldn't get a socket for SNTP\r\n");
    }

    // We're done with the socket now
    w5x00_ethernet_udp_stop(&sntp->udp_socket);
  }
  if (status == SUCCESS) {
    sntp->epoch_timestamp = (uint32_t)packet_buffer[40] << 24
                              | (uint32_t)packet_buffer[41] << 16
                              | (uint32_t)packet_buffer[42] << 8
                              | (uint32_t)packet_buffer[43];
    w5x00_log_info(TAG, "Request to NTP server: '");
    w5x00_log_print_ip(&server_ip);
    w5x00_log_printf("' is complete\r\n");
    return SL_STATUS_OK;
  } else if (status == TIMED_OUT) {
    w5x00_log_info(TAG, "Request to NTP server: '");
    w5x00_log_print_ip(&server_ip);
    w5x00_log_printf("' is timed out\r\n");
  } else if (status == TRUNCATED) {
    w5x00_log_info(TAG, "Response from NTP server: '");
    w5x00_log_print_ip(&server_ip);
    w5x00_log_printf("' is truncated\r\n");
  } else {
    w5x00_log_info(TAG, "Response from NTP server: '");
    w5x00_log_print_ip(&server_ip);
    w5x00_log_printf("' is failed\r\n");
  }
  return SL_STATUS_FAIL;
}

static int process_response(w5x00_ethernet_udp_t *udp_socket,
                            uint16_t timeout,
                            uint8_t *packet_buffer,
                            int size)
{
  uint32_t start_time = w5x00_get_tick_ms();

  // Wait for a response packet
  while (w5x00_ethernet_udp_parse_packet(udp_socket) <= 0) {
    if ((w5x00_get_tick_ms() - start_time) > timeout) {
      return TIMED_OUT;
    }
    w5x00_delay_ms(50);
  }

  // Read through the rest of the response
  if (w5x00_ethernet_udp_available(udp_socket) < size) {
    return TRUNCATED;
  }

  if (size == w5x00_ethernet_udp_read(udp_socket,
                                      packet_buffer,
                                      size)) {
    return SUCCESS;
  }

  return INVALID_RESPONSE;
}
