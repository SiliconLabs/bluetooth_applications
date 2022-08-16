/***************************************************************************//**
 * @file ethernet_client.h
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

#ifndef ETHERNET_CLIENT_H_
#define ETHERNET_CLIENT_H_

#include <stdint.h>
#include "w5x00.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup TCP_Client TCP Client
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup TCP_Client
 * @brief  Ethernet TCP client function.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    TCP client object definition
 ******************************************************************************/
typedef struct {
  w5x00_ethernet_t *eth;    /// Ethernet object reference
  w5x00_socket_t sockindex; /// Socket number,
                            ///   @ref W5x00_MAX_SOCK_NUM means client not in use
  uint16_t timeout;         /// Timeout of connection
} w5x00_ethernet_client_t;

/***************************************************************************//**
 * @brief
 *    Initialize ethernet client instance.
 * @param[in] c
 *    Ethernet client instance.
 * @param[in] eth
 *    Ethernet instance.
 * @param[in] timeout
 *    Connection timeout.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_init(w5x00_ethernet_client_t *c,
                                       w5x00_ethernet_t *eth,
                                       uint16_t timeout);

/***************************************************************************//**
 * @brief
 *    Connect to host.
 * @param[in] c
 *    Ethernet client instance.
 * @param[in] host
 *    Server hostname.
 * @param[in] port
 *    Server port.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_connect_host(w5x00_ethernet_client_t *c,
                                               const char *host,
                                               uint16_t port);

/***************************************************************************//**
 * @brief
 *    Connect to TCP server IP.
 * @param[in] c
 *    Ethernet client instance.
 * @param[in] ip
 *    Server IP.
 * @param[in] port
 *    Server port.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_connect(w5x00_ethernet_client_t *c,
                                          w5x00_ip4_addr_t ip,
                                          uint16_t port);

/***************************************************************************//**
 * @brief
 *    Get available Tx buffer size.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 *    >=0 Available Tx buffer size.
 *    <0 Error is occurred
 ******************************************************************************/
int w5x00_ethernet_client_available_for_write(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Client write data.
 * @param[in] c
 *    Ethernet client instance.
 * @param[in] buf
 *    Pointer of the input buffer.
 * @param[in] size
 *    Size of the data.
 * @return
 *    >=0 Size of the written data.
 *    <0 Error is occurred
 ******************************************************************************/
int w5x00_ethernet_client_write(w5x00_ethernet_client_t *c,
                                const uint8_t *buf,
                                size_t size);

/***************************************************************************//**
 * @brief
 *    Get client read buffer size.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 *    >=0 Size of current read buffer.
 *    <0 Error is occurred
 ******************************************************************************/
int w5x00_ethernet_client_available(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Read data from socket buffer.
 * @param[in] c
 *    Ethernet client instance.
 * @param[out] buf
 *    Pointer of the output buffer.
 * @param[in] size
 *    Size of output buffer.
 * @return
 *    >=0 Size of read data.
 *    <0  Error is occurred when reading data.
 ******************************************************************************/
int w5x00_ethernet_client_read(w5x00_ethernet_client_t *c,
                               uint8_t *buf,
                               size_t size);
/***************************************************************************//**
 * @brief
 *    Get the first byte in the receive queue
 * @param[in] c
 *    Ethernet client instance
 * @return
 *    >=0 First byte in the receive queue
 *    <0  Error is occurred when reading data
 ******************************************************************************/
int w5x00_ethernet_client_peek(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Flush Tx buffer.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_flush(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Stop ethernet client.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_client_stop(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Check client is connected.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 ******************************************************************************/
bool w5x00_ethernet_client_connected(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Get socket status.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 ******************************************************************************/
uint8_t w5x00_ethernet_client_status(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Get client local port.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 *    Client local port.
 ******************************************************************************/
uint16_t w5x00_ethernet_client_local_port(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Get client remote IP.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 *    Client remote IP.
 ******************************************************************************/
w5x00_ip4_addr_t w5x00_ethernet_client_remote_ip(w5x00_ethernet_client_t *c);

/***************************************************************************//**
 * @brief
 *    Get client remote port.
 * @param[in] c
 *    Ethernet client instance.
 * @return
 *    Client remote port.
 ******************************************************************************/
uint16_t w5x00_ethernet_client_remote_port(w5x00_ethernet_client_t *c);

/** @} (end group TCP_Client) */
#ifdef __cplusplus
}
#endif
#endif /* ETHERNET_CLIENT_H_ */
