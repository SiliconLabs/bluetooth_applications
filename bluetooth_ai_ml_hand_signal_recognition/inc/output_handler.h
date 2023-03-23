/***************************************************************************//**
 * @file output_handler.h
 * @brief output handler header file.
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
 * 1. The origin of this software must not be misrepresented{} you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef OUTPUT_HANDLER_H_
#define OUTPUT_HANDLER_H_

#include "sl_bt_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * Handle inference result
 *
 * This function is called whenever we have a succesful inference result.
 *
 * @param current_time timestamp of the inference result.
 * @param result classification result, this is number >= 0.
 * @param score the score of the result. This is number represents
 * the confidence of the result classification.
 * @param is_new_command true if the result is a new command, false otherwise.
 ******************************************************************************/
void handle_result(int32_t current_time,
                   uint8_t result,
                   uint8_t score,
                   bool is_new_command);

/*******************************************************************************
 * @brief
 *   Function to handle sl_bt_evt_gatt_server_user_read_request_id event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void hand_signal_read_requests(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle evt_gatt_server_characteristic_status_id event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void hand_signal_characteristic_status(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void hand_signal_disconnect_event(sl_bt_msg_t *evt);

#ifdef __cplusplus
}
#endif

#endif // OUTPUT_HANDLER_H_
