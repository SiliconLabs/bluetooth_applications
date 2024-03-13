/***************************************************************************//**
 * @file fingeprint_doorlock_app.c
 * @brief fingerprint doorlock app source file.
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include "app_assert.h"
#include "app_log.h"
#include "mikroe_a172mrq.h"
#include "sl_bt_api.h"
#include "sl_iostream_init_eusart_instances.h"
#include "sl_iostream_init_usart_instances.h"
#include "sl_sleeptimer.h"
#include "fingerprint_app.h"
#include "fingerprint_nvm.h"

#define fp_slot_is_empty(slot) (fp_slot[slot] == 0xFF)
#define REGISTER_TIMEOUT 300000000

static uint8_t fp_slot[10];
static uint8_t fp_count = 0;
static uint8_t fp_authorized_index = 0;
static volatile bool register_completed = false;
static bool send_cmd = false;
static sl_sleeptimer_timer_handle_t fingerprint_process_timer;

/***************************************************************************//**
 * Enum for describe the status of new fingerprint .
 ******************************************************************************/
typedef enum fingerprint_status {
  FINGERPRINT_AUTHORIZED,
  FINGERPRINT_NOT_AUTHORIZED,
  FINGERPRINT_UNKNOW
} fingerprint_status_t;

static volatile fingerprint_status_t fingerprint_status = FINGERPRINT_UNKNOW;

static void fingerprint_process_callback(sl_sleeptimer_timer_handle_t *timer,
                                         void *data);

/***************************************************************************//**
 * Init Fingerprint Function.
 ******************************************************************************/
void fingerprint_init(void)
{
  sl_status_t sc;

  fingerprint_config_nvm3_init();
  fingerprint_nvm3_get_config(fp_slot);

  for (int i = 0; i < FINGERPRINT_MAX_SLOT; i++) {
    if (!fp_slot_is_empty(i)) {
      fp_count++;
    }
  }

  mikroe_a172mrq_cfg_setup();
  sc = mikroe_a172mrq_init(sl_iostream_uart_mikroe_handle);
  app_assert_status(sc);

  mikroe_a172mrq_reset();
  sl_sleeptimer_delay_millisecond(1000);
  app_log("Fingerprint 2 Click is initialized!\r\n");
  sl_sleeptimer_start_periodic_timer_ms(&fingerprint_process_timer,
                                        500,
                                        fingerprint_process_callback,
                                        NULL,
                                        0,
                                        0);
}

/***************************************************************************//**
 * Add new fingerprint function.
 ******************************************************************************/
sl_status_t fingerprint_add(uint8_t index)
{
  sl_status_t sc = SL_STATUS_OK;
  uint32_t count_timeout = 0;

  register_completed = false;
  mikroe_a172mrq_reg_one_fp(index);
  while ((register_completed == false) && (count_timeout < REGISTER_TIMEOUT)) {
    count_timeout++;
  }
  if (register_completed) {
    fp_count++;
    fp_slot[index] = index;
    fingerprint_nvm3_u8_write(NVM3_FINGERPRINT_KEY + index, index);
  } else {
    sc = SL_STATUS_FAIL;
  }

  return sc;
}

/***************************************************************************//**
 * Remove authorized fingerprint function.
 ******************************************************************************/
sl_status_t fingerprint_remove(uint8_t index)
{
  if ((index >= FINGERPRINT_MAX_SLOT)
      || fp_slot_is_empty(index)) {
    return SL_STATUS_BT_ATT_VALUE_NOT_ALLOWED;
  }

  fp_slot[index] = 0xFF;
  fp_count--;
  mikroe_a172mrq_delete_one_fp(index);
  send_cmd = false;
  app_log("Fingerprint has been removed from location : %d \r\n", index);

  return fingerprint_nvm3_u8_write(NVM3_FINGERPRINT_KEY + index,
                                   fp_slot[index]);
}

/***************************************************************************//**
 * Compare the new fingerprint with authorized fingerprint function.
 ******************************************************************************/
