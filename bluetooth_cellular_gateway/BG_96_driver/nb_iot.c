/***************************************************************************//**
 * @file nb_iot.c
 * @brief NB IoT high level command driver source
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include "nb_iot.h"
#include "at_parser_core.h"
#include "at_parser_events.h"
#include "bg96_driver.h"
#include <string.h>
#include <stdio.h>
#include "at_parser_utility.h"

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT initialization.
 *
 *****************************************************************************/
void bg96_nb_init(void)
{
  bg96_init();
}

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT open a TCP or UDP connection.
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
sl_status_t bg96_nb_open_connection(bg96_nb_connection_t *connection,
                                    at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  //format example: "AT+QIOPEN=1,0,\"TCP\",\"cloudsocket.hologram.io\",9999,0,1"
  uint8_t conn_string[50];
  uint8_t base_cmd[] = "AT+QIOPEN=1,";
  static at_cmd_desc_t at_open = { "", at_open_cb, AT_OPEN_TIMEOUT };
  static at_cmd_desc_t at_qstate = { "AT+QISTATE=0,1", at_qistate_cb,
  AT_DEFAULT_TIMEOUT };
  static at_cmd_desc_t at_qiact = { "AT+QIACT=1", at_ok_error_cb,
    AT_DEFAULT_TIMEOUT };



  at_parser_clear_cmd(&at_open);
  validate(cmd_status, at_parser_extend_cmd(&at_open, base_cmd));
  if (!strcmp((const char*) connection->port_type, "TCP")
      || !strcmp((const char*) connection->port_type, "UDP")) {
    sprintf((char*) conn_string, "%d,\"%s\",\"%s\",%d,0,1",
        (int) connection->socket, (const char*) connection->port_type,
        (const char*) connection->address, (int) connection->port);
    validate(cmd_status, at_parser_extend_cmd(&at_open, conn_string));
    //validate(cmd_status, at_parser_add_cmd_to_q(&at_qiact));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_open));
    //validate(cmd_status, at_parser_add_cmd_to_q(&at_qstate));

  } else if (!strcmp((const char*) connection->port_type, "TCP LISTENER")
      || !strcmp((const char*) connection->port_type, "UDP LISTENER")) {

    sprintf((char*) conn_string, "%d,\"%s\",\"%s\",0,%d,0",
        (int) connection->socket, (const char*) connection->port_type,
        (const char*) connection->address, (int) connection->port);
    validate(cmd_status, at_parser_extend_cmd(&at_open, conn_string));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_qiact));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_open));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_qstate));
  } else {
    return SL_STATUS_FAIL;
  }
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT close client connection.
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
sl_status_t bg96_nb_close_connection(bg96_nb_connection_t *connection,
                                     at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  uint8_t conn_string[5];
  uint8_t base_cmd[] = "AT+QICLOSE=";
  static at_cmd_desc_t at_close = { "", at_ok_error_cb, AT_OPEN_TIMEOUT };

  sprintf((char*) conn_string, "%d", (int) connection->socket);

  at_parser_clear_cmd(&at_close);
  validate(cmd_status, at_parser_extend_cmd(&at_close, base_cmd));
  validate(cmd_status, at_parser_extend_cmd(&at_close, conn_string));
  validate(cmd_status, at_parser_add_cmd_to_q(&at_close));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 * @brief
 *    BG96 NB send data function.
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
sl_status_t bg96_nb_send_data(bg96_nb_connection_t *connection,
                              uint8_t *data,
                              at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  uint8_t data_l_string[10];
  uint8_t base_cmd[] = "AT+QISEND=";
  static at_cmd_desc_t at_qisend = { "", at_send_cb, AT_DEFAULT_TIMEOUT };
  static at_cmd_desc_t at_data = { "", at_data_cb, AT_SEND_TIMEOUT };

  at_parser_clear_cmd(&at_qisend);
  at_parser_clear_cmd(&at_data);
  validate(cmd_status, at_parser_extend_cmd(&at_qisend, base_cmd));
  validate(cmd_status, at_parser_extend_cmd(&at_data, data));

  size_t data_length = strlen((const char*) at_data.cms_string);
  if (data_length < DATA_MAX_LENGTH) {
    sprintf((char*) data_l_string, "%d,%d", (int) connection->socket,
        (int) data_length);
    validate(cmd_status, at_parser_extend_cmd(&at_qisend, data_l_string));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_qisend));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_data));
    validate(cmd_status, at_parser_start_scheduler(output_object));
    return cmd_status;
  }
  return SL_STATUS_ALLOCATION_FAILED;
}

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT receive data function.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_nb_receive_data(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_qird = { "AT+QIRD=11,100", at_recv_cb,
  AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_qird));
  validate(cmd_status, at_parser_start_scheduler(output_object));

  return cmd_status;
}

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT network registration.
 *
 * @param[out] output_object
 *    Pointer to the output object which contains the command status and
 *    output data.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_network_registration(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_cmds[] = { { "AT+CFUN=0", at_ok_error_cb,
  AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QCFG=\"nbsibscramble\",0",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QCFG=\"nwscanmode\",0,1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QCFG=\"roamservice\",2,1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QCFG=\"nwscanseq\",020103,1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QCFG=\"band\",0,0,80,1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QCFG=\"iotopmode\",1,1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QCFG=\"servicedomain\",1,1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+CGDCONT=1,\"IP\",\"hologram\"",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     { "AT+CFUN=1", at_ok_error_cb,
                                     AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+COPS?",
                                       at_cops_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+CREG=1;+CGREG=1;+CEREG=1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+COPS=0",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT },
                                     {
                                       "AT+QICSGP=1,1,\"hologram\",\"\",\"\",1",
                                       at_ok_error_cb,
                                       AT_DEFAULT_TIMEOUT }, };

  uint8_t at_cmd_size = sizeof(at_cmds) / sizeof(at_cmd_desc_t);
  uint8_t i;

  for (i = 0; i < at_cmd_size; i++) {
    validate(cmd_status, at_parser_add_cmd_to_q(&at_cmds[i]));
  }
  validate(cmd_status, at_parser_start_scheduler(output_object));

  return cmd_status;
}

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT read actual IP address function.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t read_ip(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_ip = { "AT+QIACT?", at_ip_cb, AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_ip));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT get actual operator function.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t bg96_get_operator(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_cops = { "AT+COPS?", at_cops_cb, AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_cops));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT read HW IMEI number.
 *
 * @param[in] connection
 *    Pointer to the connection descriptor structure.
 *
 * @return
 *    SL_STATUS_OK if command successfully added to the command queue.
 *    SL_STATUS_FAIL if scheduler is busy or command queue is full.
 *****************************************************************************/
sl_status_t read_imei(at_scheduler_status_t *output_object)
{
  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_imei = { "AT+GSN", at_imei_cb, AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_imei));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}
