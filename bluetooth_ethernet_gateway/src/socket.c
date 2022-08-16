/***************************************************************************//**
 * @file socket.c
 * @brief Wiznet Socket.
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
#include <stdbool.h>
#include "sl_status.h"

#include "w5x00.h"
#include "ethernet.h"
#include "socket.h"

#define yield()

static uint16_t local_port = 49152;  // 49152 to 65535

typedef struct {
  uint16_t RX_RSR; // Number of bytes received
  uint16_t RX_RD;  // Address to read
  uint16_t TX_FSR; // Free space ready for transmit
  uint8_t  RX_inc; // how much have we advanced RX_RD
} socketstate_t;

static socketstate_t state[W5x00_MAX_SOCK_NUM];

static uint16_t getSnTX_FSR(w5x00_socket_t s);
static uint16_t getSnRX_RSR(w5x00_socket_t s);
static void write_data(w5x00_socket_t s,
                       uint16_t offset,
                       const uint8_t *data,
                       uint16_t len);
static void read_data(w5x00_socket_t s,
                      uint16_t src,
                      uint8_t *dst,
                      uint16_t len);



/*****************************************/
/*          Socket management            */
/*****************************************/

/***************************************************************************//**
 * Socket Port Rand.
 ******************************************************************************/
void w5x00_socket_port_rand(uint16_t n)
{
  n &= 0x3FFF;
  local_port ^= n;
  // w5x00_log_info("Local port: %d, srcport=%d\n", n, local_port);
}

/***************************************************************************//**
 * Init Socket.
 ******************************************************************************/
w5x00_socket_t w5x00_socket_init(w5x00_socket_t s,
                                 uint8_t protocol,
                                 uint16_t port)
{
  if ((protocol != SnMR_TCP)
      && (protocol != SnMR_UDP)) {
    return W5x00_MAX_SOCK_NUM;
  }
  if (s >= W5x00_MAX_SOCK_NUM) {
    return W5x00_MAX_SOCK_NUM;
  }
  if (SnSR_CLOSED != w5x00_readSnSR(s)) {
    return W5x00_MAX_SOCK_NUM;
  }
  //  EthernetServer::server_port[s] = 0;
  w5x00_delay_us(250); // TODO: is this needed??
  w5x00_writeSnMR(s, protocol);
  w5x00_writeSnIR(s, 0xFF);
  if (port > 0) {
    w5x00_writeSnPORT(s, port);
  } else {
    // if don't set the source port, set local_port number.
    if (++local_port < 49152) {
      local_port = 49152;
    }
    w5x00_writeSnPORT(s, local_port);
  }
  w5x00_exec_cmd_socket(s, Sock_OPEN);
  state[s].RX_RSR = 0;
  state[s].RX_RD = w5x00_readSnRX_RD(s);  // always zero?
  state[s].RX_inc = 0;
  state[s].TX_FSR = 0;
  // w5x00_log_info("W5000socket prot=%d, RX_RD=%d\n", w5x00_readSnMR(s), state[s].RX_RD);
  return s;
}

/***************************************************************************//**
 * Init Multicast Socket.
 ******************************************************************************/
w5x00_socket_t w5x00_socket_init_multicast(w5x00_socket_t s,
                                           w5x00_ip4_addr_t ip,
                                           uint8_t protocol,
                                           uint16_t port)
{
  if ((protocol != SnMR_TCP)
      && (protocol != SnMR_UDP)) {
    return W5x00_MAX_SOCK_NUM;
  }
  if (s >= W5x00_MAX_SOCK_NUM) {
    return W5x00_MAX_SOCK_NUM;
  }
  if (SnSR_CLOSED != w5x00_readSnSR(s)) {
    return W5x00_MAX_SOCK_NUM;
  }

  // w5x00_log_info("W5000socket %d\n", s);
  w5x00_delay_us(250);
  w5x00_writeSnMR(s, protocol);
  w5x00_writeSnIR(s, 0xFF);
  if (port > 0) {
    w5x00_writeSnPORT(s, port);
  } else {
    // if don't set the source port, set local_port number.
    if (++local_port < 49152) {
      local_port = 49152;
    }
    w5x00_writeSnPORT(s, local_port);
  }
  // Calculate MAC address from Multicast IP Address
  uint8_t mac[] = {  0x01, 0x00, 0x5E, 0x00, 0x00, 0x00 };
  mac[3] = w5x00_ip4_addr_get_byte(&ip, 1) & 0x7F;
  mac[4] = w5x00_ip4_addr_get_byte(&ip, 2);
  mac[5] = w5x00_ip4_addr_get_byte(&ip, 3);
  w5x00_writeSnDIPR(s, (uint8_t *)&ip.addr);   // 239.255.0.1
  w5x00_writeSnDPORT(s, port);
  w5x00_writeSnDHAR(s, mac);
  w5x00_exec_cmd_socket(s, Sock_OPEN);
  state[s].RX_RSR = 0;
  state[s].RX_RD = w5x00_readSnRX_RD(s);  // always zero?
  state[s].RX_inc = 0;
  state[s].TX_FSR = 0;
  return s;
}

