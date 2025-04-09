/***************************************************************************//**
 * @file at_cmds.c
 * @brief source file
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
#include "bg96_at_commands.h"

#if (BG96_ENALBLE_DEBUGOUT == 1)
#define bg96_log(...)                       printf(__VA_ARGS__)
#else
#define bg96_log(...)
#endif

#define BITMASK_7BITS                       0x7F
#define BITMASK_8BITS                       0xFF
#define BITMASK_HIGH_4BITS                  0xF0
#define BITMASK_LOW_4BITS                   0x0F
#define TYPE_OF_ADDRESS_INTERNATIONAL_PHONE 0x91
#define SMS_MAX_7BIT_TEXT_LENGTH            160
#define SMS_MAX_PDU_LENGTH                  256
#define SMS_DELIVER_ONE_MESSAGE             0x04
#define SMS_SUBMIT                          0x11

static const char digits[] =
{
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static int16_t pdu_encode(uint8_t *service_center_number,
                          uint8_t *phone_number,
                          uint8_t *sms_text,
                          uint8_t *output_buffer,
                          uint16_t buffer_size);

static int16_t encode_pdu_message(uint8_t *sms_text,
                                  int16_t sms_text_length,
                                  uint8_t *output_buffer,
                                  uint16_t buffer_size);
static int16_t encode_phone_number(uint8_t *phone_number,
                                   uint8_t *output_buffer,
                                   uint16_t buffer_size);

static void str_cut_chr(uint8_t *str, uint8_t chr);
static void uint8_to_hex(uint8_t input, uint8_t *output);
static void uint8_to_str(uint8_t input, uint8_t *output);

/**************************************************************************//**
 *                       AT CALLBACK FUNCTIONS
 *****************************************************************************/
static void bg96_at_cpin_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_imei_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_infor_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_te_gsm_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_service_domain_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_set_sms_mode_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_sms_send_command_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_sms_send_data_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_set_sim_apn_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_gps_start_stop_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_ok_error_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_cops_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_recv_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_send_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_data_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_ip_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_qistate_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_open_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_gpsloc_cb(uint8_t *new_line, uint8_t call_number);
static void bg96_at_net_reg_status_cb(uint8_t *new_line, uint8_t call_number);

/**************************************************************************//**
 *    BG96 AT - Query SIM card status.
 *****************************************************************************/
sl_status_t bg96_sim_status(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_cpin = { "AT+CPIN?",
                                         bg96_at_cpin_cb,
                                         AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_cpin));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Read HW IMEI number.
 *****************************************************************************/
sl_status_t bg96_read_imei(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_imei = { "AT+GSN",
                                         bg96_at_imei_cb,
                                         AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_imei));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *   BG96 AT - Get module information.
 *****************************************************************************/
sl_status_t bg96_read_infor(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_infor = { "ATI",
                                          bg96_at_infor_cb,
                                          AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_infor));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Query actual operator.
 *****************************************************************************/
sl_status_t bg96_get_operator(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_cops = { "AT+COPS?",
                                         bg96_at_cops_cb,
                                         AT_COPS_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_cops));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT -  Network registration.
 *****************************************************************************/
