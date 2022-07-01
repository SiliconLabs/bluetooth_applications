/***************************************************************************//**
 * @file logger_sd_card.c
 * @brief Write & read log from sd card
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ********************************************************************************
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
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

#ifndef LOGGER_SD_CARD_H_
#define LOGGER_SD_CARD_H_
#include <stdarg.h>
#include "ff.h"

/***************************************************************************//**
 * @brief
 *    Output line of log callback
 * @param line
 *    Pointer to line string
 * @param length
 *    Length of log line
 ******************************************************************************/
typedef void (*logger_sd_card_readline_callback_t)(const char *line,
                                                   int length);

/***************************************************************************//**
 * @brief
 *    Initialize sd card logger module
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t logger_sd_card_init(void);

/***************************************************************************//**
 * @brief
 *    Create formated log entry
 * @param fmt
 *    String format that follow printf function's format
 * @param args
 *    Argument list
 * @return
 *    Length of the log entry
 ******************************************************************************/
int logger_sd_card_create_log_entry_va_args(const char *fmt,
                                            va_list args);

/***************************************************************************//**
 * @brief
 *    Create formated log entry
 * @param fmt
 *    String format that follow printf function's format
 * @return
 *    Length of the log entry
 ******************************************************************************/
static inline int logger_sd_card_create_log_entry(const char *fmt, ...)
{
  va_list args;
  int n;

  va_start(args, fmt);
  n = logger_sd_card_create_log_entry_va_args(fmt, args);
  va_end(args);
  return n;
}

/***************************************************************************//**
 * @brief
 *    Get current log entry
 * @param line
 *    Pointer to the output that will get address of the log entry
 * @param length
 *    Pointer to length of the log entry
 * @return
 *    SL_STATUS_OK: log is created
 *    SL_STATUS_FAIL: log is not created
 ******************************************************************************/
sl_status_t logger_sd_card_get_current_log_entry(const char **line,
                                                 int *length);

/***************************************************************************//**
 * @brief
 *    Write current log entry to file
 * @param filename
 *    File to write
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t logger_sd_card_append_current_log_entry(const char *filename);

/***************************************************************************//**
 * @brief
 *    Write formated log entry to file
 * @param filename
 *    File to write
 * @param fmt
 *    String format that follow printf function's format
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
static inline sl_status_t logger_sd_card_append_log_entry(const char *filename,
                                                          const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  logger_sd_card_create_log_entry_va_args(fmt, args);
  va_end(args);
  return logger_sd_card_append_current_log_entry(filename);
}

/***************************************************************************//**
 * @brief
 *    Read all log from file line by line
 * @param filename
 *    File to read
 * @param callback
 *    Line data callback
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure.
 ******************************************************************************/
sl_status_t logger_sd_card_readline(const char *filename,
                                    logger_sd_card_readline_callback_t callback);

/***************************************************************************//**
 * @brief
 *    Delete a log file
 * @param filename
 *    File to delete
 ******************************************************************************/
#define logger_sd_card_clear_log(filename) f_unlink(filename)

#endif /* LOGGER_SD_CARD_H_ */