/***************************************************************************//**
 * Socket Begin.
 ******************************************************************************/
w5x00_socket_t w5x00_socket_begin(uint8_t protocol, uint16_t port)
{
  w5x00_socket_t s;
  uint8_t status[W5x00_MAX_SOCK_NUM];
  uint8_t maxindex = W5x00_MAX_SOCK_NUM;

  // first check hardware compatibility
  enum W5x00Chip chip = w5x00_get_chip();
  if (chip == W5x00_UNKNOWN) {
    return W5x00_MAX_SOCK_NUM; // immediate error if no hardware detected
  }
#if W5x00_MAX_SOCK_NUM > 4
  if (chip == W5x00_W5100) {
    maxindex = 4; // W5100 chip never supports more than 4 sockets
  }
#endif
  // look at all the hardware sockets, use any that are closed (unused)
  for (s = 0; s < maxindex; s++) {
    status[s] = w5x00_readSnSR(s);
    if (status[s] == SnSR_CLOSED) {
      goto makesocket;
    }
  }
  // w5x00_log_info("W5000socket step2\n");
  // as a last resort, forcibly close any already closing
  for (s = 0; s < maxindex; s++) {
    uint8_t stat = status[s];
    if (stat == SnSR_LAST_ACK) {
      goto closemakesocket;
    }
    if (stat == SnSR_TIME_WAIT) {
      goto closemakesocket;
    }
    if (stat == SnSR_FIN_WAIT) {
      goto closemakesocket;
    }
    if (stat == SnSR_CLOSING) {
      goto closemakesocket;
    }
  }

#if 0

  w5x00_log_info("W5000socket step3\n");
  // next, use any that are effectively closed
  for (s = 0; s < W5x00_MAX_SOCK_NUM; s++) {
    uint8_t stat = status[s];
    // TODO: this also needs to check if no more data
    if (stat == SnSR_CLOSE_WAIT) {
      goto closemakesocket;
    }
  }

#endif

  return W5x00_MAX_SOCK_NUM; // all sockets are in use
closemakesocket:
  // w5x00_log_info("W5000socket close\n");
  w5x00_exec_cmd_socket(s, Sock_CLOSE);
makesocket:
  return w5x00_socket_init(s, protocol, port);
}

/***************************************************************************//**
 * Socket Begin Multicast.
 ******************************************************************************/
