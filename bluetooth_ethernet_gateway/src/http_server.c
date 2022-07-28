/***************************************************************************//**
 * @file http_server.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ethernet.h"
#include "socket.h"

#include "http_server.h"

// HTTP Method
enum METHOD_HTTP {
  METHOD_ERR = 0,    /**< Error Method. */
  METHOD_GET = 1,    /**< GET Method.   */
  METHOD_HEAD = 2,   /**< HEAD Method.  */
  METHOD_POST = 3    /**< POST Method.  */
};

// HTTP GET Method
#define   PTYPE_ERR               0 /**< Error file. */
#define   PTYPE_HTML              1 /**< HTML file.  */
#define   PTYPE_GIF               2 /**< GIF file.   */
#define   PTYPE_TEXT              3 /**< TEXT file.  */
#define   PTYPE_JPEG              4 /**< JPEG file.  */
#define   PTYPE_FLASH             5 /**< FLASH file. */
#define   PTYPE_MPEG              6 /**< MPEG file.  */
#define   PTYPE_PDF               7 /**< PDF file.   */
#define   PTYPE_CGI               8 /**< CGI file.   */
#define   PTYPE_XML               9 /**< XML file.   */
#define   PTYPE_CSS               10 /**< CSS file.   */
#define   PTYPE_JS                11 /**< JavaScript file. */
#define   PTYPE_JSON              12 /**< JSON (JavaScript Standard Object Notation) file. */
#define   PTYPE_PNG               13 /**< PNG file.  */
#define   PTYPE_ICO               14 /**< ICON file. */
#define   PTYPE_TTF               20 /**< Font type: TTF file. */
#define   PTYPE_OTF               21 /**< Font type: OTF file. */
#define   PTYPE_WOFF              22 /**< Font type: WOFF file. */
#define   PTYPE_EOT               23 /**< Font type: EOT file. */
#define   PTYPE_SVG               24 /**< Font type: SVG file. */

// HTTP response
#define   STATUS_OK               200
#define   STATUS_CREATED          201
#define   STATUS_ACCEPTED         202
#define   STATUS_NO_CONTENT       204
#define   STATUS_MV_PERM          301
#define   STATUS_MV_TEMP          302
#define   STATUS_NOT_MODIF        304
#define   STATUS_BAD_REQ          400
#define   STATUS_UNAUTH           401
#define   STATUS_FORBIDDEN        403
#define   STATUS_NOT_FOUND        404
#define   STATUS_INT_SERR         500
#define   STATUS_NOT_IMPL         501
#define   STATUS_BAD_GATEWAY      502
#define   STATUS_SERV_UNAVAIL     503

#define INITIAL_WEBPAGE           "index.html"
#define M_INITIAL_WEBPAGE         "m/index.html"
#define MOBILE_INITIAL_WEBPAGE    "mobile/index.html"