sl_status_t fingerprint_compare(void)
{
  sl_status_t sc = SL_STATUS_NOT_INITIALIZED;
  static bool send_cmd = false;

  if ((fingerprint_status == FINGERPRINT_UNKNOW) && !send_cmd) {
    sc = mikroe_a172mrq_generic_write(MIKROE_FINGERPRINT2_CMD_FP_CMP,
                                      strlen(MIKROE_FINGERPRINT2_CMD_FP_CMP));
    send_cmd = true;
    if (SL_STATUS_OK == sc) {
    } else {
      app_log("Comparison processing failed!\r\n");
    }
  } else {
    if (fingerprint_status == FINGERPRINT_AUTHORIZED) {
      sc = SL_STATUS_ALREADY_EXISTS;
      send_cmd = false;
    } else if (fingerprint_status == FINGERPRINT_NOT_AUTHORIZED) {
      sc = SL_STATUS_INVALID_HANDLE;
      send_cmd = false;
    }
    fingerprint_status = FINGERPRINT_UNKNOW;
  }

  return sc;
}

/***************************************************************************//**
 * Fingerprint process callback function.
 ******************************************************************************/
static void fingerprint_process_callback(sl_sleeptimer_timer_handle_t *timer,
                                         void *data)
{
  (void) data;
  (void) timer;
  int32_t rsp_size;
  sl_status_t sc;
  int32_t check_buf_cnt;
  char uart_rx_buffer[MIKROE_FP2_RX_BUFFER_SIZE] = { 0 };

  sc = mikroe_a172mrq_generic_read(uart_rx_buffer,
                                   MIKROE_FP2_RX_BUFFER_SIZE,
                                   &rsp_size);
  if ((SL_STATUS_OK == sc) && (rsp_size > 0)) {
    // Validation of received data
    for (check_buf_cnt = 0; check_buf_cnt < rsp_size; check_buf_cnt++)
    {
      if (uart_rx_buffer[check_buf_cnt] == 0) {
        uart_rx_buffer[check_buf_cnt] = 13;
      }
    }
    app_log("%s", uart_rx_buffer);
    if (strstr(uart_rx_buffer, "</R>")) {
      if (strstr(uart_rx_buffer, "PASS_")) {
        fingerprint_status = FINGERPRINT_AUTHORIZED;
        for (check_buf_cnt = 0; check_buf_cnt < rsp_size; check_buf_cnt++) {
          if (uart_rx_buffer[check_buf_cnt] == '_') {
            break;
          }
        }
        fp_authorized_index = uart_rx_buffer[check_buf_cnt + 1] - 48;
      } else if (strstr(uart_rx_buffer, "FAIL")) {
        fingerprint_status = FINGERPRINT_NOT_AUTHORIZED;
      } else if (strstr(uart_rx_buffer, "FINISHED")) {
        register_completed = true;
      }
    }
    // Clear RX buffer
    memset(uart_rx_buffer, 0, MIKROE_FP2_RX_BUFFER_SIZE);
  } else {
  }
}

/***************************************************************************//**
 * Check free slot function.
 ******************************************************************************/
sl_status_t fingerprint_check_slot(uint8_t slot, bool *is_free)
{
  if (slot >= FINGERPRINT_MAX_SLOT) {
    return SL_STATUS_INVALID_INDEX;
  }

  if (NULL == is_free) {
    return SL_STATUS_NULL_POINTER;
  }

  if (fp_slot_is_empty(slot)) {
    *is_free = true;
  } else {
    *is_free = false;
  }

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Get free slot function.
 ******************************************************************************/
sl_status_t fingerprint_get_free_slot(uint8_t *free_slot)
{
  if (fp_count > FINGERPRINT_MAX_SLOT) {
    return SL_STATUS_FULL;
  } else {
    for (int i = 0; i < FINGERPRINT_MAX_SLOT; i++) {
      if (fp_slot_is_empty(i)) {
        *free_slot = i;
        return SL_STATUS_OK;
      }
    }
  }

  return SL_STATUS_FULL;
}

/***************************************************************************//**
 * Get the index of authorized fingerprint function.
 ******************************************************************************/
uint8_t fingerprint_get_authorized_index(void)
{
  return fp_authorized_index;
}

/***************************************************************************//**
 * Get the number of authorized fingerprints function.
 ******************************************************************************/
uint8_t fingerprint_get_num_of_fps(void)
{
  return fp_count;
}