sl_status_t bg96_network_registration(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_cmds[] =
  {
    {
      "AT+COPS=0",     // Automatic mode to select Operator
      bg96_at_ok_error_cb,
      AT_COPS_TIMEOUT
    },
    {
      "AT+CFUN=0",     //  Switch the ME to minimum functionality
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+QCFG=\"nbsibscramble\",0",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+QCFG=\"nwscanmode\",0,1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+QCFG=\"roamservice\",2,1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+QCFG=\"nwscanseq\",020103,1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+QCFG=\"band\",0,0,80,1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+QCFG=\"iotopmode\",1,1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+QCFG=\"servicedomain\",1,1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+CFUN=1",     // Switch the ME to full functionality
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      "AT+CREG=1;+CGREG=1;+CEREG=1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
    {
      // Set the APN to "hologram" with no username or password
      "AT+QICSGP=1,1,\"hologram\",\"\",\"\",1",
      bg96_at_ok_error_cb,
      AT_DEFAULT_TIMEOUT
    },
  };

  uint8_t at_cmd_size = sizeof(at_cmds) / sizeof(at_cmd_desc_t);
  uint8_t i;

  for (i = 0; i < at_cmd_size; i++) {
    validate(cmd_status, at_parser_add_cmd_to_q(&at_cmds[i]));
  }
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Query CS service.
 *****************************************************************************/
sl_status_t bg96_query_cs_service(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_query_cs = { "AT+CREG?",
                                             bg96_at_net_reg_status_cb,
                                             AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_query_cs));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Query GPRS service.
 *****************************************************************************/
sl_status_t bg96_query_gprs_service(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_query_gprs = { "AT+CGREG?",
                                               bg96_at_net_reg_status_cb,
                                               AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_query_gprs));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *   BG96 AT - Query LTE service.
 *****************************************************************************/
sl_status_t bg96_query_lte_service(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_query_lte = { "AT+CEREG?",
                                              bg96_at_net_reg_status_cb,
                                              AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_query_lte));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/***************************************************************************//**
 *  BG96 AT - Activate a PDP Context.
 ******************************************************************************/
sl_status_t bg96_activate_pdp_context(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_qiact = { "AT+QIACT=1",
                                          bg96_at_ok_error_cb,
                                          AT_QIACT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_qiact));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/***************************************************************************//**
 *  BG96 AT - Deactivate a PDP Context .
 ******************************************************************************/
sl_status_t bg96_deactivate_pdp_context(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_qideact = { "AT+QIDEACT=1",
                                            bg96_at_ok_error_cb,
                                            AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_qideact));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Read actual IP address.
 *****************************************************************************/
sl_status_t bg96_read_ip_addr(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_ip = { "AT+QIACT?",
                                       bg96_at_ip_cb,
                                       AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_ip));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Open a TCP or UDP connection.
 *****************************************************************************/
sl_status_t bg96_tcp_open_connection(bg96_tcp_connection_t *connection,
                                     at_scheduler_status_t *output_object)
{
  if ((NULL == output_object) || (NULL == connection)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  // format example:
  //   "AT+QIOPEN=1,0,\"TCP\",\"cloudsocket.hologram.io\",9999,0,1"
  uint8_t conn_string[50];
  uint8_t base_cmd[] = "AT+QIOPEN=1,";
  static at_cmd_desc_t at_open = { "",
                                   bg96_at_open_cb,
                                   AT_QIOPEN_TIMEOUT };
  static const at_cmd_desc_t at_qstate = { "AT+QISTATE=0,1",
                                           bg96_at_qistate_cb,
                                           AT_DEFAULT_TIMEOUT };

  at_parser_clear_cmd(&at_open);
  validate(cmd_status,
           at_parser_extend_cmd(&at_open, base_cmd));
  if (!strncmp((const char *) connection->port_type, "TCP", 15)
      || !strncmp((const char *) connection->port_type, "UDP", 15)) {
    snprintf((char *) conn_string, 50, "%d,\"%s\",\"%s\",%d,0,1",
             (int) connection->socket, (const char *) connection->port_type,
             (const char *) connection->address, (int) connection->port);
    validate(cmd_status, at_parser_extend_cmd(&at_open, conn_string));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_open));
  } else if (!strncmp((const char *) connection->port_type, "TCP LISTENER",
                      15)
             || !strncmp((const char *) connection->port_type,
                         "UDP LISTENER", 15)) {
    snprintf((char *) conn_string, 50, "%d,\"%s\",\"%s\",0,%d,0",
             (int) connection->socket, (const char *) connection->port_type,
             (const char *) connection->address, (int) connection->port);
    validate(cmd_status, at_parser_extend_cmd(&at_open, conn_string));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_open));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_qstate));
  } else {
    return SL_STATUS_FAIL;
  }
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Close TCP client connection.
 *****************************************************************************/