// HTML Doc. for ERROR
static const char ERROR_HTML_PAGE[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 78\r\n\r\n<HTML>\r\n<BODY>\r\nSorry, the page you requested was not found.\r\n</BODY>\r\n</HTML>\r\n\0";
static const char ERROR_REQUEST_PAGE[] = "HTTP/1.1 400 OK\r\nContent-Type: text/html\r\nContent-Length: 50\r\n\r\n<HTML>\r\n<BODY>\r\nInvalid request.\r\n</BODY>\r\n</HTML>\r\n\0";

// HTML Doc. for CGI result
#define HTML_HEADER \
  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "

// Response header for HTML
#define RES_HTMLHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nContent-Length: "

// Response head for TEXT
#define RES_TEXTHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "

// Response head for GIF
#define RES_GIFHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: image/gif\r\nContent-Length: "

// Response head for JPEG
#define RES_JPEGHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: "

/* Response head for PNG */
#define RES_PNGHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: "

// Response head for FLASH
#define RES_FLASHHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: application/x-shockwave-flash\r\nContent-Length: "

// Response head for XML
#define RES_XMLHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nConnection: keep-alive\r\nContent-Length: "

// Response head for CSS
#define RES_CSSHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\nContent-Length: "

// Response head for JavaScript
#define RES_JSHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nContent-Length: "

// Response head for JSON
#define RES_JSONHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "

// Response head for ICO
#define RES_ICOHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: image/x-icon\r\nContent-Length: "

// Response head for CGI
#define RES_CGIHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "

// Response head for TTF, Font
#define RES_TTFHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: application/x-font-truetype\r\nContent-Length: "

// Response head for OTF, Font
#define RES_OTFHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: application/x-font-opentype\r\nContent-Length: "

// Response head for WOFF, Font
#define RES_WOFFHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: application/font-woff\r\nContent-Length: "

// Response head for EOT, Font
#define RES_EOTHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: application/vnd.ms-fontobject\r\nContent-Length: "

// Response head for SVG, Font
#define RES_SVGHEAD_OK \
  "HTTP/1.1 200 OK\r\nContent-Type: image/svg+xml\r\nContent-Length: "

static inline void safe_strncpy(char *dst, const char *src, size_t dst_size)
{
  while (*src && --dst_size > 0) {
    *dst++ = *src++;
  }
  *dst = '\0';
}

/*****************************************************************************
 * Private functions
 ****************************************************************************/
static void http_process_handler(w5x00_http_server_t *http,
                                 uint8_t s,
                                 w5x00_http_request_t *p_http_request);
static void send_http_response_header(uint8_t s,
                                      uint8_t *buf,
                                      uint32_t buf_size,
                                      uint8_t content_type,
                                      uint32_t body_len,
                                      uint16_t http_status);
static void send_http_response_body(w5x00_http_socket_t *socket,
                                    uint8_t *uri_name,
                                    uint8_t *buf,
                                    uint32_t buf_size,
                                    uint32_t start_addr,
                                    uint32_t file_len,
                                    const w5x00_http_server_callback_t *callback);
static void send_http_response_cgi(uint8_t s,
                                   uint8_t *buf,
                                   uint8_t *http_body,
                                   uint16_t file_len);

static const char *get_content_body(const char *uri);
static void make_http_response_head(char *buf,
                                    uint32_t buf_size,
                                    char type,
                                    uint32_t len);
static void find_http_uri_type(uint8_t *type, uint8_t *buff);
static void parse_http_request(w5x00_http_request_t *request, uint8_t *buf);
static uint8_t get_http_uri_name(uint8_t *uri,
                                 uint8_t *uri_buf,
                                 uint32_t uri_buf_len);
static void mid(char* src, char* s1, char* s2, char* sub);

/***************************************************************************//**
 * HTTP Server Init.
 ******************************************************************************/
sl_status_t w5x00_http_server_init(w5x00_http_server_t *http,
                                   uint16_t port,
                                   const w5x00_http_server_callback_t *callback)
{
  int i;

  if ((http == NULL) || (callback == NULL)) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  http->port = port;
  for (i = 0; i < W5x00_HTTP_MAX_CLIENT; i++) {
    uint8_t sockindex = w5x00_socket_begin(SnMR_TCP, port);
    if (sockindex < W5x00_MAX_SOCK_NUM) {
      if (SL_STATUS_OK == w5x00_socket_listen(sockindex)) {
        http->socket[i].socknum = sockindex;
        http->socket[i].file_id = 0;
        http->socket[i].file_len = 0;
        http->socket[i].file_offset = 0;
        http->socket[i].sock_status = STATE_HTTP_IDLE;
      } else {
        w5x00_socket_disconnect(sockindex);
        http->socket[i].socknum = W5x00_MAX_SOCK_NUM;
      }
    } else {
      http->socket[i].socknum = W5x00_MAX_SOCK_NUM;
    }
  }
  http->callback = callback;
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * HTTP Server Run.
 ******************************************************************************/
sl_status_t w5x00_http_server_run(w5x00_http_server_t *http)
{
  uint8_t i;

  if (http == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  for (i = 0; i < W5x00_HTTP_MAX_CLIENT; i++) {
    w5x00_http_server_socket_run(http, i);
  }
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * HTTP Server Socket Run.
 ******************************************************************************/
sl_status_t w5x00_http_server_socket_run(w5x00_http_server_t *http, uint8_t s)
{
  uint16_t len;
  uint32_t gettime = 0;
  w5x00_http_request_t parsed_http_request;

#ifdef W5x00_HTTP_SERVER_DEBUG
  uint8_t destip[4] = { 0, };
#endif
  w5x00_http_socket_t *socket;

  if ((http == NULL) || (s > W5x00_HTTP_MAX_CLIENT)) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  socket = &http->socket[s];

  // HTTP Service Start
  switch (w5x00_socket_status(socket->socknum)) {
    case SnSR_ESTABLISHED:
      // Interrupt clear
      if (w5x00_readSnIR(socket->socknum) & SnIR_CON) {
        w5x00_writeSnIR(socket->socknum, SnIR_CON);
      }

      // HTTP Process states
      switch (socket->sock_status) {
        case STATE_HTTP_IDLE:
          if ((len = w5x00_socket_recv_available(socket->socknum)) > 0) {
            if (len > (W5x00_HTTP_SERVER_BUFFER_SIZE - 1)) {
              len = W5x00_HTTP_SERVER_BUFFER_SIZE - 1;
            }
            len = w5x00_socket_recv(socket->socknum,
                                     (uint8_t *)http->buf,
                                     len);

            *(((uint8_t *)http->buf) + len) = '\0';

            parse_http_request(&parsed_http_request, http->buf);
#ifdef W5x00_HTTP_SERVER_DEBUG
            w5x00_readSnDIPR(socket->socknum, destip);
            w5x00_log_printf("\r\n");
            w5x00_log_printf("> HTTPSocket[%d] : HTTP Request received ", s);
            w5x00_log_printf("from %d.%d.%d.%d : %d\r\n",
                   destip[0],
                   destip[1],
                   destip[2],
                   destip[3],
                   w5x00_readSnDPORT(socket->socknum));
#endif
#ifdef W5x00_HTTP_SERVER_DEBUG
            w5x00_log_printf("> HTTPSocket[%d] : [State] STATE_HTTP_REQ_DONE\r\n",
                             s);
#endif
            // HTTP 'response' handler; 
            // includes send_http_response_header / body function
            http_process_handler(http,
                                 socket->socknum,
                                 &parsed_http_request);

            gettime = w5x00_get_tick_ms();
            // Check the TX socket buffer for End of HTTP response sends
            while (w5x00_socket_get_tx_free_size(socket->socknum)
                   != w5x00_get_socket_tx_max_size(socket->socknum)) {
              if ((w5x00_get_tick_ms() - gettime) > 3000) {
#ifdef W5x00_HTTP_SERVER_DEBUG
                w5x00_log_printf("> HTTPSocket[%d] : [State] STATE_HTTP_REQ_DONE: TX Buffer clear timeout\r\n",
                                 s);
#endif
                break;
              }
            }

            if (socket->file_len > 0) {
              socket->sock_status = STATE_HTTP_RES_INPROC;
            } else {
              socket->sock_status = STATE_HTTP_RES_DONE; // Send the 'HTTP response' end
            }
          }
          break;

        case STATE_HTTP_RES_INPROC:
          // Repeat: Send the remain parts of HTTP responses
#ifdef W5x00_HTTP_SERVER_DEBUG
          w5x00_log_printf("> HTTPSocket[%d] : [State] STATE_HTTP_RES_INPROC\r\n",
                           s);
#endif
          // Repeatedly send remaining data to client
          send_http_response_body(socket,
                                  0,
                                  http->buf,
                                  W5x00_HTTP_SERVER_BUFFER_SIZE,
                                  0,
                                  0,
                                  http->callback);

          if (socket->file_len == 0) {
            socket->sock_status = STATE_HTTP_RES_DONE;
          }
          break;

        case STATE_HTTP_RES_DONE:
#ifdef W5x00_HTTP_SERVER_DEBUG
          w5x00_log_printf("> HTTPSocket[%d] : [State] STATE_HTTP_RES_DONE\r\n",
                           s);
#endif
          // Socket file info structure re-initialize
          socket->file_len = 0;
          socket->file_offset = 0;
          socket->file_id = 0;
          socket->sock_status = STATE_HTTP_IDLE;
#ifdef W5x00_USE_WATCHDOG
          http->callback->wdt_reset();
#endif
          w5x00_socket_disconnect(socket->socknum);
          break;

        default:
          break;
      }
      break;

    case SnSR_CLOSE_WAIT:
#ifdef W5x00_HTTP_SERVER_DEBUG
    w5x00_log_printf("> HTTPSocket[%d] : ClOSE_WAIT\r\n", socket->socknum);  // if a peer requests to close the current connection
#endif
      w5x00_socket_disconnect(socket->socknum);
      break;

    case SnSR_CLOSED:
#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : CLOSED\r\n", socket->socknum);
#endif
      if (w5x00_socket_init(socket->socknum, SnMR_TCP,
                            http->port) == socket->socknum) {   // Reinitialize the socket
#ifdef W5x00_HTTP_SERVER_DEBUG
        w5x00_log_printf("> HTTPSocket[%d] : OPEN\r\n", socket->socknum);
#endif
      }
      break;

    case SnSR_INIT:
      w5x00_socket_listen(socket->socknum);
      break;

    case SnSR_LISTEN:
      break;

    default:
      break;
  } // end of switch

#ifdef W5x00_USE_WATCHDOG
  http->callback->wdt_reset();
#endif
  return SL_STATUS_OK;
}

static void send_http_response_header(uint8_t s,
                                      uint8_t *buf,
                                      uint32_t buf_size,
                                      uint8_t content_type,
                                      uint32_t body_len,
                                      uint16_t http_status)
{
  switch (http_status) {
    case STATUS_OK:     // HTTP/1.1 200 OK
      if ((content_type != PTYPE_CGI)
          && (content_type != PTYPE_XML)) { // CGI/XML type request does not respond HTTP header
#ifdef W5x00_HTTP_SERVER_DEBUG
        w5x00_log_printf("> HTTPSocket[%d] : HTTP Response Header - STATUS_OK\r\n",
                         s);
#endif
        make_http_response_head((char*)buf, buf_size, content_type, body_len);
      } else {
#ifdef W5x00_HTTP_SERVER_DEBUG
        w5x00_log_printf("> HTTPSocket[%d] : HTTP Response Header - NONE / CGI or XML\r\n",
                         s);
#endif
        // CGI/XML type request does not respond HTTP header to client
        http_status = 0;
      }
      break;

    case STATUS_BAD_REQ:   // HTTP/1.1 400 OK
#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : HTTP Response Header - STATUS_BAD_REQ\r\n",
                       s);
#endif
      if (buf_size > sizeof(ERROR_REQUEST_PAGE)) {
        buf_size = sizeof(ERROR_REQUEST_PAGE);
      }
      memcpy(buf, ERROR_REQUEST_PAGE, buf_size);
      break;

    case STATUS_NOT_FOUND:  // HTTP/1.1 404 Not Found
#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : HTTP Response Header - STATUS_NOT_FOUND\r\n",
                       s);
#endif
      if (buf_size > sizeof(ERROR_HTML_PAGE)) {
        buf_size = sizeof(ERROR_HTML_PAGE);
      }
      memcpy(buf, ERROR_HTML_PAGE, buf_size);
      break;

    default:
      break;
  }

  // Send the HTTP Response 'header'
  if (http_status) {
#ifdef W5x00_HTTP_SERVER_DEBUG
    w5x00_log_printf("> HTTPSocket[%d] : [Send] HTTP Response Header [ %d ]byte\r\n",
           s,
           (uint16_t)strlen((char *)buf));
#endif
    w5x00_socket_send(s, buf, strlen((char *)buf));
  }
}

static void send_http_response_body(w5x00_http_socket_t *socket,
                                    uint8_t *uri_name,
                                    uint8_t *buf,
                                    uint32_t buf_size,
                                    uint32_t start_addr,
                                    uint32_t file_len,
                                    const w5x00_http_server_callback_t *callback)
{
  uint32_t send_len;

  uint8_t flag_datasend_end = 0;

  // Send the HTTP Response 'body'; requested file
  if (!socket->file_len) { // ### Send HTTP response body: First part ###
    socket->file_id = start_addr;
    socket->file_len = file_len;
    if (file_len > (buf_size - 1)) {
      send_len = buf_size - 1;

      int n = strlen((char *)uri_name);
      if (n > (W5x00_HTTP_SERVER_MAX_CONTENT_NAME_LEN - 1)) {
        n = W5x00_HTTP_SERVER_MAX_CONTENT_NAME_LEN - 1;
      }
      memcpy(socket->file_name, uri_name, n);
      socket->file_name[n] = '\0';
#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : HTTP Response body - file name [ %s ]\r\n",
                       socket->socknum,
                       socket->file_name);
#endif

#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : HTTP Response body - file len [ %ld ]byte\r\n",
                       socket->socknum,
                       file_len);
#endif
    } else {
      // Send process end
      send_len = file_len;

#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : HTTP Response end - file len [ %ld ]byte\r\n",
                       socket->socknum,
                       send_len);
#endif
    }
  } else { // remained parts
    send_len = socket->file_len - socket->file_offset;

    if (send_len > buf_size - 1) {
      send_len = buf_size - 1;
      // socket->file_offset += send_len;
    } else {
#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : HTTP Response end - file len [ %ld ]byte\r\n",
                       socket->socknum,
                       socket->file_len);
#endif
      // Send process end
      flag_datasend_end = 1;
    }
