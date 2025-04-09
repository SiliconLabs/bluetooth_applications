/***************************************************************************//**
 * @file lcd.h
 * @brief LCD Display functions
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: LicenseRef-MSLA
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement
 * By installing, copying or otherwise using this software, you agree to the
 * terms of the MSLA.
 *
 ******************************************************************************/

#ifndef LCD__H
#define LCD__H

#include "sl_status.h"

#ifndef DISABLE_LCD

sl_status_t lcd_init(void);
void lcd_clear_row(unsigned int row);
void lcd_write_row(unsigned int row, const char *str, int len);
void lcd_update(void);

#else

static inline sl_status_t lcd_init(void)
{
  return SL_STATUS_OK;
}

static inline void lcd_clear_row(unsigned int row)
{
  (void)row;
}

static inline void lcd_write_row(unsigned int row, const char *str, int len)
{
  (void)row;
  (void)str;
  (void)len;
}

static inline void lcd_update(void)
{
}

#endif

#endif
