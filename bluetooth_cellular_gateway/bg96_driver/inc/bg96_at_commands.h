/***************************************************************************//**
 * @file at_cmds.h
 * @brief header file
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#ifndef AT_CMDS_H
#define AT_CMDS_H

#include "sl_status.h"
#include "at_parser_core.h"
#include "at_parser_utility.h"

/******************************************************************************
 ******************************   MACROS   ************************************
 *****************************************************************************/
#define AT_DEFAULT_TIMEOUT  15000
#define AT_OPEN_TIMEOUT     6000
#define AT_SEND_TIMEOUT     90000
#define AT_SMS_TEXT_TIMEOUT 120000
#define AT_QIACT_TIMEOUT    150000
#define AT_QIDEACT_TIMEOUT  40000
#define AT_QIOPEN_TIMEOUT   150000
#define AT_COPS_TIMEOUT     180000

#define DATA_MAX_LENGTH     1460

/*******************************************************************************
 *****************************   STRUCTURES   **********************************
 ******************************************************************************/
typedef struct {
  uint8_t socket;
  uint16_t port;
  uint8_t port_type[15];
  uint8_t *address;
} bg96_tcp_connection_t;

typedef struct {
  uint8_t phone_number[32];
  uint8_t sms_text_content[256];
} bg96_sms_text_t;

typedef struct {
  uint8_t service_center_number[32];
  uint8_t phone_number[32];
  uint8_t sms_text_content[256];
} bg96_sms_pdu_t;

typedef enum {
  set_sms_mode_pdu = 0,
  set_sms_mode_text,
} set_sms_mode_t;

typedef enum {
  service_domain_type_PSOnly_e = 0,
  service_domain_type_CS_and_PS_e,
} config_service_domain_type_t;

/**************************************************************************//**
 * @brief
 *    BG96 AT - Query SIM card status.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data..
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_sim_status(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Read HW IMEI number.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data..
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_read_imei(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *   BG96 AT - Get module information.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_read_infor(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Query actual operator.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_get_operator(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT -  Network registration.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_network_registration(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Query CS service.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *****************************************************************************/
sl_status_t bg96_query_cs_service(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Query GPRS service.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *****************************************************************************/
sl_status_t bg96_query_gprs_service(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Query LTE service.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *****************************************************************************/
sl_status_t bg96_query_lte_service(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Activate a PDP Context.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *****************************************************************************/
sl_status_t bg96_activate_pdp_context(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Deactivate a PDP Context.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *    SL_STATUS_INVALID_PARAMETER if output_object == NULL.
 *****************************************************************************/
sl_status_t bg96_deactivate_pdp_context(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Read actual IP address.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_read_ip_addr(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Open a TCP or UDP connection.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_tcp_open_connection(bg96_tcp_connection_t *connection,
                                     at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Close TCP client connection.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_tcp_close_connection(bg96_tcp_connection_t *connection,
                                      at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - TCP send data function.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @param[in] data
 *    Pointer to the data to send.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_tcp_send_data(bg96_tcp_connection_t *connection,
                               uint8_t *data,
                               at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Retrieve the Received TCP/IP Data
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_tcp_receive_data(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Set TE character set as GSM.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data..
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_set_te_gsm(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Configure the service domain of UE.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @param[in] type
 *    Variable to set type of the service domain
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_config_service_domain(at_scheduler_status_t *output_object,
                                       config_service_domain_type_t type);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Set SMS mode.
 *
 * @param[in] mode
 *    Variable to set sms mode
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_set_sms_mode(at_scheduler_status_t *output_object,
                              set_sms_mode_t mode);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Send SMS Text.
 *
 * @param[in] sms_text_object
 *    Pointer to the sms text descriptor structure.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_send_sms_text(at_scheduler_status_t *output_object,
                               bg96_sms_text_t *sms_text_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Send SMS PDU.
 *
 * @param[in] sms_pdu_object
 *    Pointer to the sms pdu descriptor structure.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_send_sms_pdu(at_scheduler_status_t *output_object,
                              bg96_sms_pdu_t *sms_pdu_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Set SIM APN.
 *
 * @param[in] sim_apn
 *    Pointer to the SIM APN descriptor .
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_set_sim_apn(at_scheduler_status_t *output_object,
                             uint8_t *sim_apn);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Turn on GNSS.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_gnss_start(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Acquire Positioning Information.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_gnss_get_position(at_scheduler_status_t *output_object);

/**************************************************************************//**
 * @brief
 *    BG96 AT - Turn off GNSS.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_gnss_stop(at_scheduler_status_t *output_object);

#endif // AT_CMDS_H
