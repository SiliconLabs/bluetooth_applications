/***************************************************************************//**
 * @file sl_sdc_media.c
 * @brief Storage Device Controls Generic Media
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
#include "ff.h"
#include "diskio.h"
#include "sl_sdc_sd_card.h"
#include "sl_sleeptimer.h"

/***************************************************************************//**
 * Get Drive Status.
 ******************************************************************************/
DSTATUS disk_status(BYTE pdrv)
{
  switch (pdrv) {
    case SD_CARD_MMC:
      return sd_card_disk_status();
      break;

    default:
      break;
  }
  return STA_NOINIT;
}

/***************************************************************************//**
 * Initialize a Drive.
 ******************************************************************************/
DSTATUS disk_initialize(BYTE pdrv)
{
  switch (pdrv) {
    case SD_CARD_MMC:
      return sd_card_disk_initialize();
      break;

    default:
      break;
  }
  return STA_NOINIT;
}

/***************************************************************************//**
 * Read Sector(s).
 ******************************************************************************/
dresult_t disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
  switch (pdrv) {
    case SD_CARD_MMC:
      return sd_card_disk_read(buff, sector, count);
      break;

    default:
      break;
  }
  return RES_PARERR;
}

/***************************************************************************//**
 * Write Sector(s).
 ******************************************************************************/
dresult_t disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
  switch (pdrv) {
    case SD_CARD_MMC:
      return sd_card_disk_write(buff, sector, count);
      break;

    default:
      break;
  }
  return RES_PARERR;
}

/***************************************************************************//**
 *  The disk_ioctl function is called to control device specific features
 *  and miscellaneous functions other than generic read/write.
 ******************************************************************************/
dresult_t disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  switch (pdrv) {
    case SD_CARD_MMC:
      return sd_card_disk_ioctl(cmd, buff);
      break;

    default:
      break;
  }
  return RES_PARERR;
}

/***************************************************************************//**
 * @brief
 *   Get current time.
 *
 * @details
 *   The get_fattime function shall return any valid time even if the system
 *   does not support a real time clock.
 *   If a zero is returned, the file will not have a valid timestamp.
 *   Currnet local time shall be returned as bit-fields packed
 *   into a DWORD value.
 *
 * @note
 *   This function is not needed when FF_FS_READONLY == 1 or FF_FS_NORTC == 1.
 *
 * @return
 *   @li @ref bit31:25 Year origin from the 1980 (0..127, e.g. 37 for 2017)
 *   @li @ref bit24:21 Month (1..12)
 *   @li @ref bit20:16 Day of the month (1..31)
 *   @li @ref bit15:11 Hour (0..23)
 *   @li @ref bit10:5  Minute (0..59)
 *   @li @ref bit4:0   Second / 2 (0..29, e.g. 25 for 50)
 ******************************************************************************/
#if !FF_FS_NORTC && !FF_FS_READONLY
DWORD get_fattime(void)
{
  static sl_sleeptimer_date_t date_time;

  // Get local time
  if (sl_sleeptimer_get_datetime(&date_time) != SL_STATUS_OK) {
    return 0;
  }

  // Pack date and time into a DWORD variable
  return ((DWORD)(date_time.year - 80) << 25) // adjust year to 1900 epoch
         | ((DWORD)(date_time.month) << 21)
         | ((DWORD)date_time.month_day << 16)
         | ((DWORD)date_time.hour << 11)
         | ((DWORD)date_time.min << 5)
         | ((DWORD)date_time.sec >> 1);
}
#endif