w5x00_socket_t w5x00_socket_begin_multicast(uint8_t protocol,
                                            w5x00_ip4_addr_t ip,
                                            uint16_t port)
{
  w5x00_socket_t s;
  uint8_t status[W5x00_MAX_SOCK_NUM];
  uint8_t maxindex = W5x00_MAX_SOCK_NUM;

  // first check hardware compatibility
  enum W5x00Chip chip = w5x00_get_chip();
  if (chip == W5x00_UNKNOWN) {
    return W5x00_MAX_SOCK_NUM; // immediate error if no hardware detected
  }
#if W5x00_MAX_SOCK_NUM > 4
  if (chip == W5x00_W5100) {
    maxindex = 4; // W5100 chip never supports more than 4 sockets
  }
#endif
  // w5x00_log_info("W5000socket begin, protocol=%d, port=%d\n", protocol, port);
  // look at all the hardware sockets, use any that are closed (unused)
  for (s = 0; s < maxindex; s++) {
    status[s] = w5x00_readSnSR(s);
    if (status[s] == SnSR_CLOSED) {
      goto makesocket;
    }
  }
  // as a last resort, forcibly close any already closing
  for (s = 0; s < maxindex; s++) {
    uint8_t stat = status[s];
    if (stat == SnSR_LAST_ACK) {
      goto closemakesocket;
    }
    if (stat == SnSR_TIME_WAIT) {
      goto closemakesocket;
    }
    if (stat == SnSR_FIN_WAIT) {
      goto closemakesocket;
    }
    if (stat == SnSR_CLOSING) {
      goto closemakesocket;
    }
  }

#if 0

  w5x00_log_info("W5000socket step3\n");
  // next, use any that are effectively closed
  for (s = 0; s < W5x00_MAX_SOCK_NUM; s++) {
    uint8_t stat = status[s];
    // TODO: this also needs to check if no more data
    if (stat == SnSR_CLOSE_WAIT) {
      goto closemakesocket;
    }
  }

#endif

  return W5x00_MAX_SOCK_NUM; // all sockets are in use
closemakesocket:
  // w5x00_log_info("W5000socket close\n");
  w5x00_exec_cmd_socket(s, Sock_CLOSE);
makesocket:
  return w5x00_socket_init_multicast(s, ip, protocol, port);
}

/***************************************************************************//**
 * Socket Status.
 ******************************************************************************/
uint8_t w5x00_socket_status(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return SnSR_CLOSED;
  }
  uint8_t status = w5x00_readSnSR(s);
  return status;
}

/***************************************************************************//**
 * Socket Get TX Max Size.
 ******************************************************************************/
uint16_t w5x00_socket_get_tx_max_size(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  enum W5x00Chip chip = w5x00_get_chip();
  if ((chip == W5x00_UNKNOWN)
      || (chip == W5x00_W5100)
      || (s >= W5x00_MAX_SOCK_NUM)) {
    return 0;
  }
  return ((uint16_t)w5x00_readSnTX_SIZE(s)) << 10;
}

/***************************************************************************//**
 * Socket Close.
 ******************************************************************************/
void w5x00_socket_close(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return;
  }
  w5x00_exec_cmd_socket(s, Sock_CLOSE);
}

/***************************************************************************//**
 * Socket Close.
 ******************************************************************************/
sl_status_t w5x00_socket_listen(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (w5x00_readSnSR(s) != SnSR_INIT) {
    return SL_STATUS_FAIL;
  }
  w5x00_exec_cmd_socket(s, Sock_LISTEN);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Socket Connect.
 ******************************************************************************/
void w5x00_socket_connect(w5x00_socket_t s, w5x00_ip4_addr_t *ip, uint16_t port)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return;
  }
  // set destination IP
  w5x00_writeSnDIPR(s, (uint8_t *)ip);
  w5x00_writeSnDPORT(s, port);
  w5x00_exec_cmd_socket(s, Sock_CONNECT);
}

/***************************************************************************//**
 * Socket Disconnect.
 ******************************************************************************/
void w5x00_socket_disconnect(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return;
  }
  w5x00_exec_cmd_socket(s, Sock_DISCON);
}

/*****************************************/
/*    Socket Data Receive Functions      */
/*****************************************/
static uint16_t getSnRX_RSR(w5x00_socket_t s)
{
#if 1
  uint16_t val, prev;

  prev = w5x00_readSnRX_RSR(s);
  while (1) {
    val = w5x00_readSnRX_RSR(s);
    if (val == prev) {
      return val;
    }
    prev = val;
  }
#else
  uint16_t val = w5x00_readSnRX_RSR(s);
  return val;
#endif
}

static void read_data(w5x00_socket_t s,
                      uint16_t src,
                      uint8_t *dst,
                      uint16_t len)
{
  uint16_t size;
  uint16_t src_mask;
  uint16_t src_ptr;

  // w5x00_log_info("read_data, len=%d, at:%d\n", len, src);
  src_mask = (uint16_t)src & w5x00_get_SMASK();
  src_ptr = w5x00_get_RBASE(s) + src_mask;

  if (w5x00_has_offset_address_mapping()
      || ((src_mask + len) <= w5x00_get_SSIZE())) {
    w5x00_read(src_ptr, dst, len);
  } else {
    size = w5x00_get_SSIZE() - src_mask;
    w5x00_read(src_ptr, dst, size);
    dst += size;
    w5x00_read(w5x00_get_RBASE(s), dst, len - size);
  }
}

