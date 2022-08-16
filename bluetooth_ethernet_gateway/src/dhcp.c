/***************************************************************************//**
 * @file dhcp.c
 * @brief Wiznet Ethernet DHCP Functions
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

#include <string.h>

#include "w5x00.h"
#include "ethernet.h"
#include "ethernet_udp.h"
#include "dhcp.h"

// -----------------------------------------------------------------------------
// Defines

#define TAG                   "DHCP"

// DHCP state machine.
#define STATE_DHCP_START      0
#define STATE_DHCP_DISCOVER   1
#define STATE_DHCP_REQUEST    2
#define STATE_DHCP_LEASED     3
#define STATE_DHCP_REREQUEST  4
#define STATE_DHCP_RELEASE    5

#define DHCP_FLAGSBROADCAST   0x8000

// UDP port numbers for DHCP
#define DHCP_SERVER_PORT      67  /* from server to client */
#define DHCP_CLIENT_PORT      68  /* from client to server */

// DHCP message OP code
#define DHCP_BOOTREQUEST      1
#define DHCP_BOOTREPLY        2

// DHCP message type
#define DHCP_DISCOVER         1
#define DHCP_OFFER            2
#define DHCP_REQUEST          3
#define DHCP_DECLINE          4
#define DHCP_ACK              5
#define DHCP_NAK              6
#define DHCP_RELEASE          7
#define DHCP_INFORM           8

#define DHCP_HTYPE10MB        1
#define DHCP_HTYPE100MB       2

#define DHCP_HLENETHERNET     6
#define DHCP_HOPS             0
#define DHCP_SECS             0

#define MAGIC_COOKIE          0x63825363
#define MAX_DHCP_OPT          16

#define HOST_NAME             "WIZnet"
#define DEFAULT_LEASE         (900) // default lease time in seconds

enum
{
  padOption   = 0,
  subnetMask    = 1,
  timerOffset   = 2,
  routersOnSubnet   = 3,

  // timeServer   = 4,
  // nameServer    = 5,

  dns     = 6,
  // logServer   = 7,
  // cookieServer    = 8,
  // lprServer   = 9,
  // impressServer   = 10,
  // resourceLocationServer  = 11,
  hostName    = 12,
  // bootFileSize    = 13,
  // meritDumpFile   = 14,
  domainName    = 15,
  // swapServer    = 16,
  // rootPath    = 17,
  // extentionsPath    = 18,
  // IPforwarding    = 19,
  // nonLocalSourceRouting = 20,
  // policyFilter    = 21,
  // maxDgramReasmSize = 22,
  // defaultIPTTL    = 23,
  // pathMTUagingTimeout = 24,
  // pathMTUplateauTable = 25,
  // ifMTU     = 26,
  // allSubnetsLocal   = 27,
  // broadcastAddr   = 28,
  // performMaskDiscovery  = 29,
  // maskSupplier    = 30,
  // performRouterDiscovery  = 31,
  // routerSolicitationAddr  = 32,
  // staticRoute   = 33,
  // trailerEncapsulation  = 34,
  // arpCacheTimeout   = 35,
  // ethernetEncapsulation = 36,
  // tcpDefaultTTL   = 37,
  // tcpKeepaliveInterval  = 38,
  // tcpKeepaliveGarbage = 39,
  // nisDomainName   = 40,
  // nisServers    = 41,
  // ntpServers    = 42,
  // vendorSpecificInfo  = 43,
  // netBIOSnameServer = 44,
  // netBIOSdgramDistServer  = 45,
  // netBIOSnodeType   = 46,
  // netBIOSscope    = 47,
  // xFontServer   = 48,
  // xDisplayManager   = 49,
  dhcpRequestedIPaddr = 50,
  dhcpIPaddrLeaseTime = 51,
  // dhcpOptionOverload  = 52,
  dhcpMessageType   = 53,
  dhcpServerIdentifier  = 54,
  dhcpParamRequest  = 55,
  // dhcpMsg     = 56,
  // dhcpMaxMsgSize    = 57,
  dhcpT1value   = 58,
  dhcpT2value   = 59,
  // dhcpClassIdentifier = 60,
  dhcpClientIdentifier  = 61,
  endOption   = 255
};

