/*
 * @file      default_config.c
 *
 * @brief     supports NVM and bss configuration sections:
 *            FConfig : section in NVM, where current parameters are saved and
 *   this is re-writabele.
 *
 *            Application on startup shall load_bssConfig() : this will copy
 *   FConfig -> bssConfig
 *            Accessing to variables is by pointer get_pbssConfig();
 *
 *            If application wants to re-write FConfig, use
 *   save_bssConfig(*newRamParametersBlock);
 *
 *
 * NOTE: The code is very MCU dependent and save will work with STM32F4 only
 *
 * Linker script shall define Flash sector1 ADDR_FLASH_SECTOR_1 as sections
 *
 * ".fconfig"         @0x08004000
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include <stdint.h>
#include <sys/cdefs.h>
#include "em_msc.h"
#include "default_config.h"

/* Section ".fconfig" is defined in a linker file */
//const param_block_t FConfig __attribute__((section(".fConfig"))) \
//  __attribute__((aligned(FCONFIG_SIZE))) = DEFAULT_CONFIG;

static const uint32_t FConfig[FLASH_PAGE_SIZE / sizeof(uint32_t)]
__ALIGNED(FLASH_PAGE_SIZE) __attribute__((section(".rodata")));

param_block_t const *pFConfig = (const param_block_t *)FConfig;

const param_block_t defaultFConfig = DEFAULT_CONFIG;