/***************************************************************************//**
 * Socket Receive.
 ******************************************************************************/
int w5x00_socket_recv(w5x00_socket_t s, uint8_t *buf, int16_t len)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return -1;
  }
  // Check how much data is available
  int ret = state[s].RX_RSR;
  if (ret < len) {
    uint16_t rsr = getSnRX_RSR(s);
    ret = rsr - state[s].RX_inc;
    state[s].RX_RSR = ret;
    // w5x00_log_info("Sock_RECV, RX_RSR=%d, RX_inc=%d\n", ret, state[s].RX_inc);
  }
  if (ret == 0) {
    // No data available.
    uint8_t status = w5x00_readSnSR(s);
    if ((status == SnSR_LISTEN)
        || (status == SnSR_CLOSED)
        || (status == SnSR_CLOSE_WAIT)) {
      // The remote end has closed its side of the connection,
      // so this is the eof state
      ret = 0;
    } else {
      // The connection is still up, but there's no data waiting to be read
      ret = -1;
    }
  } else {
    if (ret > len) {
      ret = len;              // more data available than buffer length
    }
    uint16_t ptr = state[s].RX_RD;
    if (buf) {
      read_data(s, ptr, buf, ret);
    }
    ptr += ret;
    state[s].RX_RD = ptr;
    state[s].RX_RSR -= ret;
    uint16_t inc = state[s].RX_inc + ret;
    if ((inc >= 250) || (state[s].RX_RSR == 0)) {
      state[s].RX_inc = 0;
      w5x00_writeSnRX_RD(s, ptr);
      w5x00_exec_cmd_socket(s, Sock_RECV);
      // w5x00_log_info("Sock_RECV cmd, RX_RD=%d, RX_RSR=%d\n",
      //                state[s].RX_RD, state[s].RX_RSR);
    } else {
      state[s].RX_inc = inc;
    }
  }
  // w5x00_log_info("socketRecv, ret=%d\n", ret);

  return ret;
}

/***************************************************************************//**
 * Socket Receive Data Available.
 ******************************************************************************/
uint16_t w5x00_socket_recv_available(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  uint16_t ret = state[s].RX_RSR;
  if (ret == 0) {
    uint16_t rsr = getSnRX_RSR(s);
    ret = rsr - state[s].RX_inc;
    state[s].RX_RSR = ret;
    // w5x00_log_info("sockRecvAvailable s=%d, RX_RSR=%d\n", s, ret);
  }
  return ret;
}

/***************************************************************************//**
 * Socket Peek.
 ******************************************************************************/
uint8_t w5x00_socket_peek(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  uint16_t ptr = state[s].RX_RD;
  uint8_t b;
  w5x00_read((ptr & w5x00_get_SMASK()) + w5x00_get_SBASE(s),
               &b,
               1);
  return b;
}


/***************************************************************************//**
 * Socket Data Transmit Functions.
 ******************************************************************************/
static uint16_t getSnTX_FSR(w5x00_socket_t s)
{
  uint16_t val, prev;

  prev = w5x00_readSnTX_FSR(s);
  while (1) {
    val = w5x00_readSnTX_FSR(s);
    if (val == prev) {
      state[s].TX_FSR = val;
      return val;
    }
    prev = val;
  }
}

static void write_data(w5x00_socket_t s,
                       uint16_t data_offset,
                       const uint8_t *data,
                       uint16_t len)
{
  uint16_t ptr = w5x00_readSnTX_WR(s);
  ptr += data_offset;
  uint16_t offset = ptr & w5x00_get_SMASK();
  uint16_t dstAddr = offset + w5x00_get_SBASE(s);

  if (w5x00_has_offset_address_mapping()
      || ((offset + len) <= w5x00_get_SSIZE())) {
    w5x00_write(dstAddr, data, len);
  } else {
    // Wrap around circular buffer
    uint16_t size = w5x00_get_SSIZE() - offset;
    w5x00_write(dstAddr, data, size);
    w5x00_write(w5x00_get_SBASE(s), data + size, len - size);
  }
  ptr += len;
  w5x00_writeSnTX_WR(s, ptr);
}

