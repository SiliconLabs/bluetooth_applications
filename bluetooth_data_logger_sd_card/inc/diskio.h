/***************************************************************************//**
 * @file diskio.h
 * @brief Low level disk interface module include file
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
#ifndef DISKIO_H
#define DISKIO_H

#include "ff.h"

// Definitions of physical drive number for each drive
#define SD_CARD_MMC       0 // Map MMC/SD card to physical drive 0

// Disk Status Bits (DSTATUS)
#define STA_NOINIT        0x01  // Drive not initialized
#define STA_NODISK        0x02  // No medium in the drive
#define STA_PROTECT       0x04  // Write protected

// Command code for disk_ioctrl fucntion
// Generic command (Used by FatFs)
#define CTRL_SYNC         0 // Complete pending write process (needed at FF_FS_READONLY == 0)
#define GET_SECTOR_COUNT  1 // Get media size (needed at FF_USE_MKFS == 1)
#define GET_SECTOR_SIZE   2 // Get sector size (needed at FF_MAX_SS != FF_MIN_SS)
#define GET_BLOCK_SIZE    3 // Get erase block size (needed at FF_USE_MKFS == 1)
#define CTRL_TRIM         4 // Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1)

// MMC/SDC specific ioctl command
#define MMC_GET_TYPE      10  // Get card type
#define MMC_GET_CSD       11  // Get CSD
#define MMC_GET_CID       12  // Get CID
#define MMC_GET_OCR       13  // Get OCR
#define MMC_GET_SDSTAT    14  // Get SD status
#define ISDIO_READ        55  // Read data form SD iSDIO register
#define ISDIO_WRITE       56  // Write data to SD iSDIO register
#define ISDIO_MRITE       57  // Masked write data to SD iSDIO register

// ATA/CF specific ioctl command
#define ATA_GET_REV       20  // Get F/W revision
#define ATA_GET_MODEL     21  // Get model name
#define ATA_GET_SN        22  // Get serial number

// MMC card type flags (MMC_GET_TYPE)
#define CT_MMC3           0x01    // MMC ver 3
#define CT_MMC4           0x02    // MMC ver 4+
#define CT_MMC            0x03    // MMC
#define CT_SDC1           0x02    // SDC ver 1
#define CT_SDC2           0x04    // SDC ver 2+
#define CT_SDC            0x0C    // SDC
#define CT_BLOCK          0x10    // Block addressing

// Status of Disk Functions
typedef BYTE DSTATUS;

/// Results of Disk Functions
typedef enum {
  RES_OK = 0,  ///< 0: Successful.
  RES_ERROR,   ///< 1: R/W Error.
  RES_WRPRT,   ///< 2: Write Protected.
  RES_NOTRDY,  ///< 3: Not Ready.
  RES_PARERR   ///< 4: Invalid Parameter.
} dresult_t;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief
 *   Initialize a Drive.
 *
 * @param[in] pdrv
 *   Physical drive number to identify the drive
 *
 * @return Status of Disk Functions
 ******************************************************************************/
DSTATUS disk_initialize(BYTE pdrv);

/***************************************************************************//**
 * @brief
 *   Get Drive Status.
 *
 * @param[in] pdrv
 *   Physical drive number to identify the drive
 *
 * @return Status of Disk Functions
 ******************************************************************************/
DSTATUS disk_status(BYTE pdrv);

/***************************************************************************//**
 * @brief
 *   Read Sector(s).
 *
 * @param[in] pdrv
 *   Physical drive number to identify the drive
 *
 * @param[out] buff
 *   Pointer to the Data buffer to store read data
 *
 * @param[in] sector
 *   Start sector in LBA
 *
 * @param[in] count
 *   Number of sectors to read
 *
 * @return Status of Disk Functions
 ******************************************************************************/
dresult_t disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);

/***************************************************************************//**
 * @brief
 *   Write Sector(s).
 *
 * @param[in] pdrv
 *   Physical drive number to identify the drive
 *
 * @param[in] buff
 *   Pointer to the Data buffer to be written
 *
 * @param[in] sector
 *   Start sector in LBA
 *
 * @param[in] count
 *   Number of sectors to write
 *
 * @return Status of Disk Functions
 ******************************************************************************/
dresult_t disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);

/***************************************************************************//**
 * @brief
 *   The disk_ioctl function is called to control device specific features
 *   and miscellaneous functions other than generic read/write.
 *
 * @param[in] pdrv
 *   Physical drive number to identify the drive
 *
 * @param[in] cmd
 *   Control code
 *
 * @param[in] buff
 *   Buffer to send/receive control data
 *
 * @return Status of Disk Functions
 ******************************************************************************/
dresult_t disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);

/***************************************************************************//**
 * Device timer function.
 * This function must be called from timer interrupt routine in period
 * of 1 ms to generate card control timing.
 ******************************************************************************/
void disk_timerproc(void);

#endif // DISKIO_H

#ifdef __cplusplus
}
#endif