#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("> HTTPSocket[%d] : HTTP Response body - send len [ %ld ]byte\r\n",
                       socket->socknum,
                       send_len);
#endif
  }

  if (send_len == callback->read_web_content(socket->file_id,
                                            buf,
                                            socket->file_offset,
                                            send_len)) {
    // Requested content send to HTTP client
#ifdef W5x00_HTTP_SERVER_DEBUG
    w5x00_log_printf("> HTTPSocket[%d] : [Send] HTTP Response body [ %ld ]byte\r\n",
                     socket->socknum,
                     send_len);
#endif
  } else {
    send_len = 0;
#ifdef W5x00_HTTP_SERVER_DEBUG
    w5x00_log_printf("> HTTPSocket[%d] : (File Read) / HTTP Send Failed - %s\r\n",
                     socket->socknum,
                     socket->file_name);
#endif
  }

  if (send_len) {
    w5x00_socket_send(socket->socknum, buf, send_len);
  } else {
    flag_datasend_end = 1;
  }

  if (flag_datasend_end) {
    socket->file_id = 0;
    socket->file_len = 0;
    socket->file_offset = 0;
    flag_datasend_end = 0;
  } else {
    socket->file_offset += send_len;
#ifdef W5x00_HTTP_SERVER_DEBUG
    w5x00_log_printf("> HTTPSocket[%d] : HTTP Response body - offset [ %ld ]\r\n",
                     socket->socknum,
                     socket->file_offset);
#endif
  }
}

