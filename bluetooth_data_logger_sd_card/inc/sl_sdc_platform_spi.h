/***************************************************************************//**
 * @file sl_sdc_platform_spi.h
 * @brief Storage Device Controls SD Card platform include header
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
#ifndef SL_SDC_PLATFORM_SPI_H
#define SL_SDC_PLATFORM_SPI_H

#include "em_gpio.h"

#include "ff.h"
#include "sl_sdc_sd_card.h"
#include "sl_sdc_platform_spi_config.h"
#include "sl_sleeptimer.h"

// Socket controls
// MMC CS = L
#define CS_LOW()      GPIO_PinOutSet(SD_CARD_MMC_CS_PORT, SD_CARD_MMC_CS_PIN)
// MMC CS = H
#define CS_HIGH()     GPIO_PinOutClear(SD_CARD_MMC_CS_PORT, SD_CARD_MMC_CS_PIN)
// Card detected      (yes:true, no:false, default:true)
#define MMC_CD        (!GPIO_PinInGet(SD_CARD_MMC_CD_PORT, SD_CARD_MMC_CD_PIN))

// SPI bit rate controls
// Set slow clock for card initialization (100k-400k)
#define FCLK_SLOW()   SPIDRV_SetBitrate(sdc_spi_handle, SD_CARD_MMC_SLOW_CLOCK)
// Set fast clock for generic read/write
#define FCLK_FAST()   SPIDRV_SetBitrate(sdc_spi_handle, SD_CARD_MMC_FAST_CLOCK)

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief
 *   Initialize platform spi.
 *
* @return SL_STATUS_OK if successful. Error code otherwise.
 ******************************************************************************/
  sl_status_t sdc_platform_spi_init(void);

/***************************************************************************//**
 * @brief
 *   Exchange a byte.
 *
 * @param[out] data
 *   Pointer to the data buffer to be exchange
 *
 * @return
 *   @ref SL_STATUS_OK on success or @ref SL_STATUS_TRANSMIT on failure
 ******************************************************************************/
sl_status_t sdc_xchg_spi(BYTE tx, BYTE *rx);

/***************************************************************************//**
 * @brief
 *   Multi-byte SPI transaction (transmit).
 *
 * @param[in] buff
 *   Pointer to the data buffer to be sent
 *
 * @param[in] cnt
 *   Number of bytes to send
 *
 * @return
 *   @ref SL_STATUS_OK on success or @ref SL_STATUS_TRANSMIT on failure
 ******************************************************************************/
sl_status_t sdc_xmit_spi_multi(const BYTE *buff, UINT cnt);

/***************************************************************************//**
 * @brief
 *   Multi-byte SPI transaction (receive).
 *
 * @param[out] buff
 *   Pointer to the data Buffer to store received data
 *
 * @param[in] cnt
 *   Number of bytes to receive
 *
 * @return
 *   @ref SL_STATUS_OK on success or @ref SL_STATUS_TRANSMIT on failure
 ******************************************************************************/
sl_status_t sdc_rcvr_spi_multi(BYTE *buff, UINT cnt);

#ifdef __cplusplus
}
#endif

#endif // SL_SDC_PLATFORM_SPI_H
