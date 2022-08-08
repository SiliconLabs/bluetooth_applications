/***************************************************************************//**
 * @file sl_sdc_sd_card.h
 * @brief Storage Device Controls SD Card include file
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
#ifndef SL_SDC_SD_CARD_H
#define SL_SDC_SD_CARD_H

#include "stdint.h"

#include "ff.h"
#include "diskio.h"

#include "sl_sdc_platform_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief Initialize an SD Card.
 *
 * @return Status of Disk Functions
 ******************************************************************************/
DSTATUS sd_card_disk_initialize(void);

/***************************************************************************//**
 * @brief Get SD Card Status.
 *
 * @return Status of Disk Functions
 ******************************************************************************/
DSTATUS sd_card_disk_status(void);

/***************************************************************************//**
 * @brief
 *   Read Sector(s) from SD Card.
 *
 * @param[out] buff
 *   Pointer to the Data buffer to be read
 *
 * @param[in] sector
 *   Start sector in LBA
 *
 * @param[in] count
 *   Number of sectors to read
 *
 * @return Status of Disk Functions
 ******************************************************************************/
dresult_t sd_card_disk_read(BYTE* buff, LBA_t sector, UINT count);

/***************************************************************************//**
 * @brief
 *   Write Sector(s) to SD Card.
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
dresult_t sd_card_disk_write(const BYTE* buff, LBA_t sector, UINT count);

/***************************************************************************//**
 * @brief
 *   Miscellaneous Functions.
 *   This function is called to control device specific features 
 *   and miscellaneous functions other than generic read/write.
 *
 * @param[in] cmd
 *   Control code
 *
 * @param[in] buff
 *   Buffer to send/receive control data
 *
 * @return Status of Disk Functions
 ******************************************************************************/
dresult_t sd_card_disk_ioctl(BYTE cmd, void* buff);

#ifdef __cplusplus
}
#endif

#endif // SL_SDC_SD_CARD_H