static void send_http_response_cgi(uint8_t s,
                                   uint8_t *buf,
                                   uint8_t *http_body,
                                   uint16_t file_len)
{
  uint16_t send_len = 0;

#ifdef W5x00_HTTP_SERVER_DEBUG
  w5x00_log_printf("> HTTPSocket[%d] : HTTP Response Header + Body - CGI\r\n",
                   s);
#endif
  send_len = sprintf((char *)buf,
                     "%s%d\r\n\r\n%s",
                     RES_CGIHEAD_OK,
                     file_len,
                     http_body);
#ifdef W5x00_HTTP_SERVER_DEBUG
  w5x00_log_printf("> HTTPSocket[%d] : HTTP Response Header + Body - send len [ %d ]byte\r\n",
                   s,
                   send_len);
#endif

  w5x00_socket_send(s, buf, send_len);
}


static void http_process_handler(w5x00_http_server_t *http,
                                 uint8_t s,
                                 w5x00_http_request_t *p_http_request)
{
  static uint8_t uri_buf[W5x00_HTTP_SERVER_MAX_URI_SIZE] = { 0x00, };

  uint8_t *uri_name;
  uint32_t content_addr = 0;
  uint32_t content_num = 0;
  uint32_t file_len = 0;

  uint16_t http_status;
  uint8_t content_found;
  w5x00_http_socket_t *socket;

  socket = &http->socket[s];
  http_status = 0;
  file_len = 0;

  // method Analyze
  switch (p_http_request->method) {
    case METHOD_ERR:
      http_status = STATUS_BAD_REQ;
      send_http_response_header(socket->socknum,
                                http->buf,
                                W5x00_HTTP_SERVER_BUFFER_SIZE,
                                0,
                                0,
                                http_status);
      break;

    case METHOD_HEAD:
    case METHOD_GET:
      get_http_uri_name(p_http_request->uri,
                        uri_buf,
                        W5x00_HTTP_SERVER_MAX_URI_SIZE);
      uri_name = uri_buf;

      // If uri is "/", respond by index.html
      if (!strcmp((char *)uri_name, "/")) {
          safe_strncpy((char *)uri_name,
                       INITIAL_WEBPAGE,
                       W5x00_HTTP_SERVER_MAX_URI_SIZE);
      }

      if (!strcmp((char *)uri_name, "m")) {
          safe_strncpy((char *)uri_name,
                       M_INITIAL_WEBPAGE,
                       W5x00_HTTP_SERVER_MAX_URI_SIZE);
      }

      if (!strcmp((char *)uri_name, "mobile")) {
          safe_strncpy((char *)uri_name,
                        MOBILE_INITIAL_WEBPAGE,
                        W5x00_HTTP_SERVER_MAX_URI_SIZE);
      }
      // Checking requested file types (HTML, TEXT, GIF, JPEG and Etc. are included)
      find_http_uri_type(&p_http_request->type, uri_name);

#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("\r\n> HTTPSocket[%d] : HTTP Method GET\r\n",
                       socket->socknum);
      w5x00_log_printf("> HTTPSocket[%d] : Request Type = %d\r\n",
                       socket->socknum,
                       p_http_request->type);
      w5x00_log_printf("> HTTPSocket[%d] : Request uri = %s\r\n",
                       socket->socknum,
                       uri_name);
#endif

      if (p_http_request->type == PTYPE_CGI) {
        content_found = http->callback->get_cgi_handler((const char *)uri_name,
                                                        uri_buf,
                                                        &file_len);
        if (content_found
            && (file_len <= (W5x00_HTTP_SERVER_BUFFER_SIZE - (strlen(RES_CGIHEAD_OK) + 8)))) {
          send_http_response_cgi(socket->socknum,
                                 http->buf,
                                 uri_buf,
                                 (uint16_t)file_len);
        } else {
          send_http_response_header(socket->socknum,
                                    http->buf,
                                    W5x00_HTTP_SERVER_BUFFER_SIZE,
                                    PTYPE_CGI,
                                    0,
                                    STATUS_NOT_FOUND);
        }
        content_found = 0;
      } else {
        // Find the User registered index for web content
        if (http->callback->open_web_content((const char *)uri_buf,
                                             &content_num,
                                             &file_len)) {
          content_found = 1; // Web content found in code flash memory
          content_addr = (uint32_t)content_num;
        } else { // Not CGI request, Web content in 'SD card' or 'Data flash' requested
          content_found = 0; // fail to find content
        }

        if (!content_found) {
#ifdef W5x00_HTTP_SERVER_DEBUG
          w5x00_log_printf("> HTTPSocket[%d] : Unknown Page Request\r\n", s);
#endif
          http_status = STATUS_NOT_FOUND;
        } else {
#ifdef W5x00_HTTP_SERVER_DEBUG
          w5x00_log_printf("> HTTPSocket[%d] : Find Content [%s] ok - Start [%ld] len [ %ld ]byte\r\n",
                           socket->socknum,
                           uri_name,
                           content_addr,
                           file_len);
#endif
          http_status = STATUS_OK;
        }

        // Send HTTP header
        if (http_status) {
#ifdef W5x00_HTTP_SERVER_DEBUG
          w5x00_log_printf("> HTTPSocket[%d] : Requested content len = [ %ld ]byte\r\n",
                           socket->socknum,
                           file_len);
#endif
          send_http_response_header(socket->socknum,
                                    http->buf,
                                    W5x00_HTTP_SERVER_BUFFER_SIZE,
                                    p_http_request->type,
                                    file_len,
                                    http_status);
        }

        // Send HTTP body (content)
        if (http_status == STATUS_OK) {
          send_http_response_body(socket,
                                  uri_name,
                                  http->buf,
                                  W5x00_HTTP_SERVER_BUFFER_SIZE,
                                  content_addr,
                                  file_len,
                                  http->callback);
        }
      }
      break;

    case METHOD_POST:
      mid((char *)p_http_request->uri, "/", " HTTP", (char *)uri_buf);
      uri_name = uri_buf;
      // Check file type (HTML, TEXT, GIF, JPEG are included)
      find_http_uri_type(&p_http_request->type,
                         uri_name);

#ifdef W5x00_HTTP_SERVER_DEBUG
      w5x00_log_printf("\r\n> HTTPSocket[%d] : HTTP Method POST\r\n", s);
      w5x00_log_printf("> HTTPSocket[%d] : Request uri = %s ",
                       socket->socknum,
                       uri_name);
      w5x00_log_printf("Type = %d\r\n", p_http_request->type);
#endif

      if (p_http_request->type == PTYPE_CGI) { // HTTP POST Method; CGI Process
        file_len = sizeof(uri_buf);
        content_found = http->callback->post_cgi_handler((const char *)uri_name,
                                                         get_content_body((const char *)p_http_request->uri),
                                                         uri_buf,
                                                         &file_len);
#ifdef W5x00_HTTP_SERVER_DEBUG
        w5x00_log_printf("> HTTPSocket[%d] : [CGI: %s] / Response len [ %ld ]byte\r\n",
                         socket->socknum,
                         content_found ? "Content found":"Content not found",
                         file_len);
#endif
        if (content_found
            && (file_len <= (W5x00_HTTP_SERVER_BUFFER_SIZE - (strlen(RES_CGIHEAD_OK) + 8)))) {
          send_http_response_cgi(socket->socknum,
                                 http->buf,
                                 uri_buf,
                                 (uint16_t)file_len);

          // Reset the H/W for apply to the change configuration information
          if (content_found == HTTP_RESET) {
            http->callback->server_restart();
          }
        } else {
          send_http_response_header(socket->socknum,
                                    http->buf,
                                    W5x00_HTTP_SERVER_BUFFER_SIZE,
                                    PTYPE_CGI,
                                    0,
                                    STATUS_NOT_FOUND);
        }
      } else { // HTTP POST Method; Content not found
        send_http_response_header(socket->socknum,
                                  http->buf,
                                  W5x00_HTTP_SERVER_BUFFER_SIZE,
                                  0,
                                  0,
                                  STATUS_NOT_FOUND);
      }
      break;

    default:
      http_status = STATUS_BAD_REQ;
      send_http_response_header(socket->socknum,
                                http->buf,
                                W5x00_HTTP_SERVER_BUFFER_SIZE,
                                0,
                                0,
                                http_status);
      break;
  }
}

