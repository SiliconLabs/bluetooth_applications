/***************************************************************************//**
 * @file ethernet_udp.h
 * @brief Wiznet Ethernet UDP protocol.
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

#ifndef ETHERNET_UDP_H_
#define ETHERNET_UDP_H_

#include <stddef.h>
#include <stdint.h>

#include "w5x00.h"
#include "w5x00_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup UDP UDP Client
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup UDP
 * @brief  Ethernet TCP client function.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    UDP object defination
 ******************************************************************************/
typedef struct {
  uint16_t port;                /// Local port to listen on
  w5x00_ip4_addr_t remote_ip;   /// Remote IP address for the incoming packet whilst it's being processed
  uint16_t remote_port;         /// Remote port for the incoming packet whilst it's being processed
  uint16_t offset;              /// Offset into the packet being sent

  w5x00_socket_t sockindex;     /// Socket index
  uint16_t remaining;           /// Remaining bytes of incoming packet yet to be processed
}w5x00_ethernet_udp_t;

sl_status_t ethernet_udp_init(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Start EthernetUDP socket, listening at local port PORT
 * @param e_udp
 *    UDP instance
 * @param port
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_udp_begin(w5x00_ethernet_udp_t *e_udp,
                                     uint16_t port);

/***************************************************************************//**
 * @brief
 *    Return number of bytes available in the current packet,
 *    will return zero if parsePacket hasn't been called yet
 * @param[in] e_udp
 *    UDP instance
 * @return
 *    Number of bytes available in the current packet
 ******************************************************************************/
int w5x00_ethernet_udp_available(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Release any resources being used by this EthernetUDP instance
 * @param[in] e_udp
 *    UDP instance
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_udp_stop(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Start UDP client
 * @param[in] e_udp
 *    UDP instance
 * @param[in] ip
 *    Remote ip
 * @param[in] port
 *    Remote port
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_udp_begin_packet(w5x00_ethernet_udp_t *e_udp,
                                            w5x00_ip4_addr_t *ip,
                                            uint16_t port);

/***************************************************************************//**
 * @brief
 *    Start sending packet data
 * @param[in] e_udp
 *    UDP instance
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_udp_end_packet(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Start receive packet data
 * @param[in] e_udp
 *    UDP instance.
 * @return
 *    Remaining bytes are the data after the header.
 ******************************************************************************/
int w5x00_ethernet_udp_parse_packet(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Read 1 byte from receive buffer.
 * @param[in] e_udp
 *    UDP instance.
 * @return
 *    1 byte data on success
 *    -1 on failure
 ******************************************************************************/
int w5x00_ethernet_udp_read_byte(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Write data to socket buffer
 * @param[in] e
 *    UDP instance.
 * @param[in] buffer
 *    Pointer to buffer to be written.
 * @param[in] size
 *    Size of the buffer
 * @return
 *    Size of the written bytes on success
 *    -1 on failure
 ******************************************************************************/
int w5x00_ethernet_udp_write(w5x00_ethernet_udp_t *e,
                             const uint8_t *buffer,
                             size_t size);

/***************************************************************************//**
 * @brief
 *    Read data from receive queue
 * @param[in] e
 *    UDP instance.
 * @param[out] buffer
 *    Pointer to the buffer to be read
 * @param[in] len
 *    Length of the buffer
 * @return
 *    Number of read bytes on success
 *    -1 on failure
 ******************************************************************************/
int w5x00_ethernet_udp_read(w5x00_ethernet_udp_t *e_udp,
                            uint8_t *buffer,
                            size_t len);

/***************************************************************************//**
 * @brief
 *    Get the first byte in the receive queue
 * @param[in] e_udp
 *    UDP instance.
 * @return
 *    1 byte data on success
 *    -1 on failure
 ******************************************************************************/
int w5x00_ethernet_udp_peek(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Flush the Tx buffer
 * @param[in] e_udp
 *    UDP instance.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_udp_flush(w5x00_ethernet_udp_t *e_udp);

/***************************************************************************//**
 * @brief
 *    Start Multicast EthernetUDP socket, listening at local port PORT
 * @param[in] e_udp
 *    UDP instance.
 * @param[in] ip
 *    Multicast address in IPv4 format
 * @param[in] port
 *    Port of the listening target
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_udp_begin_multicast(w5x00_ethernet_udp_t *e_udp,
                                               w5x00_ip4_addr_t ip,
                                               uint16_t port);

/** @} (end group UDP) */
#ifdef __cplusplus
}
#endif
#endif // ETHERNET_UDP_H_
