/***************************************************************************//**
 * @file nb_iot.h
 * @brief header file for NB IoT high level command driver
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
#ifndef NB_IOT_H_
#define NB_IOT_H_
#include <stdint.h>
#include <stdbool.h>
#include "sl_status.h"
#include "at_parser_core.h"

/*******************************************************************************
 ********************************   MACROS   ***********************************
 ******************************************************************************/
#define DATA_MAX_LENGTH 2000u

/*******************************************************************************
 *****************************   STRUCTURES   **********************************
 ******************************************************************************/
typedef struct {
  uint8_t socket;
  uint16_t port;
  uint8_t port_type[15];
  uint8_t *address;
} bg96_nb_connection_t;

/**************************************************************************//**
 * @brief
 *    BG96 NB IoT initialization.
 *
 *****************************************************************************/
void bg96_nb_init(void);

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
                                    at_scheduler_status_t *output_object);

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
sl_status_t bg96_network_registration(at_scheduler_status_t *output_object);

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
                                     at_scheduler_status_t *output_object);

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
                              at_scheduler_status_t *output_object);

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
sl_status_t bg96_nb_receive_data(at_scheduler_status_t *output_object);

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
sl_status_t read_ip(at_scheduler_status_t *output_object);

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
sl_status_t read_imei(at_scheduler_status_t *output_object);

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
sl_status_t bg96_get_operator(at_scheduler_status_t *output_object);

#endif /* NB_IOT_H_ */