sl_status_t bg96_tcp_close_connection(bg96_tcp_connection_t *connection,
                                      at_scheduler_status_t *output_object)
{
  if ((NULL == output_object) || (NULL == connection)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  uint8_t conn_string[5];
  uint8_t base_cmd[] = "AT+QICLOSE=";
  static at_cmd_desc_t at_close = { "",
                                    bg96_at_ok_error_cb,
                                    AT_OPEN_TIMEOUT };

  snprintf((char *) conn_string, 5, "%d", (int) connection->socket);

  at_parser_clear_cmd(&at_close);
  validate(cmd_status, at_parser_extend_cmd(&at_close, base_cmd));
  validate(cmd_status, at_parser_extend_cmd(&at_close, conn_string));
  validate(cmd_status, at_parser_add_cmd_to_q(&at_close));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - TCP send data function.
 *****************************************************************************/
sl_status_t bg96_tcp_send_data(bg96_tcp_connection_t *connection,
                               uint8_t *data,
                               at_scheduler_status_t *output_object)
{
  if ((NULL == output_object) || (NULL == data) || (NULL == connection)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  uint8_t data_l_string[10];
  uint8_t base_cmd[] = "AT+QISEND=";
  static at_cmd_desc_t at_qisend = { "",
                                     bg96_at_send_cb,
                                     AT_DEFAULT_TIMEOUT };
  static at_cmd_desc_t at_data = { "",
                                   bg96_at_data_cb,
                                   AT_SEND_TIMEOUT };

  at_parser_clear_cmd(&at_qisend);
  at_parser_clear_cmd(&at_data);
  validate(cmd_status, at_parser_extend_cmd(&at_qisend, base_cmd));
  validate(cmd_status, at_parser_extend_cmd(&at_data, data));

  size_t data_length = sl_strlen((char *) at_data.cms_string);
  if (data_length < DATA_MAX_LENGTH) {
    snprintf((char *) data_l_string, 10, "%d,%d", (int) connection->socket,
             (int) data_length);
    validate(cmd_status, at_parser_extend_cmd(&at_qisend, data_l_string));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_qisend));
    validate(cmd_status, at_parser_add_cmd_to_q(&at_data));
    validate(cmd_status, at_parser_start_scheduler(output_object));
  }

  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Retrieve the Received TCP/IP Data
 *****************************************************************************/
sl_status_t bg96_tcp_receive_data(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_qird = { "AT+QIRD=11,100",
                                         bg96_at_recv_cb,
                                         AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_qird));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Set TE character set as GSM.
 *****************************************************************************/
sl_status_t bg96_set_te_gsm(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_te_gsm = { "AT+CSCS=\"GSM\"",
                                           bg96_at_te_gsm_cb,
                                           AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_te_gsm));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Configure the service domain of UE.
 *****************************************************************************/
sl_status_t bg96_config_service_domain(at_scheduler_status_t *output_object,
                                       config_service_domain_type_t type)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_config_service_domain = { "",
                                                    bg96_at_service_domain_cb,
                                                    AT_DEFAULT_TIMEOUT };

  at_parser_clear_cmd(&at_config_service_domain);

  if (type == service_domain_type_PSOnly_e) {
    sl_strcpy_s((char *)at_config_service_domain.cms_string,
                CMD_MAX_SIZE,
                (const char * )"AT+QCFG=\"servicedomain\",1,1");
  } else if (type == service_domain_type_CS_and_PS_e) {
    sl_strcpy_s((char *)at_config_service_domain.cms_string,
                CMD_MAX_SIZE,
                (const char * )"AT+QCFG=\"servicedomain\",2,1");
  }

  validate(cmd_status, at_parser_add_cmd_to_q(&at_config_service_domain));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Set SMS mode.
 *****************************************************************************/
sl_status_t bg96_set_sms_mode(at_scheduler_status_t *output_object,
                              set_sms_mode_t mode)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_set_sms_mode = { "",
                                           bg96_at_set_sms_mode_cb,
                                           AT_DEFAULT_TIMEOUT };

  at_parser_clear_cmd(&at_set_sms_mode);

  if (mode == set_sms_mode_pdu) {
    sl_strcpy_s((char *)at_set_sms_mode.cms_string, CMD_MAX_SIZE,
                (const char * )"AT+CMGF=0");
  } else if (mode == set_sms_mode_text) {
    sl_strcpy_s((char *)at_set_sms_mode.cms_string, CMD_MAX_SIZE,
                (const char * )"AT+CMGF=1");
  }

  validate(cmd_status, at_parser_add_cmd_to_q(&at_set_sms_mode));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Send SMS Text.
 *****************************************************************************/
sl_status_t bg96_send_sms_text(at_scheduler_status_t *output_object,
                               bg96_sms_text_t *sms_text_object)
{
  if ((NULL == output_object) || (NULL == sms_text_object)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  uint8_t base_cmd[] = "AT+CMGS=";
  static at_cmd_desc_t at_sms_cmd = { "",
                                      bg96_at_sms_send_command_cb,
                                      AT_DEFAULT_TIMEOUT };
  static at_cmd_desc_t at_sms_data = { "",
                                       bg96_at_sms_send_data_cb,
                                       AT_SMS_TEXT_TIMEOUT };

  at_parser_clear_cmd(&at_sms_cmd);
  at_parser_clear_cmd(&at_sms_data);

  validate(cmd_status,
           at_parser_extend_cmd(&at_sms_cmd, (uint8_t *)base_cmd));
  validate(cmd_status, at_parser_extend_cmd(&at_sms_cmd, (uint8_t *)"\""));
  validate(cmd_status, at_parser_extend_cmd(&at_sms_cmd,
                                            sms_text_object->phone_number));
  validate(cmd_status, at_parser_extend_cmd(&at_sms_cmd, (uint8_t *)"\""));
  validate(cmd_status, at_parser_extend_cmd(&at_sms_data,
                                            sms_text_object->sms_text_content));
  validate(cmd_status, at_parser_extend_cmd(&at_sms_data, (uint8_t *)"\032"));
  validate(cmd_status, at_parser_add_cmd_to_q(&at_sms_cmd));
  validate(cmd_status, at_parser_add_cmd_to_q(&at_sms_data));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Send SMS PDU.
 *****************************************************************************/
sl_status_t bg96_send_sms_pdu(at_scheduler_status_t *output_object,
                              bg96_sms_pdu_t *sms_pdu_object)
{
  if ((NULL == output_object) || (NULL == sms_pdu_object)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  uint8_t smsc[32] = { 0 };
  uint8_t phone_num[32] = { 0 };
  uint8_t byte_buf[4] = { 0 };
  uint8_t pdu_buf[SMS_MAX_PDU_LENGTH] = { 0 };

  uint8_t base_cmd[] = "AT+CMGS=";
  static at_cmd_desc_t at_sms_cmd = { "",
                                      bg96_at_sms_send_command_cb,
                                      AT_DEFAULT_TIMEOUT };
  static at_cmd_desc_t at_sms_data = { "",
                                       bg96_at_sms_send_data_cb,
                                       AT_SMS_TEXT_TIMEOUT };
  at_parser_clear_cmd(&at_sms_cmd);
  at_parser_clear_cmd(&at_sms_data);

  sl_strcpy_s((char *) smsc,
              32,
              (const char *) sms_pdu_object->service_center_number);
  sl_strcpy_s((char *) phone_num,
              32,
              (const char *) sms_pdu_object->phone_number);
  str_cut_chr(smsc, '+');
  str_cut_chr(phone_num, '+');

  int16_t pdu_buf_len = pdu_encode(smsc,
                                   phone_num,
                                   sms_pdu_object->sms_text_content,
                                   pdu_buf,
                                   SMS_MAX_PDU_LENGTH);

  if (pdu_buf_len < 0) {
    return SL_STATUS_FAIL;
  }

  uint8_t length = pdu_buf_len - ((sl_strlen((char *) smsc) - 1) / 2 + 3);
  uint8_to_str(length, byte_buf);
  str_cut_chr(byte_buf, ' ');
  validate(cmd_status,
           at_parser_extend_cmd(&at_sms_cmd, (uint8_t *)base_cmd));
  validate(cmd_status,
           at_parser_extend_cmd(&at_sms_cmd, (uint8_t *)byte_buf));

  for ( int16_t cnt = 0; cnt < pdu_buf_len; cnt++ )
  {
    uint8_to_hex(pdu_buf[cnt], byte_buf);
    validate(cmd_status, at_parser_extend_cmd(&at_sms_data, byte_buf));
  }

  validate(cmd_status, at_parser_extend_cmd(&at_sms_data, (uint8_t *)"\032"));
  validate(cmd_status, at_parser_add_cmd_to_q(&at_sms_cmd));
  validate(cmd_status, at_parser_add_cmd_to_q(&at_sms_data));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Set SIM APN.
 *****************************************************************************/
sl_status_t bg96_set_sim_apn(at_scheduler_status_t *output_object,
                             uint8_t *sim_apn)
{
  if ((NULL == output_object) || (NULL == sim_apn)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static at_cmd_desc_t at_set_apn = { "AT+CGDCONT=1,\"IP\",\"",
                                      bg96_at_set_sim_apn_cb,
                                      AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_extend_cmd(&at_set_apn, (uint8_t *)sim_apn));
  validate(cmd_status, at_parser_extend_cmd(&at_set_apn, (uint8_t *)"\""));

  validate(cmd_status, at_parser_add_cmd_to_q(&at_set_apn));
  validate(cmd_status, at_parser_start_scheduler(output_object));

  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Turn on GNSS.
 *****************************************************************************/
sl_status_t bg96_gnss_start(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_qgps = { "AT+QGPS=1",
                                         bg96_at_gps_start_stop_cb,
                                         AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_qgps));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Acquire Positioning Information.
 *****************************************************************************/
sl_status_t bg96_gnss_get_position(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_gpsloc = { "AT+QGPSLOC?",
                                           bg96_at_gpsloc_cb,
                                           AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_gpsloc));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *  BG96 AT - Turn off GNSS.
 *****************************************************************************/
sl_status_t bg96_gnss_stop(at_scheduler_status_t *output_object)
{
  if (NULL == output_object) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t cmd_status = SL_STATUS_OK;
  static const at_cmd_desc_t at_imei = { "AT+QGPSEND",
                                         bg96_at_gps_start_stop_cb,
                                         AT_DEFAULT_TIMEOUT };

  validate(cmd_status, at_parser_add_cmd_to_q(&at_imei));
  validate(cmd_status, at_parser_start_scheduler(output_object));
  return cmd_status;
}

/**************************************************************************//**
 *                           STATIC FUNCTIONS
 *****************************************************************************/
static int16_t pdu_encode(uint8_t *service_center_number,
                          uint8_t *phone_number,
                          uint8_t *sms_text,
                          uint8_t *output_buffer,
                          uint16_t buffer_size)
{
  if ((NULL == service_center_number) || (NULL == phone_number)
      || (NULL == sms_text) || (NULL == output_buffer)) {
    return -1;
  }

  if (buffer_size < 2) {
    return -1;
  }

  int16_t output_buffer_len = 0;
  int16_t length = 0;

  // 1. Set SMS center number.
  if ((service_center_number != 0)
      && (sl_strlen((char *) service_center_number) > 0)) {
    // Add type of address.
    output_buffer[1] = TYPE_OF_ADDRESS_INTERNATIONAL_PHONE;
    length = encode_phone_number(service_center_number,
                                 output_buffer + 2,
                                 buffer_size - 2);
    if ((length < 0) || (length >= 254)) {
      return -1;
    }
    length++;
  }
  output_buffer[0] = length;
  output_buffer_len = length + 1;
  if (output_buffer_len + 4 > buffer_size) {
    return -1; // Check if it has space for four more bytes.
  }

  // 2. Set type of message.
  output_buffer[output_buffer_len++] = SMS_SUBMIT;
  output_buffer[output_buffer_len++] = 0x00; // Message reference.

  // 3. Set phone number.
  output_buffer[output_buffer_len] = sl_strlen((char *) phone_number);
  output_buffer[output_buffer_len + 1] = TYPE_OF_ADDRESS_INTERNATIONAL_PHONE;
  length = encode_phone_number(phone_number,
                               output_buffer + output_buffer_len + 2,
                               buffer_size - output_buffer_len - 2);
  output_buffer_len += length + 2;
  if (output_buffer_len + 4 > buffer_size) {
    return -1; // Check if it has space for four more bytes.
  }

  // 4. Protocol identifiers.
  output_buffer[output_buffer_len++] = 0x00; // TP-PID: Protocol identifier.
  output_buffer[output_buffer_len++] = 0x00; // TP-DCS: Data coding scheme.
  output_buffer[output_buffer_len++] = 0xB0; // TP-VP: Validity: 10 days

  // 5. SMS message.
  int16_t sms_text_length = sl_strlen((char *) sms_text);
  if (sms_text_length > SMS_MAX_7BIT_TEXT_LENGTH) {
    return -1;
  }
  output_buffer[output_buffer_len++] = sms_text_length;

  length = encode_pdu_message(sms_text,
                              sms_text_length,
                              output_buffer + output_buffer_len,
                              buffer_size - output_buffer_len);
  if (length < 0) {
    return -1;
  }

  output_buffer_len += length;
  return output_buffer_len;
}

static int16_t encode_pdu_message(uint8_t *sms_text, int16_t sms_text_length,
                                  uint8_t *output_buffer, uint16_t buffer_size)
{
  if ((NULL == sms_text) || (NULL == output_buffer)) {
    return -1;
  }

  int16_t output_buffer_len = 0;

  // Check if output buffer is big enough.
  if ((sms_text_length * 7 + 7) / 8 > buffer_size) {
    return -1;
  }

  int16_t carry_on_bits = 1;
  int16_t i = 0;

  for ( ; i < sms_text_length - 1; ++i )
  {
    output_buffer[output_buffer_len++] =
      ((sms_text[i] & BITMASK_7BITS) >> (carry_on_bits - 1))
      | ((sms_text[i + 1] & BITMASK_7BITS) << (8 - carry_on_bits));
    carry_on_bits++;
    if (carry_on_bits == 8) {
      carry_on_bits = 1;
      ++i;
    }
  }

  if (i < sms_text_length) {
    output_buffer[output_buffer_len++]
      = (sms_text[i] & BITMASK_7BITS) >> (carry_on_bits - 1);
  }

  return output_buffer_len;
}

static int16_t encode_phone_number(uint8_t *phone_number,
                                   uint8_t *output_buffer,
                                   uint16_t buffer_size)
{
  if ((NULL == phone_number) || (NULL == output_buffer)) {
    return -1;
  }

  int16_t output_buffer_len = 0;
  int16_t phone_number_length = sl_strlen((char *) phone_number);

  // Check if the output buffer is big enough.
  if ((phone_number_length + 1) / 2 > buffer_size) {
    return -1;
  }

  int16_t i = 0;
  for ( ; i < phone_number_length; ++i )
  {
    if ((phone_number[i] < '0') || (phone_number[i] > '9')) {
      return -1;
    }

    if (i % 2 == 0) {
      output_buffer[output_buffer_len++]
        = BITMASK_HIGH_4BITS | (phone_number[i] - '0');
    } else {
      output_buffer[output_buffer_len - 1] =
        (output_buffer[output_buffer_len - 1] & BITMASK_LOW_4BITS)
        | ((phone_number[i] - '0') << 4);
    }
  }

  return output_buffer_len;
}

static void str_cut_chr(uint8_t *str, uint8_t chr)
{
  uint16_t cnt_0, cnt_1;
  for ( cnt_0 = 0; cnt_0 < sl_strlen((char *) str); cnt_0++ )
  {
    if (str[cnt_0] == chr) {
      for ( cnt_1 = cnt_0; cnt_1 < sl_strlen((char *) str); cnt_1++ )
      {
        str[cnt_1] = str[cnt_1 + 1];
      }
    }
  }
}

static void uint8_to_hex(uint8_t input, uint8_t *output)
{
  if (output == NULL) {
    return;
  }

  output[0] = digits[input >> 4];
  output[1] = digits[input & 0xF];
  output[2] = 0;
}

static void uint8_to_str(uint8_t input, uint8_t *output)
{
  if (output == NULL) {
    return;
  }

  uint8_t digit_pos;

  for ( digit_pos = 0; digit_pos < 3; digit_pos++ )
  {
    output[digit_pos] = ' ';
  }

  output[digit_pos--] = 0;

  while (1)
  {
    output[digit_pos] = (input % 10u) + 48;
    input = input / 10u;
    if (digit_pos == 0) {
      break;
    }

    digit_pos--;
  }
}

static void bg96_at_cpin_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: bg96_at_cpin_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "AT+CPIN")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, "ERROR")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      } else {
        at_parser_report_data(new_line);
      }
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_imei_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_imei_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "AT+GSN")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      at_parser_report_data(new_line);
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_infor_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_infor_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "ATI")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      break;
    case 3:
      break;
    case 4:
      if (has_substring(new_line, "Revision:")) {
        at_parser_report_data(new_line);
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 5:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_te_gsm_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_te_gsm_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "AT+CSCS=")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_service_domain_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_service_domain_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "+QCFG=\"servicedomain\"")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_set_sms_mode_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_set_sms_mode_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "AT+CMGF=")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, "OK")) {
        at_parser_report_data(new_line);
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_sms_send_command_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_sms_send_command_cb[%d]: %s\r\n",
           call_number, new_line);
  switch (call_number) {
    case 1:
      if (has_substring(new_line, "+CMS ERROR:")
          || has_substring(new_line, "ERROR")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, ">")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_sms_send_data_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_sms_send_data_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
    case 2:
      if (has_substring(new_line, "+CMS ERROR:")
          || has_substring(new_line, "ERROR")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }

      if (has_substring(new_line, "+CMGS:")) {
        at_parser_report_data(new_line);
      }
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_set_sim_apn_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_set_sim_apn_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "AT+CGDCONT")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, "OK")) {
        at_parser_report_data(new_line);
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_gps_start_stop_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_gps_start_stop_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if ((!has_substring(new_line,
                          "AT+QGPS="))
          && (!has_substring(new_line, "AT+QGPSEND"))) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, "OK")) {
        at_parser_report_data(new_line);
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_ok_error_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_ok_error_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      break;
    case 2:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_net_reg_status_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: net_reg_status[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      break;
    case 2:
      if ((!has_substring(new_line, ",1"))
          && (!has_substring(new_line, ",5"))) {
        at_parser_scheduler_error(SL_STATUS_NOT_AVAILABLE);
      }
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_cops_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_cops_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "COPS?")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (!has_substring(new_line, ",")) {
        at_parser_scheduler_error(SL_STATUS_NOT_AVAILABLE);
      } else {
        at_parser_report_data(new_line);
      }
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_recv_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  uint8_t *space_ptr;
  uint32_t qird_data;
  static bool data_available = false;

  bg96_log("[DEBUG]: at_recv_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "+QIRD:")) {
        at_parser_report_data(new_line);
        at_parser_scheduler_error(SL_STATUS_FAIL);
      } else {
        space_ptr = (uint8_t *) strchr((const char *) new_line, ' ');
        if (space_ptr != NULL) {
          qird_data =
            (uint32_t) strtol((const char *) (++space_ptr), NULL, 10);
          if (qird_data > 0) {
            data_available = true;
          }
        } else {
          at_parser_scheduler_error(SL_STATUS_FAIL);
        }
      }
      break;
    case 2:
      if (data_available) {
        at_parser_report_data(new_line);
      } else {
        if (has_substring(new_line, "OK")) {
          at_parser_scheduler_next_cmd();
        } else {
          at_parser_scheduler_error(SL_STATUS_FAIL);
        }
      }
      data_available = false;
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_send_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_send_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      break;
    case 2:
      if (has_substring(new_line, ">")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_data_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_data_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "{")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (!has_substring(new_line, "SEND OK")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 3:
      if (!has_substring(new_line, "+QIURC:")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 4:
      if (!has_substring(new_line, "[")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 5:
      if (!has_substring(new_line, "+QIURC:")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      } else {
        at_parser_scheduler_next_cmd();
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_ip_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_ip_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      break;
    case 2:
      if (has_substring(new_line, "+QIACT:")) {
        at_parser_report_data(new_line);
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_qistate_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  bg96_log("[DEBUG]: at_qistate_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      at_parser_report_data(new_line);
      if (!has_substring(new_line, "+QISTATE:")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_open_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  // wait OK
  // wait +QIOPEN: 0,0 if second parameter != 0 then error
  uint8_t *coma;
  uint32_t error_code;

  bg96_log("[DEBUG]: at_open_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      break;
    case 2:
      if (!has_substring(new_line, "OK")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 3:
      at_parser_report_data(new_line);
      if (has_substring(new_line, "+QIOPEN")) {
        coma = (uint8_t *) strchr((const char *) new_line, ',');
        if (NULL != coma) {
          error_code = strtol((const char *) (++coma), NULL, 10);
          if (error_code != 0) {
            at_parser_report_data(new_line);
            at_parser_scheduler_error(SL_STATUS_FAIL);
          } else {
            at_parser_scheduler_next_cmd();
          }
        } else {
          at_parser_scheduler_error(SL_STATUS_FAIL);
        }
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}

static void bg96_at_gpsloc_cb(uint8_t *new_line, uint8_t call_number)
{
  if (new_line == NULL) {
    return;
  }

  // wait for +QGPSLOC: ...
  // wait OK
  bg96_log("[DEBUG]: at_gpsloc_cb[%d]: %s\r\n", call_number, new_line);
  switch (call_number) {
    case 1:
      if (!has_substring(new_line, "AT+QGPSLOC?")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;
    case 2:
      if (!has_substring(new_line, "+QGPSLOC:")) {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      } else {
        at_parser_report_data(new_line);
      }
      break;
    case 3:
      if (has_substring(new_line, "OK")) {
        at_parser_scheduler_next_cmd();
      } else {
        at_parser_scheduler_error(SL_STATUS_FAIL);
      }
      break;

    default:
      at_parser_scheduler_error(SL_STATUS_FAIL);
      break;
  }
}