static const char *get_content_body(const char *uri)
{
  const char *start;
  const char *end;

  start = end = uri;
  while (*uri) {
    if (((uri[0] == '\r') && (uri[1] == '\n'))
        || ((uri[0] == '\n') && (uri[1] == '\r'))) { // line ending with <cr><lf>
      if (end == start) { // checking for empty line
        if (uri[2]) {
          return &uri[2]; // body is the next line
        } else {
          break; // next line is not found
        }
      }
      end = start = &uri[2];
      uri += 2;
      } else if ((uri[0] == '\n')
                 && (uri[1] != '\r')) { // line ending with <lf>
        if (end == start) { // checking for empty line
          if (uri[1]) {
            return &uri[1]; // body is the next line
          } else {
            break; // next line is not found
          }
        }
        end = start = &uri[1];
        uri += 1;
    } else {
      end = uri++;
    }
  }
  return NULL;
}

#if 0

/**
 *    @brief convert escape characters(%XX) to ASCII character
 */
  static void unescape_http_url(
    char * url /**< pointer to be converted (escape characters )*/
    )
  {
    int x, y;

    for (x = 0, y = 0; url[y]; ++x, ++y) {
      if ((url[x] = url[y]) == '%') {
        url[x] = c2d(url[y + 1]) * 0x10 + c2d(url[y + 2]);
        y += 2;
      }
    }
    url[x] = '\0';
  }

