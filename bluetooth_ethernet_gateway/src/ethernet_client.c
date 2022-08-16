/***************************************************************************//**
 * @file ethernet_client.c
 * @brief Wiznet Ethernet TCP Client.
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
#include "dns.h"
#include "ethernet.h"
#include "socket.h"
#include "ethernet_client.h"

/***************************************************************************//**
 * Ethernet Client Init.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_init(w5x00_ethernet_client_t *c,
                                       w5x00_ethernet_t *eth,
                                       uint16_t timeout)
{
  if (c == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  c->sockindex = W5x00_MAX_SOCK_NUM;
  c->eth = eth;
  c->timeout = timeout;
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Client Connect Host.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_connect_host(w5x00_ethernet_client_t *c,
                                               const char *host,
                                               uint16_t port)
{
  w5x00_dns_t dns_client; // Look up the host first
  w5x00_ip4_addr_t remote_addr;

  if (c == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (c->sockindex < W5x00_MAX_SOCK_NUM) {
    if (w5x00_socket_status(c->sockindex) != SnSR_CLOSED) {
      w5x00_socket_disconnect(c->sockindex); // TODO: should we call stop()?
    }
    c->sockindex = W5x00_MAX_SOCK_NUM;
  }
  w5x00_dns_init(&dns_client, c->eth->dns_server_address);
  if (SL_STATUS_OK != w5x00_dns_get_host_by_name(&dns_client,
                                                 host,
                                                 &remote_addr,
                                                 c->timeout)) {
    return SL_STATUS_FAIL;
  }
//  w5x00_log_printf("remote server ip:");
//  w5x00_log_print_ip(&remote_addr);
//  w5x00_log_printf("\r\n");
  return w5x00_ethernet_client_connect(c, remote_addr, port);
}

/***************************************************************************//**
 * Ethernet Client Connect.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_connect(w5x00_ethernet_client_t *c,
                                          w5x00_ip4_addr_t ip,
                                          uint16_t port)
{
  if (c == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (c->sockindex < W5x00_MAX_SOCK_NUM) {
    if (w5x00_socket_status(c->sockindex) != SnSR_CLOSED) {
      w5x00_socket_disconnect(c->sockindex); // TODO: should we call stop()?
    }
    c->sockindex = W5x00_MAX_SOCK_NUM;
  }
  if ((ip.addr == WIZNET_IPADDR_ANY) || (ip.addr == WIZNET_IPADDR_NONE)) {
    return 0;
  }
  c->sockindex = w5x00_socket_begin(SnMR_TCP, 0);
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  w5x00_socket_connect(c->sockindex,
                        &ip,
                        port);
  uint32_t start = w5x00_get_tick_ms();
  while (1) {
    uint8_t stat = w5x00_socket_status(c->sockindex);
    if (stat == SnSR_ESTABLISHED) {
      return SL_STATUS_OK;
    }
    if (stat == SnSR_CLOSE_WAIT) {
      return SL_STATUS_OK;
    }
    if (stat == SnSR_CLOSED) {
      return SL_STATUS_FAIL;
    }
    if (w5x00_get_tick_ms() - start > c->timeout) {
      break;
    }
    w5x00_delay_ms(1);
  }
  w5x00_socket_close(c->sockindex);
  c->sockindex = W5x00_MAX_SOCK_NUM;
  return SL_STATUS_FAIL;
}

/***************************************************************************//**
 * Ethernet Client Available For Write.
 ******************************************************************************/
int w5x00_ethernet_client_available_for_write(w5x00_ethernet_client_t *c)
{
  if (c == NULL) {
    return -1;
  }
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  return w5x00_socket_send_available(c->sockindex);
}

/***************************************************************************//**
 * Ethernet Client Write.
 ******************************************************************************/
int w5x00_ethernet_client_write(w5x00_ethernet_client_t *c,
                                const uint8_t *buf,
                                size_t size)
{
  if (c == NULL) {
    return -1;
  }
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  if (w5x00_socket_send(c->sockindex, buf, size)) {
    return size;
  }
  return 0;
}

/***************************************************************************//**
 * Ethernet Client Available For Read.
 ******************************************************************************/