typedef struct _RIP_MSG_FIXED
{
  uint8_t  op;
  uint8_t  htype;
  uint8_t  hlen;
  uint8_t  hops;
  uint32_t xid;
  uint16_t secs;
  uint16_t flags;
  uint8_t  ciaddr[4];
  uint8_t  yiaddr[4];
  uint8_t  siaddr[4];
  uint8_t  giaddr[4];
  uint8_t  chaddr[6];
} RIP_MSG_FIXED;

// -----------------------------------------------------------------------------
// Private function declarations

static void print_byte(char *buf, uint8_t n);
static void send_dhcp_message(w5x00_dhcp_t *dhcp,
                              uint8_t messageType,
                              uint16_t secondsElapsed);
static void reset_dhcp_lease(w5x00_dhcp_t *dhcp);
static sl_status_t request_dhcp_lease(w5x00_dhcp_t *dhcp);
static uint8_t parse_dhcp_response(w5x00_dhcp_t *dhcp,
                                   unsigned long response_timeout,
                                   uint32_t *transaction_id);

// -----------------------------------------------------------------------------
// Public function definitions

/***************************************************************************//**
 * DHCP Begin.
 ******************************************************************************/
sl_status_t w5x00_dhcp_request_dhcp_lease(w5x00_dhcp_t *dhcp,
                                          uint8_t *mac,
                                          unsigned long timeout,
                                          unsigned long response_timeout)
{
  if (dhcp == NULL || mac == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  dhcp->lease_time = 0;
  dhcp->dhcp_T1 = 0;
  dhcp->dhcp_T2 = 0;
  dhcp->timeout = timeout;
  dhcp->response_timeout = response_timeout;

  ethernet_udp_init(&dhcp->udp_socket);

  // zero out mac_addr
  memset(&(dhcp->mac_addr), 0, 6);
  reset_dhcp_lease(dhcp);

  memcpy((void*)dhcp->mac_addr, (void*)mac, 6);
  dhcp->dhcp_state = STATE_DHCP_START;
  return request_dhcp_lease(dhcp);
}

/***************************************************************************//**
 * DHCP Check Lease.
 ******************************************************************************/
enum W5x00_DHCP_CHECK w5x00_dhcp_check_lease(w5x00_dhcp_t *dhcp)
{
  enum W5x00_DHCP_CHECK rc = W5x00_DHCP_CHECK_NONE;

  unsigned long now = w5x00_get_tick_ms();
  unsigned long elapsed = now - dhcp->last_check_lease_ms;

  if (dhcp == NULL) {
    return W5x00_DHCP_CHECK_NONE;
  }
  // if more then one sec passed, reduce the counters accordingly
  if (elapsed >= 1000) {
    // set the new timestamps
    dhcp->last_check_lease_ms = now - (elapsed % 1000);
    elapsed = elapsed / 1000;

    // decrease the counters by elapsed seconds
    // we assume that the cycle time (elapsed) is fairly constant
    // if the remainder is less than cycle time * 2
    // do it early instead of late
    if (dhcp->renew_in_sec < elapsed * 2) {
      dhcp->renew_in_sec = 0;
    } else {
      dhcp->renew_in_sec -= elapsed;
    }
    if (dhcp->rebind_in_sec < elapsed * 2) {
      dhcp->rebind_in_sec = 0;
    } else {
      dhcp->rebind_in_sec -= elapsed;
    }
  }

  // if we have a lease but should renew, do it
  if ((dhcp->renew_in_sec == 0)
      && (dhcp->dhcp_state == STATE_DHCP_LEASED)) {
    dhcp->dhcp_state = STATE_DHCP_REREQUEST;
    if (SL_STATUS_OK == request_dhcp_lease(dhcp)) {
      rc = W5x00_DHCP_CHECK_RENEW_OK;
    } else {
      rc = W5x00_DHCP_CHECK_RENEW_FAIL;
    }
  }

  // if we have a lease or is renewing but should bind, do it
  if ((dhcp->rebind_in_sec == 0)
      && ((dhcp->dhcp_state == STATE_DHCP_LEASED)
          || (dhcp->dhcp_state == STATE_DHCP_START))) {
    // this should basically restart completely
    dhcp->dhcp_state = STATE_DHCP_START;
    reset_dhcp_lease(dhcp);
    if (SL_STATUS_OK == request_dhcp_lease(dhcp)) {
      rc = W5x00_DHCP_CHECK_REBIND_OK;
    } else {
      rc = W5x00_DHCP_CHECK_REBIND_FAIL;
    }
  }
  return rc;
}

// -----------------------------------------------------------------------------
// Private function

static void reset_dhcp_lease(w5x00_dhcp_t *dhcp)
{
  // zero out subnet_mask, gateway_ip, local_ip, server_ip, dns_server_ip
  dhcp->subnet_mask.addr = 0;
  dhcp->gateway_ip.addr = 0;
  dhcp->local_ip.addr = 0;
  dhcp->server_ip.addr = 0;
  dhcp->dns_server_ip.addr = 0;
}

  // return:0 on error, 1 if request is sent and response is received
static sl_status_t request_dhcp_lease(w5x00_dhcp_t *dhcp)
{
  uint8_t messageType = 0;
  sl_status_t result;

  // Pick an initial transaction ID
  dhcp->transaction_id = w5x00_random2(1UL, 2000UL);
  dhcp->initial_transaction_id = dhcp->transaction_id;

  w5x00_ethernet_udp_stop(&dhcp->udp_socket);
  result = w5x00_ethernet_udp_begin(&dhcp->udp_socket,
                                  DHCP_CLIENT_PORT);
  if (SL_STATUS_OK != SL_STATUS_OK) {
    // Couldn't get a socket
    w5x00_log_error(TAG,
                    "Couldn't get a socket to start DHCP configuration\r\n");
    return SL_STATUS_FAIL;
  }
  w5x00_log_info(TAG, "Start DHCP configuration on socket[%d] success\r\n",
                   dhcp->udp_socket.sockindex);

  result = SL_STATUS_FAIL;

  unsigned long startTime = w5x00_get_tick_ms();

  while (dhcp->dhcp_state != STATE_DHCP_LEASED) {
    if (dhcp->dhcp_state == STATE_DHCP_START) {
      dhcp->transaction_id++;
      send_dhcp_message(dhcp,
                        DHCP_DISCOVER,
                        ((w5x00_get_tick_ms() - startTime) / 1000));
      dhcp->dhcp_state = STATE_DHCP_DISCOVER;
    } else if (dhcp->dhcp_state == STATE_DHCP_REREQUEST) {
      dhcp->transaction_id++;
      send_dhcp_message(dhcp,
                        DHCP_REQUEST,
                        ((w5x00_get_tick_ms() - startTime) / 1000));
      dhcp->dhcp_state = STATE_DHCP_REQUEST;
    } else if (dhcp->dhcp_state == STATE_DHCP_DISCOVER) {
      uint32_t respId;
      messageType = parse_dhcp_response(dhcp,
                                        dhcp->response_timeout,
                                        &respId);
      if (messageType == DHCP_OFFER) {
        // We'll use the transaction ID that the offer came with,
        // rather than the one we were up to
        dhcp->transaction_id = respId;
        send_dhcp_message(dhcp,
                          DHCP_REQUEST,
                          ((w5x00_get_tick_ms() - startTime) / 1000));
        dhcp->dhcp_state = STATE_DHCP_REQUEST;
      }
    } else if (dhcp->dhcp_state == STATE_DHCP_REQUEST) {
      uint32_t respId;
      messageType = parse_dhcp_response(dhcp,
                                        dhcp->response_timeout,
                                        &respId);
      if (messageType == DHCP_ACK) {
        dhcp->dhcp_state = STATE_DHCP_LEASED;
        result = SL_STATUS_OK;
        // use default lease time if we didn't get it
        if (dhcp->lease_time == 0) {
          dhcp->lease_time = DEFAULT_LEASE;
        }
        // Calculate T1 & T2 if we didn't get it
        if (dhcp->dhcp_T1 == 0) {
          // T1 should be 50% of lease_time
            dhcp->dhcp_T1 = dhcp->lease_time >> 1;
        }
        if (dhcp->dhcp_T2 == 0) {
          // T2 should be 87.5% (7/8ths) of lease_time
            dhcp->dhcp_T2 = dhcp->lease_time - (dhcp->lease_time >> 3);
        }
        dhcp->renew_in_sec = dhcp->dhcp_T1;
        dhcp->rebind_in_sec = dhcp->dhcp_T2;
        w5x00_log_info(TAG, "DHCP configuration on socket[%d] is complete\r\n",
                           dhcp->udp_socket.sockindex);
      } else if (messageType == DHCP_NAK) {
        dhcp->dhcp_state = STATE_DHCP_START;
      }
    }

    if (messageType == 255) {
      messageType = 0;
      dhcp->dhcp_state = STATE_DHCP_START;
    }

    if ((result != SL_STATUS_OK)
        && ((w5x00_get_tick_ms() - startTime) > dhcp->timeout)) {
      w5x00_log_info(TAG, "DHCP configuration on socket[%d] is failed\r\n",
                                   dhcp->udp_socket.sockindex);
      break;
    }
  }

  // We're done with the socket now
  w5x00_ethernet_udp_stop(&dhcp->udp_socket);
  dhcp->transaction_id++;

  dhcp->last_check_lease_ms = w5x00_get_tick_ms();
  return result;
}

static void send_dhcp_message(w5x00_dhcp_t *dhcp,
                              uint8_t messageType,
                              uint16_t secondsElapsed)
{
  uint8_t buffer[32];
  memset(buffer, 0, 32);
  w5x00_ip4_addr_t dest_addr = { WIZNET_IPADDR_NONE }; // Broadcast address

  if (w5x00_ethernet_udp_begin_packet(&dhcp->udp_socket,
                                       &dest_addr,
                                       DHCP_SERVER_PORT) != SL_STATUS_OK) {
    // Serial.printf("DHCP transmit error\n");
    // FIXME Need to return errors
    return;
  }

  buffer[0] = DHCP_BOOTREQUEST;   // op
  buffer[1] = DHCP_HTYPE10MB;     // htype
  buffer[2] = DHCP_HLENETHERNET;  // hlen
  buffer[3] = DHCP_HOPS;          // hops

  // xid
  unsigned long xid = htonl(dhcp->transaction_id);
  memcpy(buffer + 4, &(xid), 4);

  // 8, 9 - seconds elapsed
  buffer[8] = ((secondsElapsed & 0xff00) >> 8);
  buffer[9] = (secondsElapsed & 0x00ff);

  // flags
  unsigned short flags = htons(DHCP_FLAGSBROADCAST);
  memcpy(buffer + 10, &(flags), 2);

  // ciaddr: already zeroed
  // yiaddr: already zeroed
  // siaddr: already zeroed
  // giaddr: already zeroed

  // put data in W5100 transmit buffer
  w5x00_ethernet_udp_write(&dhcp->udp_socket,
                            buffer,
                            28);

  memset(buffer, 0, 32); // clear local buffer

  memcpy(buffer, dhcp->mac_addr, 6); // chaddr

  // put data in W5100 transmit buffer
  w5x00_ethernet_udp_write(&dhcp->udp_socket,
                            buffer,
                            16);

  memset(buffer, 0, 32); // clear local buffer

  // leave zeroed out for sname && file
  // put in W5100 transmit buffer x 6 (192 bytes)

  for (int i = 0; i < 6; i++) {
    w5x00_ethernet_udp_write(&dhcp->udp_socket,
                             buffer,
                             32);
  }

  // OPT - Magic Cookie
  buffer[0] = (uint8_t)((MAGIC_COOKIE >> 24) & 0xFF);
  buffer[1] = (uint8_t)((MAGIC_COOKIE >> 16) & 0xFF);
  buffer[2] = (uint8_t)((MAGIC_COOKIE >> 8) & 0xFF);
  buffer[3] = (uint8_t)(MAGIC_COOKIE & 0xFF);

  // OPT - message type
  buffer[4] = dhcpMessageType;
  buffer[5] = 0x01;
  buffer[6] = messageType; // DHCP_REQUEST;

  // OPT - client identifier
  buffer[7] = dhcpClientIdentifier;
  buffer[8] = 0x07;
  buffer[9] = 0x01;
  memcpy(buffer + 10, dhcp->mac_addr, 6);

  // OPT - host name
  buffer[16] = hostName;
  buffer[17] = strlen(HOST_NAME) + 6; // length of hostname + last 3 bytes of mac address
  strcpy((char*)&(buffer[18]), HOST_NAME);

  print_byte((char*)&(buffer[24]), dhcp->mac_addr[3]);
  print_byte((char*)&(buffer[26]), dhcp->mac_addr[4]);
  print_byte((char*)&(buffer[28]), dhcp->mac_addr[5]);

  // put data in W5100 transmit buffer
  w5x00_ethernet_udp_write(&dhcp->udp_socket,
                            buffer,
                            30);

  if (messageType == DHCP_REQUEST) {
    buffer[0] = dhcpRequestedIPaddr;
    buffer[1] = 0x04;
    buffer[2] = w5x00_ip4_addr_get_byte(&dhcp->local_ip, 0);
    buffer[3] = w5x00_ip4_addr_get_byte(&dhcp->local_ip, 1);
    buffer[4] = w5x00_ip4_addr_get_byte(&dhcp->local_ip, 2);
    buffer[5] = w5x00_ip4_addr_get_byte(&dhcp->local_ip, 3);

    buffer[6] = dhcpServerIdentifier;
    buffer[7] = 0x04;
    buffer[8] = w5x00_ip4_addr_get_byte(&dhcp->server_ip, 0);
    buffer[9] = w5x00_ip4_addr_get_byte(&dhcp->server_ip, 1);
    buffer[10] = w5x00_ip4_addr_get_byte(&dhcp->server_ip, 2);
    buffer[11] = w5x00_ip4_addr_get_byte(&dhcp->server_ip, 3);

    // put data in W5100 transmit buffer
    w5x00_ethernet_udp_write(&dhcp->udp_socket,
                              buffer,
                              12);
  }

  buffer[0] = dhcpParamRequest;
  buffer[1] = 0x06;
  buffer[2] = subnetMask;
  buffer[3] = routersOnSubnet;
  buffer[4] = dns;
  buffer[5] = domainName;
  buffer[6] = dhcpT1value;
  buffer[7] = dhcpT2value;
  buffer[8] = endOption;

  // put data in W5100 transmit buffer
  w5x00_ethernet_udp_write(&dhcp->udp_socket,
                            buffer,
                            9);

  w5x00_ethernet_udp_end_packet(&dhcp->udp_socket);
}

static uint8_t parse_dhcp_response(w5x00_dhcp_t *dhcp,
                                   unsigned long response_timeout,
                                   uint32_t *transaction_id)
{
  uint8_t type = 0;
  uint8_t opt_len = 0;

  unsigned long startTime = w5x00_get_tick_ms();

  while (w5x00_ethernet_udp_parse_packet(&dhcp->udp_socket) <= 0) {
    if ((w5x00_get_tick_ms() - startTime) > response_timeout) {
      return 255;
    }
    w5x00_delay_ms(50);
  }
  // start reading in the packet
  RIP_MSG_FIXED fixedMsg;
  w5x00_ethernet_udp_read(&dhcp->udp_socket,
                          (uint8_t*)&fixedMsg,
                          sizeof(RIP_MSG_FIXED));

  if ((fixedMsg.op == DHCP_BOOTREPLY)
      && (dhcp->udp_socket.remote_port == DHCP_SERVER_PORT)) {
    *transaction_id = ntohl(fixedMsg.xid);
    if ((memcmp(fixedMsg.chaddr, dhcp->mac_addr, 6) != 0)
        || (*transaction_id < dhcp->initial_transaction_id)
        || (*transaction_id > dhcp->transaction_id)) {
      // Need to read the rest of the packet here regardless
      w5x00_ethernet_udp_flush(&dhcp->udp_socket); // FIXME
      return 0;
    }

    memcpy(&dhcp->local_ip, fixedMsg.yiaddr, 4);

    // Skip to the option part
    w5x00_ethernet_udp_read(&dhcp->udp_socket,
                            (uint8_t *)NULL,
                            240 - (int)sizeof(RIP_MSG_FIXED));

    while (w5x00_ethernet_udp_available(&dhcp->udp_socket) > 0) {
      switch (w5x00_ethernet_udp_read_byte(&dhcp->udp_socket)) {
      case endOption:
        break;

      case padOption:
        break;

      case dhcpMessageType:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        type = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        break;

      case subnetMask:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t*)&dhcp->subnet_mask,
                                4);
        break;

      case routersOnSubnet:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t*)&dhcp->gateway_ip,
                                4);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t *)NULL,
                                opt_len - 4);
        break;

      case dns:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t*)&dhcp->dns_server_ip,
                                4);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t *)NULL,
                                opt_len - 4);
        break;

      case dhcpServerIdentifier:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        if ((dhcp->server_ip.addr == WIZNET_IPADDR_ANY)
            || (dhcp->server_ip.addr == dhcp->udp_socket.remote_ip.addr)) {
          w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                  (uint8_t*)&dhcp->server_ip,
                                  sizeof(dhcp->server_ip));
        } else {
          // Skip over the rest of this option
          w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                  (uint8_t *)NULL,
                                  opt_len);
        }
        break;

      case dhcpT1value:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t*)&dhcp->dhcp_T1,
                                sizeof(dhcp->dhcp_T1));
        dhcp->dhcp_T1 = ntohl(dhcp->dhcp_T1);
        break;

      case dhcpT2value:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t*)&dhcp->dhcp_T2,
                                sizeof(dhcp->dhcp_T2));
        dhcp->dhcp_T2 = ntohl(dhcp->dhcp_T2);
        break;

      case dhcpIPaddrLeaseTime:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t*)&dhcp->lease_time,
                                sizeof(dhcp->lease_time));
        dhcp->lease_time = ntohl(dhcp->lease_time);
        dhcp->renew_in_sec = dhcp->lease_time;
        break;

      default:
        opt_len = w5x00_ethernet_udp_read_byte(&dhcp->udp_socket);
        // Skip over the rest of this option
        w5x00_ethernet_udp_read(&dhcp->udp_socket,
                                (uint8_t *)NULL,
                                opt_len);
        break;
      }
    }
  }

  // Need to skip to end of the packet regardless here
  w5x00_ethernet_udp_flush(&dhcp->udp_socket); // FIXME

  return type;
}

static void print_byte(char *buf, uint8_t n)
{
  char *str = &buf[1];
  buf[0] = '0';
  do {
    unsigned long m = n;
    n /= 16;
    char c = m - 16 * n;
    *str-- = c < 10 ? c + '0' : c + 'A' - 10;
  } while (n);
}