#endif

/***************************************************************************//**
 * @brief
 *    Make response header such as html, gif, jpeg,etc.
 * @param[in] buf
 *    pointer to response header to be made
 * @param[in] buf_size
 *    Size of the body buffer/response body data
 * @param[in] type
 *    Response type
 * @param[in] len
 *    Size of response body
 ******************************************************************************/
static void make_http_response_head(char *buf,
                                    uint32_t buf_size,
                                    char type,
                                    uint32_t len)
{
  char *head;

  if (buf_size < 5) {
    return;
  }

  // File type
  if (type == PTYPE_HTML) {
    head = RES_HTMLHEAD_OK;
  } else if (type == PTYPE_GIF) {
    head = RES_GIFHEAD_OK;
  } else if (type == PTYPE_TEXT) {
    head = RES_TEXTHEAD_OK;
  } else if (type == PTYPE_JPEG) {
    head = RES_JPEGHEAD_OK;
  } else if (type == PTYPE_FLASH) {
    head = RES_FLASHHEAD_OK;
  } else if (type == PTYPE_XML) {
    head = RES_XMLHEAD_OK;
  } else if (type == PTYPE_CSS) {
    head = RES_CSSHEAD_OK;
  } else if (type == PTYPE_JSON) {
    head = RES_JSONHEAD_OK;
  } else if (type == PTYPE_JS) {
    head = RES_JSHEAD_OK;
  } else if (type == PTYPE_CGI) {
    head = RES_CGIHEAD_OK;
  } else if (type == PTYPE_PNG) {
    head = RES_PNGHEAD_OK;
  } else if (type == PTYPE_ICO) {
    head = RES_ICOHEAD_OK;
  } else if (type == PTYPE_TTF) {
    head = RES_TTFHEAD_OK;
  } else if (type == PTYPE_OTF) {
    head = RES_OTFHEAD_OK;
  } else if (type == PTYPE_WOFF) {
    head = RES_WOFFHEAD_OK;
  } else if (type == PTYPE_EOT) {
    head = RES_EOTHEAD_OK;
  } else if (type == PTYPE_SVG) {
    head = RES_SVGHEAD_OK;
  }
#ifdef _HTTPPARSER_DEBUG_
  else {
    head = NULL;
    w5x00_log_printf("\r\n\r\n-MAKE HEAD UNKNOWN-\r\n");
  }
#else
  else {
    head = NULL;
  }
#endif

  snprintf(buf, buf_size, "%s%ld", head, len);
  len = strlen(buf);
    if (len > (buf_size - 5)) {
      len = (buf_size - 5);
    }
  strcat(buf, "\r\n\r\n");
  buf[len + 4] = '\0';
}

