/***************************************************************************//**
 * @file ethernet_server.h
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
#ifndef ETHERNET_SERVER_
#define ETHERNET_SERVER_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup TCP_Server TCP Server
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup TCP_Server
 * @brief  Ethernet TCP Server function.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    TCP server object definition
 ******************************************************************************/
typedef struct {
  uint16_t port;                            /// Listen port
  uint16_t server_port[W5x00_MAX_SOCK_NUM]; /// Port list to mark socket are in use
} w5x00_ethernet_server_t;

/***************************************************************************//**
 * @brief
 *    Initialize TCP server
 * @param ss
 *    TCP server instance
 * @param port
 *    Listen port
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_server_init(w5x00_ethernet_server_t *ss,
                                       uint16_t port);

/***************************************************************************//**
 * @brief
 *    Start server
 * @param[in] ss
 *    TCP server instance
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_ethernet_server_begin(w5x00_ethernet_server_t *ss);

/***************************************************************************//**
 * @brief
 *    Server listen
 * @param[in] ss
 *    TCP server instance
 * @return
 *    Client object data.
 ******************************************************************************/
w5x00_ethernet_client_t w5x00_ethernet_server_accept(w5x00_ethernet_server_t *ss);

/***************************************************************************//**
 * @brief
 *    Write to all client
 * @param[in] ss
 *    TCP server instance
 * @param[in] buffer
 *    Pointer to the Data buffer to be written
 * @param[in] size
 *    Size of data buffer
 * @return
 *    >= 0: Size of written data
 *    <0: Error is occurred
 ******************************************************************************/
int w5x00_ethernet_server_write(w5x00_ethernet_server_t *ss,
                                const uint8_t *buffer,
                                size_t size);

/** @} (end group TCP_Server) */
#ifdef __cplusplus
}
#endif
#endif // ETHERNET_SERVER_
