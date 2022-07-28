/***************************************************************************//**
 * @file socket.h
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
#ifndef SOCKET_H_
#define SOCKET_H_
#include <stdbool.h>
#include "w5x00.h"
#include "w5x00_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup Socket Socket
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup Socket
 * @brief  Wiznet Ethernet Socket.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    Initialize random local port for socket
 * @param[in] n
 *    Input random number
 ******************************************************************************/
void w5x00_socket_port_rand(uint16_t n);

/***************************************************************************//**
 * @brief
 *    Initialize socket
 * @param[in] s
 *    Socket number
 * @param[in] protocol
 *    #SnMR value enum
 * @param[in] port
 *    Remote port
 * @return
 *    Socket number
 *      < @ref W5x00_MAX_SOCK_NUM on success
 *      = @ref W5x00_MAX_SOCK_NUM on failure
 ******************************************************************************/
w5x00_socket_t w5x00_socket_init(w5x00_socket_t s,
                                 uint8_t protocol,
                                 uint16_t port);

/***************************************************************************//**
 * @brief
 *    Initialize multicast socket
 * @param[in] s
 *    Socket number
 * @param[in] ip
 *    Remote IP
 * @param[in] protocol
 *    #SnMR value enum
 * @param[in] port
 *    Remote port
 * @return
 *    Socket number
 *      < @ref W5x00_MAX_SOCK_NUM on success
 *      = @ref W5x00_MAX_SOCK_NUM on failure
 ******************************************************************************/
w5x00_socket_t w5x00_socket_init_multicast(w5x00_socket_t s,
                                           w5x00_ip4_addr_t ip,
                                           uint8_t protocol,
                                           uint16_t port);

/***************************************************************************//**
 * @brief
 *    Start a socket
 * @param[in] protocol
 *    #SnMR value enum
 * @param[in] port
 *    Remote port
 * @return
 *    Socket number
 *      < @ref W5x00_MAX_SOCK_NUM on success
 *      = @ref W5x00_MAX_SOCK_NUM on failure
 ******************************************************************************/
w5x00_socket_t w5x00_socket_begin(uint8_t protocol, uint16_t port);

/***************************************************************************//**
 * @brief
 *    Start a multicast socket
 * @param[in] protocol
 *    #SnMR value enum
 * @param[in] ip
 *    Remote IP
 * @param[in] port
 *    Remote port
 * @return
 *    Socket number
 *      < @ref W5x00_MAX_SOCK_NUM on success
 *      = @ref W5x00_MAX_SOCK_NUM on failure
 ******************************************************************************/
w5x00_socket_t w5x00_socket_begin_multicast(uint8_t protocol,
                                            w5x00_ip4_addr_t ip,
                                            uint16_t port);
/***************************************************************************//**
 * @brief
 *    Get the socket status
 * @param[in] s
 *    Socket number
 * @return
 *    returns an #SnSR value enum
 ******************************************************************************/
uint8_t w5x00_socket_status(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    Get socket tx buffer max size
 * @param[in] s
 *    Socket number
 * @return
 *    Socket tx buffer max size on success, 0 on failure
 ******************************************************************************/
uint16_t w5x00_socket_get_tx_max_size(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    Get socket tx buffer free size
 * @param[in] s
 *    Socket number
 ******************************************************************************/
#define w5x00_socket_get_tx_free_size(s)  \
        w5x00_readSnTX_FSR(s)

/***************************************************************************//**
 * @brief
 *    Immediately close.  If a TCP connection is established,
 *    the remote host is left unaware we closed
 * @param[in] s
 *    Socket number
 ******************************************************************************/
void w5x00_socket_close(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    Place the socket in listening (server) mode
 * @param[in] s
 *    Socket number
 * @return
 ******************************************************************************/
sl_status_t w5x00_socket_listen(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    Establish a TCP connection in Active (client) mode.
 * @param[in] s
 *    Socket number
 * @param[in] ip
 *    Remote IP
 * @param[in] port
 *    Remote port
 ******************************************************************************/
void w5x00_socket_connect(w5x00_socket_t s,
                          w5x00_ip4_addr_t *ip,
                          uint16_t port);

/***************************************************************************//**
 * @brief
 *    Gracefully disconnect a TCP connection.
 * @param[in] s
 *    Socket number
 ******************************************************************************/
void w5x00_socket_disconnect(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    Receive data.
 * @param[in] s
 *    Socket number
 * @param[out] buf
 *    Pointer to receive buffer
 * @param[in] len
 *    Size of receive buffer
 * @return
 *    Returns size, or -1 for no data, or 0 if connection closed
 ******************************************************************************/
int w5x00_socket_recv(w5x00_socket_t s, uint8_t *buf, int16_t len);

/***************************************************************************//**
 * @brief
 *    Get available data in socket receive queue
 * @param[in] s
 *    Socket number
 * @return
 *    > 0 size of data
 *    = 0 if no data or error
 ******************************************************************************/
uint16_t w5x00_socket_recv_available(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    Get the first byte in the receive queue (no checking)
 * @param[in] s
 * @return
 *    The first byte in the receive queue
 ******************************************************************************/
uint8_t w5x00_socket_peek(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    This function used to send the data in TCP mode
 * @param[in] s
 *    Socket number
 * @param[in] buf
 *    Pointer to send buffer
 * @param[in] len
 *    Size of send buffer
 * @return
 *    Size of the sending data
 ******************************************************************************/
uint16_t w5x00_socket_send(w5x00_socket_t s,
                           const uint8_t * buf,
                           uint16_t len);
/***************************************************************************//**
 * @brief
 *    Get available size of socket send queue
 * @param[in] s
 *    Socket number
 * @return
 *    Size of send queue
 ******************************************************************************/
uint16_t w5x00_socket_send_available(w5x00_socket_t s);

/***************************************************************************//**
 * @brief
 *    Write data to socket buffer
 * @param[in] s
 *    Socket number
 * @param[in] offset
 *    Offset of socket send buffer
 * @param[in] buf
 *    Pointer of the send data
 * @param[in] len
 *    Size of the send data
 * @return
 *    Size of written data
 ******************************************************************************/
uint16_t w5x00_socket_buffer_data(w5x00_socket_t s,
                                  uint16_t offset,
                                  const uint8_t* buf,
                                  uint16_t len);

/***************************************************************************//**
 * @brief
 *    Start a udp socket
 * @param[in] s
 *    Socket number
 * @param[in] ip
 *    Remote ip
 * @param[in] port
 *    Remote port
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_socket_begin_udp(w5x00_socket_t s,
                                   w5x00_ip4_addr_t *ip,
                                   uint16_t port);

/***************************************************************************//**
 * @brief
 *    Start send data for UDP socket
 * @param[in] s
 *    Socket number
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_socket_send_udp(w5x00_socket_t s);

/** @} (end group Socket) */
#ifdef __cplusplus
}
#endif
#endif /* SOCKET_H_ */
