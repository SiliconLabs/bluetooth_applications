/***************************************************************************//**
 * @file http_server.h
 * @brief Wiznet W5x00 HTTP server implementation.
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

#include <stdint.h>
#include "w5x00.h"

#ifndef  HTTP_SERVER_H__
#define  HTTP_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup HTTP HTTP Server
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup HTTP
 * @brief  Ethernet DHCP client function.
 * @details
 * @{
 ******************************************************************************/

/// HTTP Simple Return Value
#define HTTP_FAILED         0
#define HTTP_OK             1
#define HTTP_RESET          2

/// HTTP Content NAME length
#define W5x00_HTTP_SERVER_MAX_CONTENT_NAME_LEN    128

/// HTTP Timeout
#define HTTP_MAX_TIMEOUT_SEC                      3      // Second

/// HTTP Max URI size
#ifndef W5x00_HTTP_SERVER_MAX_URI_SIZE
// #define HTTP_SERVER_MAX_URI_SIZE               1461
#define W5x00_HTTP_SERVER_MAX_URI_SIZE            1024
#endif

#ifndef W5x00_HTTP_SERVER_BUFFER_SIZE
#define W5x00_HTTP_SERVER_BUFFER_SIZE             (1024)
#endif

#if !defined(W5x00_HTTP_MAX_CLIENT)
#define W5x00_HTTP_MAX_CLIENT                     W5x00_MAX_SOCK_NUM
#endif

/// HTTP Process states list
enum STATE_HTTP {
  STATE_HTTP_IDLE       = 0,   ///< IDLE, Waiting for data received (TCP established)
  STATE_HTTP_REQ_INPROC = 1,   ///< Received HTTP request from HTTP client
  STATE_HTTP_REQ_DONE   = 2,   ///< The end of HTTP request parse
  STATE_HTTP_RES_INPROC = 3,   ///< Sending the HTTP response to HTTP client (in progress)
  STATE_HTTP_RES_DONE   = 4,   ///< The end of HTTP response send (HTTP transaction ended)
};

/// HTTP request info
typedef struct {
  uint8_t method;                               ///< request method #STATE_HTTP value enum
  uint8_t type;                                 ///< request type(PTYPE_HTML...).
  uint8_t uri[W5x00_HTTP_SERVER_MAX_URI_SIZE];  ///< request file name.
} w5x00_http_request_t;

/// HTTP socket state
typedef struct {
  w5x00_socket_t socknum;                                    ///< Soket number
  uint8_t sock_status;                                       ///< Socket status
  uint8_t file_name[W5x00_HTTP_SERVER_MAX_CONTENT_NAME_LEN]; ///< Content file name
  uint32_t file_id;                                          ///< Content file ID
  uint32_t file_len;                                         ///< Content file total length
  uint32_t file_offset;                                      ///< Content file offset
} w5x00_http_socket_t;

/***************************************************************************//**
 * @brief
 *    Restart server request callback type
 ******************************************************************************/
typedef void (*w5x00_http_server_restart_t)(void);
#ifdef W5x00_USE_WATCHDOG
/***************************************************************************//**
 * @brief
 *    Restart watchdog callback type
 ******************************************************************************/
typedef void (*w5x00_http_wdt_reset)(void);
#endif

/***************************************************************************//**
 * @brief
 *    Open content file callback function type (HTTP GET request)
 * @param[in] content_name
 *    Content file name
 * @param[out] file_id
 *    Content file id
 * @param[out] file_len
 *    Size of file
 * @return
 *    1 on success
 *    0 on failure
 ******************************************************************************/
typedef uint8_t (*w5x00_http_open_web_content_t)(const char *content_name,
                                                 uint32_t *file_id,
                                                 uint32_t *file_len);
/***************************************************************************//**
 * @brief
 *    Read content file callback type (HTTP GET request)
 * @param[in] file_id
 *    Content file id
 * @param[out] buf
 *    Output buffer
 * @param[in] offset
 *    Offset in file
 * @param[in] size
 *    Size to read
 * @return
 *    Size of read data
 ******************************************************************************/
typedef uint16_t (*w5x00_http_read_web_content_t)(uint32_t file_id,
                                                  uint8_t *buf,
                                                  uint32_t offset,
                                                  uint16_t size);

/***************************************************************************//**
 * @brief
 *    Close content file callback type (HTTP GET request)
 * @param file_id
 *    Content file id
 ******************************************************************************/
typedef void (*w5x00_http_close_web_content_t)(uint32_t file_id);

/***************************************************************************//**
 * @brief
 *    Read CGI content callback type (HTTP GET request)
 * @param[in] uri_name
 *    CGI name
 * @param[out] buf
 *    Output buffer
 * @param[out] file_len
 *    Size of the CGI data
 * @return
 *    1 on success
 *    0 on failure
 ******************************************************************************/
typedef uint8_t (*w5x00_http_get_cgi_handler_t)(const char *uri_name,
                                                uint8_t *buf,
                                                uint32_t *file_len);

/***************************************************************************//**
 * @brief
 *    CGI Post request handle (HTTP POST request)
 * @param[in] uri_name
 *    CGI name
 * @param[in] body
 *    Body data
 * @param[out] buf
 *    Response body buffer
 * @param[in] file_len
 *    Size of the body buffer/response body data
 * @return
 *    1 on success
 *    0 on failure
 ******************************************************************************/
typedef uint8_t (*w5x00_http_post_cgi_handler_t)(const char *uri_name,
                                                 const char *body,
                                                 uint8_t *buf,
                                                 uint32_t *file_len);
typedef struct {
  w5x00_http_server_restart_t server_restart;       ///< Request restart server
#ifdef W5x00_USE_WATCHDOG
  w5x00_http_wdt_reset wdt_reset;                   ///< Reset watchdog
#endif
  w5x00_http_open_web_content_t open_web_content;   ///< Open web content file
  w5x00_http_read_web_content_t read_web_content;   ///< Open web content file
  w5x00_http_close_web_content_t close_web_content; ///< Close web content file
  w5x00_http_get_cgi_handler_t get_cgi_handler;     ///< Get CGI
  w5x00_http_post_cgi_handler_t post_cgi_handler;   ///< Post CGI
} w5x00_http_server_callback_t;

/// HTTP server object defination
typedef struct {
  w5x00_http_socket_t socket[W5x00_HTTP_MAX_CLIENT];  ///< Socket state
  uint16_t port;                                      ///< Listen port
  const w5x00_http_server_callback_t *callback;       ///< Callback
  uint8_t buf[W5x00_HTTP_SERVER_BUFFER_SIZE];         ///< Buffer to parse the request
} w5x00_http_server_t;

/***************************************************************************//**
 * @brief
 *    Init HTTP server
 * @param http
 *    HTTP server instance
 * @param[in] port
 *    Listen port
 * @param[in] callback
 *    Callback struct define by #w5x00_http_server_callback_t
 ******************************************************************************/
sl_status_t w5x00_http_server_init(w5x00_http_server_t *http,
                                   uint16_t port,
                                   const w5x00_http_server_callback_t *callback);

/***************************************************************************//**
 * @brief
 *    Run server on 1 socket
 * @param[in] http
 *    HTTP server instance
 * @param[in] s
 *    Socket number
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_http_server_socket_run(w5x00_http_server_t *http, uint8_t s);

/***************************************************************************//**
 * @brief
 *    Run server on all socket
 * @param[in] http
 *    HTTP server instance
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t w5x00_http_server_run(w5x00_http_server_t *http);

/** @} (end group HTTP) */
#ifdef __cplusplus
}
#endif
#endif // HTTP_SERVER_H__