/***************************************************************************//**
 * @brief
 *    Find MIME type of a file
 * @param[in] type
 *    Type to be returned
 * @param[in] buff
 *    File name
 ******************************************************************************/
static void find_http_uri_type(uint8_t *type, uint8_t *buff)
{
  // Decide type according to extension
  char * buf;
  buf = (char *)buff;

    if (strstr(buf, ".htm") || strstr(buf, ".html")) {
      *type = PTYPE_HTML;
    } else if (strstr(buf, ".gif")) {
      *type = PTYPE_GIF;
    } else if (strstr(buf, ".text") || strstr(buf, ".txt")) {
      *type = PTYPE_TEXT;
    } else if (strstr(buf, ".jpeg") || strstr(buf, ".jpg")) {
      *type = PTYPE_JPEG;
    } else if (strstr(buf, ".swf")) {
      *type = PTYPE_FLASH;
    } else if (strstr(buf, ".cgi") || strstr(buf, ".CGI")) {
      *type = PTYPE_CGI;
    } else if (strstr(buf, ".json") || strstr(buf, ".JSON")) {
      *type = PTYPE_JSON;
    } else if (strstr(buf, ".js") || strstr(buf, ".JS")) {
      *type = PTYPE_JS;
    } else if (strstr(buf, ".CGI") || strstr(buf, ".cgi")) {
      *type = PTYPE_CGI;
    } else if (strstr(buf, ".xml") || strstr(buf, ".XML")) {
      *type = PTYPE_XML;
    } else if (strstr(buf, ".css") || strstr(buf, ".CSS")) {
      *type = PTYPE_CSS;
    } else if (strstr(buf, ".png") || strstr(buf, ".PNG")) {
      *type = PTYPE_PNG;
    } else if (strstr(buf, ".ico") || strstr(buf, ".ICO")) {
      *type = PTYPE_ICO;
    } else if (strstr(buf, ".ttf") || strstr(buf, ".TTF")) {
      *type = PTYPE_TTF;
    } else if (strstr(buf, ".otf") || strstr(buf, ".OTF")) {
      *type = PTYPE_OTF;
    } else if (strstr(buf, ".woff") || strstr(buf, ".WOFF")) {
      *type = PTYPE_WOFF;
    } else if (strstr(buf, ".eot") || strstr(buf, ".EOT")) {
      *type = PTYPE_EOT;
    } else if (strstr(buf, ".svg") || strstr(buf, ".SVG")) {
      *type = PTYPE_SVG;
    } else {
      *type = PTYPE_ERR;
    }
}

/***************************************************************************//**
 * @brief
 *    Parse http request from a peer
 * @param[in] request
 *    Request to be returned
 * @param[in] buff
 *    Pointer to be parsed
 ******************************************************************************/
static void parse_http_request(w5x00_http_request_t *request, uint8_t *buf)
{
  char *nexttok;

  nexttok = strtok((char*)buf, " ");
  if (!nexttok) {
    request->method = METHOD_ERR;
    return;
  }
  if (!strcmp(nexttok, "GET") || !strcmp(nexttok, "get")) {
    request->method = METHOD_GET;
    nexttok = strtok(NULL, " ");
  } else if (!strcmp(nexttok, "HEAD") || !strcmp(nexttok, "head")) {
    request->method = METHOD_HEAD;
    nexttok = strtok(NULL, " ");
  } else if (!strcmp(nexttok, "POST") || !strcmp(nexttok, "post")) {
    nexttok = strtok(NULL, "\0");
    request->method = METHOD_POST;
  } else {
    request->method = METHOD_ERR;
  }

  if (!nexttok) {
    request->method = METHOD_ERR;
    return;
  }
  int len = strlen(nexttok);
  if (len > (W5x00_HTTP_SERVER_MAX_URI_SIZE - 1)) {
    len = W5x00_HTTP_SERVER_MAX_URI_SIZE - 1;
  }
  memcpy(request->uri, nexttok, len);
  request->uri[len] = '\0';
}

