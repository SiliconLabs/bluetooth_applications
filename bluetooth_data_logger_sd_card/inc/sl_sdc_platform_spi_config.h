/***************************************************************************//**
 * @file sl_sdc_platform_spi_config.h
 * @brief Platform SPI Driver Configuration
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
#ifndef SL_SDC_PLATFORM_SPI_CONFIG_H
#define SL_SDC_PLATFORM_SPI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_spidrv_instances.h"
#include "sl_spidrv_exp_config.h"

#define sdc_spi_handle                      sl_spidrv_exp_handle

#define SD_CARD_MMC_PERIPHERAL              SL_SPIDRV_EXP_PERIPHERAL
#define SD_CARD_MMC_PERIPHERAL_NO           SL_SPIDRV_EXP_PERIPHERAL_NO

// Set slow clock for card initialization (100k-400k)
#define SD_CARD_MMC_SLOW_CLOCK              200000
// Set fast clock for generic read/write
#define SD_CARD_MMC_FAST_CLOCK              SL_SPIDRV_EXP_BITRATE

// USART0 CS on PC03
#define SD_CARD_MMC_CS_PORT                 SL_SPIDRV_EXP_CS_PORT
#define SD_CARD_MMC_CS_PIN                  SL_SPIDRV_EXP_CS_PIN

// USART0 CS on PB00
#define SD_CARD_MMC_CD_PORT                 gpioPortA
#define SD_CARD_MMC_CD_PIN                  6

#ifdef __cplusplus
}
#endif

#endif /* SL_SDC_PLATFORM_SPI_CONFIG_H_ */
