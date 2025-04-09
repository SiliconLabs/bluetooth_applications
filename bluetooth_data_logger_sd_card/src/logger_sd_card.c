/***************************************************************************//**
 * @file logger_sd_card.c
 * @brief Write & read log from sd card
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
#include <string.h>
#include <stdio.h>
#include "sl_status.h"
#include "sl_gpio.h"
#include "logger_sd_card.h"
#include "sl_spidrv_instances.h"
#include "sl_sdc_sd_card.h"

#define EOL                       "\n"
#define EOL_LENGTH                (1)
#define LOG_DATA_LINE_MAX_LENGTH  (512)
#define MOUNT_PATH                ""

static char line_buf[LOG_DATA_LINE_MAX_LENGTH];
static int line_length = 0;

static FATFS fs;

/***************************************************************************//**
 * Logger Initialize.
 ******************************************************************************/
sl_status_t logger_sd_card_init(void)
{
  FRESULT fr;
  int i;

  sl_gpio_t pin_port = { sl_spidrv_exp_handle->initData.portRx,
                         sl_spidrv_exp_handle->initData.pinRx };
  sl_gpio_set_pin_mode(&pin_port, SL_GPIO_MODE_INPUT_PULL, 1);
  sd_card_spi_init(sl_spidrv_exp_handle);

  for (i = 0; i < 5; i++) {
    fr = f_mount(&fs, MOUNT_PATH, 1);
    if (fr == FR_OK) {
      f_unmount(MOUNT_PATH);
      break;
    }
    if (FR_OK != f_mkfs("", NULL, line_buf, sizeof(line_buf))) {
      break;
    }
  }
  return fr == FR_OK ? SL_STATUS_OK : SL_STATUS_IO;
}

/***************************************************************************//**
 * Create A Log Entry.
 ******************************************************************************/
int logger_sd_card_create_log_entry_va_args(const char *fmt,
                                            va_list args)
{
  int i;
  char *eol = EOL;

  line_length = vsnprintf(line_buf, sizeof(line_buf) - 3, fmt, args);

  for (i = 0; eol[i]; i++) {
    line_buf[line_length++] = eol[i];
  }
  line_buf[line_length] = '\0';
  return line_length;
}

/***************************************************************************//**
 * Get Current Log Entry.
 ******************************************************************************/
sl_status_t logger_sd_card_get_current_log_entry(const char **line, int *length)
{
  if (line_length > 0) {
    *line = line_buf;
    *length = line_length;
    return SL_STATUS_OK;
  }
  return SL_STATUS_FAIL;
}

/***************************************************************************//**
 * Append Current Log Entry To SD Card.
 ******************************************************************************/
sl_status_t logger_sd_card_append_current_log_entry(const char *filename)
{
  FRESULT fr;
  FIL fil;
  UINT bw;

  if (line_length == 0) {
    return SL_STATUS_NOT_READY;
  }

  fr = f_mount(&fs, MOUNT_PATH, 1);
  if (FR_OK != fr) {
    return SL_STATUS_IO;
  }

  fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_APPEND);
  if (FR_OK != fr) {
    goto end;
  }

  fr = f_write(&fil, line_buf, line_length, &bw);

  if ((FR_OK != fr) || (bw != (UINT)line_length)) {
    goto close;
  }
  // discard current log entry
  line_length = 0;
  // call f_sync() to force write all log data to sdcard
  f_sync(&fil);

  close:
  f_close(&fil);

  end:
  f_unmount(MOUNT_PATH);
  return FR_OK == fr ? SL_STATUS_OK : SL_STATUS_IO;
}

/***************************************************************************//**
 * Read Line By Line From File.
 ******************************************************************************/
sl_status_t logger_sd_card_readline(const char *filename,
                                    logger_sd_card_readline_callback_t callback)
{
  FRESULT fr;
  FIL fil;
  UINT br;
  char *eol = EOL;

  fr = f_mount(&fs, MOUNT_PATH, 1);
  if (FR_OK != fr) {
    return SL_STATUS_IO;
  }

  fr = f_open(&fil, filename, FA_READ | FA_OPEN_EXISTING);
  if (FR_OK != fr) {
    goto end;
  }

  do {
    char c;
    fr = f_read(&fil, &c, 1, &br);
    if (FR_OK != fr) {
      goto close;
    }
    if (br == 0) { // End of file reached
      break;
    }
    if (strchr(eol, c)) { // End of line
      if ((line_length > 0)
          && ((line_length + EOL_LENGTH) < (int)sizeof(line_buf))) {
        memcpy(&line_buf[line_length], eol, EOL_LENGTH);
        line_length += EOL_LENGTH;
        line_buf[line_length] = '\0';
        callback(line_buf, line_length);
      }
      line_length = 0;
      continue;
    }
    if (line_length < (int)(sizeof(line_buf) - 1)) {
      line_buf[line_length++] = c;
    }
  } while (br);
  if ((line_length > 0) && (line_length < (int)sizeof(line_buf))) {
    callback(line_buf, line_length);
  }
  line_length = 0;

  close:
  f_close(&fil);

  end:
  f_unmount(MOUNT_PATH);
  return FR_OK == fr ? SL_STATUS_OK : SL_STATUS_IO;
}

sl_status_t logger_sd_card_clear_log(const char *filename)
{
  FRESULT fr;

  fr = f_mount(&fs, MOUNT_PATH, 1);
  if (FR_OK != fr) {
    return SL_STATUS_IO;
  }

  fr = f_unlink(filename);
  f_unmount(MOUNT_PATH);
  return FR_OK == fr ? SL_STATUS_OK : SL_STATUS_IO;
}
