/*
 * @file       config.c
 *
 * @brief      supports NVM and bss configuration sections:
 *             defaultFConfig : section in RAM, where default parameters are
 *   saved and is not re-writabele.
 *              FCONFIG_ADDR  : section in NVM, where current parameters are
 *   saved and this is re-writabele.
 *                 bssConfig  : section in RAM, which is representing config
 *   data exist in FCONFIG_ADDR.
 *
 *             application on startup shall load_bssConfig() : this will copy
 *   data from FCONFIG_ADDR -> tmpConfig
 *             Accessing to variables is by pointer get_pbssConfig();
 *
 *             if application wants to re-write data in FCONFIG_ADDR, use
 *   save_bssConfig(*newRamParametersBlock);
 *
 *             NOTE: The code is very MCU dependent and save will work with
 *   nRF52840 only
 *
 * @author     Decawave Software
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 */

#include <stdint.h>
#include <string.h>

#include "em_msc.h"
#include "sl_common.h"

#include "deca_device_api.h"
#include "version.h"

#include "config.h"

#define STATIC_ASSERT(condition) typedef char assert[(condition) ? 1 : -1]

// ------------------------------------------------------------------------------
extern const param_block_t FConfig;
extern const param_block_t defaultFConfig;

extern uint32_t __fconfig_start[];
extern uint32_t __fconfig_end[];

/* run-time parameters block.
 *
 * This is the RAM image of the FCONFIG_ADDR .
 *
 * Accessible from application by get_pbssConfig function after load_bssConfig()
 *
 * */
static param_block_t tmpConfig;

// ------------------------------------------------------------------------------
// Size checks
STATIC_ASSERT(sizeof(tmpConfig) == FCONFIG_SIZE);
STATIC_ASSERT(sizeof(FConfig) == FCONFIG_SIZE);
STATIC_ASSERT(sizeof(defaultFConfig) == FCONFIG_SIZE);
STATIC_ASSERT(sizeof(param_block_t) == FCONFIG_SIZE);
STATIC_ASSERT(FCONFIG_SIZE <= FLASH_PAGE_SIZE);

// ------------------------------------------------------------------------------
// Implementation

/*
 * @brief get pointer to run-time bss param_block_t block
 *
 * */
param_block_t *get_pbssConfig(void)
{
  return &tmpConfig;
}

/* @fn      load_bssConfig
 * @brief   copy parameters from NVM to RAM structure.
 *
 *          assumes that memory model in the MCU of .text and .bss are the same
 * */
void load_bssConfig(void)
{
  memcpy(&tmpConfig, &FConfig, FCONFIG_SIZE);
}

/* @fn      restore_bssConfig
 * @brief   copy parameters from default NVM section to RAM structure.
 *
 *          assumes that memory model in the MCU of .text and .bss are the same
 * */
void restore_bssConfig(void)
{
  save_bssConfig(&defaultFConfig);
  load_bssConfig();
}

/* @brief    save pNewRamParametersBlock to FCONFIG_ADDR
 * @return  _NO_ERR for success and error_e code otherwise
 * */
error_e save_bssConfig(const param_block_t *pNewRamParametersBlock)
{
  MSC_Init();
  int sts = MSC_ErasePage(__fconfig_start);
  sts |=
    MSC_WriteWord(__fconfig_start,
                  pNewRamParametersBlock,
                  sizeof(*pNewRamParametersBlock));
  MSC_Deinit();

  return (sts ? _ERR_Flash_Prog : _NO_ERR);
}