#if 0

/**
 *    @brief get next parameter value in the request
 */
  static uint8_t *get_http_param_value(char *uri, char *param_name)
  {
    static uint8_t BUFPUB[256];
    uint8_t *name = 0;
    uint8_t *ret = BUFPUB;
    uint8_t *pos2;
    uint16_t len = 0, content_len = 0;
    uint8_t tmp_buf[10] = { 0x00, };

    if (!uri || !param_name) {
      return 0;
    }

    /***************/
    mid(uri, "Content-Length: ", "\r\n", (char *)tmp_buf);
    content_len = a2i(tmp_buf, 10);
    uri = strstr(uri, "\r\n\r\n");
    uri += 4;
    uri[content_len] = 0;

    /***************/

    if ((name = (uint8_t *)strstr(uri, param_name))) {
      name += strlen(param_name) + 1;
      pos2 = (uint8_t*)strstr((char*)name, "&");
      if (!pos2) {
        pos2 = name + strlen((char*)name);
      }
      len = pos2 - name;

      if (len) {
        ret[len] = 0;
        strncpy((char*)ret, (char*)name, len);
        unescape_http_url((char *)ret);
        replacetochar(ret, '+', ' ');
        // ret[len] = 0;
        // ret[strlen((int8*)ret)] = 0;
        // w5x00_log_printf("len=%d\r\n",len);
      } else {
        ret[0] = 0;
      }
    } else {
      return NULL;
    }
#ifdef _HTTPPARSER_DEBUG_
    w5x00_log_printf("  %s=%s\r\n", param_name, ret);
#endif
    return ret;
  }

#endif

static uint8_t get_http_uri_name(uint8_t *uri,
                                 uint8_t *uri_buf,
                                 uint32_t uri_buf_len)
{
  uint8_t * uri_ptr;

    if (!uri) {
      return 0;
    }

  safe_strncpy((char *)uri_buf, (char *)uri, uri_buf_len);

  uri_ptr = (uint8_t *)strtok((char *)uri_buf, " ?");

    if (strcmp((char *)uri_ptr, "/")) {
      uri_ptr++;
    }
  safe_strncpy((char *)uri_buf, (char *)uri_ptr, uri_buf_len);

#ifdef _HTTPPARSER_DEBUG_
  w5x00_log_printf("  uri_name = %s\r\n", uri_buf);
#endif

  return 1;
}

#if 0

/**
 *    @brief  CONVERT STRING INTO INTEGER
 *    @return a integer number
 */
  static uint16_t a2i(
    uint8_t * str, /**< is a pointer to convert */
    uint8_t base /**< is a base value (must be in the range 2 - 16) */
    )
  {
    unsigned int num = 0;
// debug_2013_11_25
//        while (*str !=0)
    while ((*str != 0) && (*str != 0x20)) {  // not include the space(0x020)
      num = num * base + c2d(*str++);
    }
    return num;
  }

#endif

/***************************************************************************//**
 * @brief
 *    Check strings and then execute callback function by each string.
 * @param[in] src
 *     The information of uri
 * @param[in] s1
 *    The start string to be researched
 * @param[in] s2
 *    The end string to be researched
 * @param[in] sub
 *    The string between s1 and s2
 * @return The length value atfer working
 ******************************************************************************/
static void mid(char *src, char *s1, char *s2, char *sub)
{
  char *sub1;
  char *sub2;
  uint16_t n;

  sub1 = strstr((char*)src, (char*)s1);
  sub1 += strlen((char*)s1);
  sub2 = strstr((char*)sub1, (char*)s2);

  n = sub2 - sub1;
  strncpy((char*)sub, (char*)sub1, n);
  sub[n] = '\0';
}

#if 0

/**
 *    @brief  replace the specified character in a string with new character
 */
  static void replacetochar(
    uint8_t * str,    /**< pointer to be replaced */
    uint8_t oldchar,  /**< old character */
    uint8_t newchar /**< new character */
    )
  {
    int x;
    for (x = 0; str[x]; x++) {
      if (str[x] == oldchar) {
        str[x] = newchar;
      }
    }
  }

#endif

#if 0

/**
 *    @brief  CONVERT CHAR INTO HEX
 *    @return HEX
 *
 *    This function converts HEX(0-F) to a character
 */
  static uint8_t c2d(
    uint8_t c /**< is a character('0'-'F') to convert to HEX */
    )
  {
    if ((c >= '0') && (c <= '9')) {
      return c - '0';
    }
    if ((c >= 'a') && (c <= 'f')) {
      return 10 + c - 'a';
    }
    if ((c >= 'A') && (c <= 'F')) {
      return 10 + c - 'A';
    }

    return (char)c;
  }

#endif
