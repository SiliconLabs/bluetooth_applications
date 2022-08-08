/***************************************************************************//**
 * @file sl_sdc_platform_spi.c
 * @brief Storage Device Controls SD Card platform
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include "sl_sdc_platform_spi.h"

static sl_sleeptimer_timer_handle_t disk_timerproc_timer_handle;
static void disk_timerproc_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                          void *data);

/***************************************************************************//**
 * Initialize platform spi.
 ******************************************************************************/
sl_status_t sdc_platform_spi_init(void)
{
  sl_status_t status;

  GPIO_PinModeSet(SD_CARD_MMC_CD_PORT,
                  SD_CARD_MMC_CD_PIN,
                  gpioModeInputPullFilter,
                  1);

  // Start a periodic timer 1 ms to generate card control timing
  status = sl_sleeptimer_start_periodic_timer_ms(&disk_timerproc_timer_handle,
                                                 1,
                                                 disk_timerproc_timer_callback,
                                                 (void *)NULL,
                                                 0,
                                                 0);
  return status;
}

/***************************************************************************//**
 * Exchange a byte.
 ******************************************************************************/
sl_status_t sdc_xchg_spi(BYTE tx, BYTE *rx)
{
  sl_status_t retval;

  retval = SPIDRV_MTransferB(sdc_spi_handle, &tx, rx, 1);
  if (retval != ECODE_EMDRV_SPIDRV_OK) {
    *rx = 0;
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Multi-byte SPI transaction (transmit).
 ******************************************************************************/
sl_status_t sdc_xmit_spi_multi(const BYTE *buff, UINT cnt)
{
  sl_status_t retval;

  retval = SPIDRV_MTransmitB(sdc_spi_handle, buff, cnt);
  if (retval != ECODE_EMDRV_SPIDRV_OK) {
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Multi-byte SPI transaction (receive).
 ******************************************************************************/
sl_status_t sdc_rcvr_spi_multi(BYTE *buff, UINT cnt)
{
  sl_status_t retval;

  retval = SPIDRV_MTransferB(sdc_spi_handle, buff, buff, cnt);
  if (retval != ECODE_EMDRV_SPIDRV_OK) {
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Sleeptimer callback function to generate card control timing.
 ******************************************************************************/
static void disk_timerproc_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                          void *data)
{
  (void)handle;
  (void)data;

  disk_timerproc();
}
