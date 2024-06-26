/**
 * @file      config.h
 *
 * @brief     Supports NVM and bss configuration sections:
 *            FConfig : section in NVM, where current parameters are saved and
 *   this is re-writabele.
 *            bssConfig : section in RAM, which is representing FConfig.
 *
 *            Application on startup shall load_bssConfig() : this will copy
 *   FConfig -> bssConfig
 *            Accessing to variables is by pointer get_pbssConfig();
 *
 *            If application wants to re-write FConfig, use
 *   save_bssConfig(*newRamParametersBlock);
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef CONFIG_H_
#define CONFIG_H_                   1

#ifdef __cplusplus
extern "C" {
#endif

#define NETBOOT_FLASH               0
#define APPLICATION_FLASH           1

typedef enum {
  Netboot_Erase,
  Config_Erase
}eFlashErase_e;

#include "default_config.h"
#include "error.h"

void load_bssConfig(void);
void restore_bssConfig(void); // require defaultFConfig
param_block_t *get_pbssConfig(void);
error_e save_bssConfig(const param_block_t *); /**< save to FConfig */

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H_ */