/***************************************************************************//**
 * Socket Send.
 ******************************************************************************/
uint16_t w5x00_socket_send(w5x00_socket_t s,
                            const uint8_t * buf,
                            uint16_t len)
{
  uint8_t status = 0;
  uint16_t ret = 0;
  uint16_t freesize = 0;

  if (s >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }

  if (len > w5x00_get_SSIZE()) {
    ret = w5x00_get_SSIZE(); // check size not to exceed MAX size.
  } else {
    ret = len;
  }

  // if freebuf is available, start.
  do {
    freesize = getSnTX_FSR(s);
    status = w5x00_readSnSR(s);
    if ((status != SnSR_ESTABLISHED) && (status != SnSR_CLOSE_WAIT)) {
      ret = 0;
      break;
    }
    yield();
  } while (freesize < ret);

  // copy data
  write_data(s, 0, (uint8_t *)buf, ret);
  w5x00_exec_cmd_socket(s, Sock_SEND);

  /* +2008.01 bj */
  while ((w5x00_readSnIR(s) & SnIR_SEND_OK) != SnIR_SEND_OK) {
    /* m2008.01 [bj] : reduce code */
    if (w5x00_readSnSR(s) == SnSR_CLOSED) {
      return 0;
    }
    yield();
  }

  /* +2008.01 bj */
  w5x00_writeSnIR(s, SnIR_SEND_OK);
  return ret;
}

/***************************************************************************//**
 * Socket Send Buffer Available.
 ******************************************************************************/
uint16_t w5x00_socket_send_available(w5x00_socket_t s)
{
  uint8_t status = 0;
  uint16_t freesize = 0;

  if (s >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }
  
  freesize = getSnTX_FSR(s);
  status = w5x00_readSnSR(s);
  if ((status == SnSR_ESTABLISHED)
      || (status == SnSR_CLOSE_WAIT)) {
    return freesize;
  }
  return 0;
}

/***************************************************************************//**
 * Socket Buffer Data.
 ******************************************************************************/
uint16_t w5x00_socket_buffer_data(w5x00_socket_t s,
                                  uint16_t offset,
                                  const uint8_t* buf,
                                  uint16_t len)
{
  uint16_t ret =0;
  uint16_t txfree = getSnTX_FSR(s);

  if (s >= W5x00_MAX_SOCK_NUM) {
    return 0;
  }

  if (len > txfree) {
    ret = txfree; // check size not to exceed MAX size.
  } else {
    ret = len;
  }
  write_data(s, offset, buf, ret);
  return ret;
}

/***************************************************************************//**
 * Socket Begin UDP.
 ******************************************************************************/
sl_status_t w5x00_socket_begin_udp(w5x00_socket_t s,
                                   w5x00_ip4_addr_t *ip,
                                   uint16_t port)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if ((ip->addr == WIZNET_IPADDR_ANY)
      || ((port == 0x00))) {
    return SL_STATUS_FAIL;
  }
  w5x00_writeSnDIPR(s, (uint8_t *)&ip->addr);
  w5x00_writeSnDPORT(s, port);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Socket Send UDP.
 ******************************************************************************/
sl_status_t w5x00_socket_send_udp(w5x00_socket_t s)
{
  if (s >= W5x00_MAX_SOCK_NUM) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  w5x00_exec_cmd_socket(s, Sock_SEND);

  // +2008.01 bj
  while ((w5x00_readSnIR(s) & SnIR_SEND_OK) != SnIR_SEND_OK) {
    if (w5x00_readSnIR(s) & SnIR_TIMEOUT) {
      // +2008.01 [bj]: clear interrupt
      w5x00_writeSnIR(s, (SnIR_SEND_OK | SnIR_TIMEOUT));
      return SL_STATUS_FAIL;
    }
    yield();
  }

  // +2008.01 bj
  w5x00_writeSnIR(s, SnIR_SEND_OK);

  // Sent ok
  return SL_STATUS_OK;
}