int w5x00_ethernet_client_available(w5x00_ethernet_client_t *c)
{
  if (c == NULL) {
    return -1;
  }
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  return w5x00_socket_recv_available(c->sockindex);
  // TODO: do the Wiznet chips automatically retransmit TCP ACK
  // packets if they are lost by the network?  Someday this should
  // be checked by a man-in-the-middle test which discards certain
  // packets.  If ACKs aren't resent, we would need to check for
  // returning 0 here and after a timeout do another Sock_RECV
  // command to cause the Wiznet chip to resend the ACK packet.
}

/***************************************************************************//**
 * Ethernet Client Read.
 ******************************************************************************/
int w5x00_ethernet_client_read(w5x00_ethernet_client_t *c,
                               uint8_t *buf,
                               size_t size)
{
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  return w5x00_socket_recv(c->sockindex, buf, size);
}

/***************************************************************************//**
 * Ethernet Client Peek.
 ******************************************************************************/
int w5x00_ethernet_client_peek(w5x00_ethernet_client_t *c)
{
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return -1;
  }
  if (!w5x00_ethernet_client_available(c)) {
    return -1;
  }
  return w5x00_socket_peek(c->sockindex);
}

/***************************************************************************//**
 * Ethernet Client Flush Tx Buffer.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_flush(w5x00_ethernet_client_t *c)
{
  while (c->sockindex < W5x00_MAX_SOCK_NUM) {
    uint8_t stat = w5x00_socket_status(c->sockindex);
    if ((stat != SnSR_ESTABLISHED) && (stat != SnSR_CLOSE_WAIT)) {
      return SL_STATUS_INVALID_STATE;
    }
    if (w5x00_socket_send_available(c->sockindex) >= w5x00_get_SSIZE()) {
      return SL_STATUS_OK;
    }
  }
  return SL_STATUS_FAIL;
}

/***************************************************************************//**
 * Ethernet Client Stop.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_stop(w5x00_ethernet_client_t *c)
{
  if (c == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  // attempt to close the connection gracefully (send a FIN to other side)
  w5x00_socket_disconnect(c->sockindex);
  unsigned long start = w5x00_get_tick_ms();

  // wait up to a second for the connection to close
  do {
    if (w5x00_socket_status(c->sockindex) == SnSR_CLOSED) {
      c->sockindex = W5x00_MAX_SOCK_NUM;
      return SL_STATUS_OK; // exit the loop
    }
    w5x00_delay_ms(1);
  } while (w5x00_get_tick_ms() - start < c->timeout);

  // if it hasn't closed, close it forcefully
  w5x00_socket_close(c->sockindex);
  c->sockindex = W5x00_MAX_SOCK_NUM;
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Client Is Connected.
 ******************************************************************************/
bool w5x00_ethernet_client_connected(w5x00_ethernet_client_t *c)
{
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }

  uint8_t s = w5x00_socket_status(c->sockindex);
  return !(s == SnSR_LISTEN
           || s == SnSR_CLOSED
           || s == SnSR_FIN_WAIT
           || (s == SnSR_CLOSE_WAIT && !w5x00_ethernet_client_available(c)));
}

/***************************************************************************//**
 * Ethernet Client Get Socket Status.
 ******************************************************************************/
uint8_t w5x00_ethernet_client_status(w5x00_ethernet_client_t *c)
{
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return SnSR_CLOSED;
  }
  return w5x00_socket_status(c->sockindex);
}

/***************************************************************************//**
 * Ethernet Client Get Local Port.
 ******************************************************************************/
uint16_t w5x00_ethernet_client_local_port(w5x00_ethernet_client_t *c)
{
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  uint16_t port;
  port = w5x00_readSnPORT(c->sockindex);
  return port;
}

/***************************************************************************//**
 * Ethernet Client Get Remote IP.
 ******************************************************************************/
w5x00_ip4_addr_t w5x00_ethernet_client_remote_ip(w5x00_ethernet_client_t *c)
{
  w5x00_ip4_addr_t ip = { WIZNET_IP4_DATA(127, 0, 0, 1) };
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return ip;
  }
  w5x00_readSnDIPR(c->sockindex, (uint8_t *)&ip);
  return ip;
}

/***************************************************************************//**
 * Ethernet Client Get Remote Port.
 ******************************************************************************/
uint16_t w5x00_ethernet_client_remote_port(w5x00_ethernet_client_t *c)
{
  if (c->sockindex >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  uint16_t port;
  port = w5x00_readSnDPORT(c->sockindex);
  return port;
}
