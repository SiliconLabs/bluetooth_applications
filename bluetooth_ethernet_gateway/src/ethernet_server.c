/***************************************************************************//**
 * @file ethernet_server.c
 * @brief Wiznet Ethernet TCP Server.
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
#include "socket.h"
#include "ethernet.h"
#include "ethernet_client.h"
#include "ethernet_server.h"

/***************************************************************************//**
 * Ethernet Server Init.
 ******************************************************************************/
sl_status_t w5x00_ethernet_server_init(w5x00_ethernet_server_t *ss,
                                       uint16_t port)
{
  if (ss == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  ss->port = port;
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Ethernet Server Begin.
 ******************************************************************************/
sl_status_t w5x00_ethernet_server_begin(w5x00_ethernet_server_t *ss)
{
  if (ss == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  uint8_t sockindex = w5x00_socket_begin(SnMR_TCP, ss->port);
  if (sockindex < W5x00_MAX_SOCK_NUM) {
    if (w5x00_socket_listen(sockindex)) {
      ss->server_port[sockindex] = ss->port;
    } else {
      w5x00_socket_disconnect(sockindex);
    }
    return SL_STATUS_OK;
  }
  return SL_STATUS_FAIL;
}

/***************************************************************************//**
 * Ethernet Server Accept Client.
 ******************************************************************************/
w5x00_ethernet_client_t w5x00_ethernet_server_accept(w5x00_ethernet_server_t *ss)
{
  bool listening = false;
  uint8_t sockindex = W5x00_MAX_SOCK_NUM;
  enum W5x00Chip chip;
  uint8_t maxindex = W5x00_MAX_SOCK_NUM;
  w5x00_ethernet_client_t client;

  client.sockindex = W5x00_MAX_SOCK_NUM;
  if (ss == NULL) {
    return client;
  }
  chip = w5x00_get_chip();
  if (chip == W5x00_UNKNOWN) {
    return client;
  }
#if W5x00_MAX_SOCK_NUM > 4
  if (chip == W5x00_W5100) {
    maxindex = 4; // W5100 chip never supports more than 4 sockets
  }
#endif
  for (uint8_t i = 0; i < maxindex; i++) {
    if (ss->server_port[i] == ss->port) {
      uint8_t stat = w5x00_socket_status(i);
      if ((sockindex == W5x00_MAX_SOCK_NUM)
          && ((stat == SnSR_ESTABLISHED)
              || (stat == SnSR_CLOSE_WAIT))) {
        // Return the connected client even if no data received.
        // Some protocols like FTP expect the server to send the
        // first data.
        sockindex = i;
        ss->server_port[i] = 0; // only return the client once
      } else if (stat == SnSR_LISTEN) {
        listening = true;
      } else if (stat == SnSR_CLOSED) {
        ss->server_port[i] = 0;
      }
    }
  }
  if (!listening) {
    w5x00_ethernet_server_begin(ss);
  }
  client.sockindex = sockindex;
  return client;
}

/***************************************************************************//**
 * Ethernet Server Write.
 ******************************************************************************/
int w5x00_ethernet_server_write(w5x00_ethernet_server_t *ss,
                                const uint8_t *buffer,
                                size_t size)
{
  enum W5x00Chip chip;
  uint8_t maxindex = W5x00_MAX_SOCK_NUM;

  if (ss == NULL) {
    return -1;
  }
  chip = w5x00_get_chip();
  if (chip == W5x00_UNKNOWN) {
    return 0;
  }
#if W5x00_MAX_SOCK_NUM > 4
  if (chip == W5x00_W5100) {
    maxindex = 4; // W5100 chip never supports more than 4 sockets
  }
#endif
  w5x00_ethernet_server_accept(ss);
  for (uint8_t i = 0; i < maxindex; i++) {
    if (ss->server_port[i] == ss->port) {
      if (w5x00_socket_status(i) == SnSR_ESTABLISHED) {
        w5x00_socket_send(i, buffer, size);
      }
    }
  }
  return size;
}
