/***************************************************************************//**
 * @file switch_nvm.h
 * @brief switch nvm header file.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef SWITCH_NVM_H_
#define SWITCH_NVM_H_

#include "sl_status.h"
#include "nvm3_default.h"
#include "nvm3_default_config.h"

#define SWITCH_STATE_KEY 0

/***************************************************************************//**
 * APP nvm3 initializes.
 * brief: nvm3 initialization
 * paramater: null
 * return: null
 ******************************************************************************/
void switch_nvm3_init(void);

/***************************************************************************//**
 * APP get switch state.
 * brief: read switch state from nvm3
 * parameter: [out]state: switch state
 * return: SL_STATUS_OK if success or SL_STATUS_FAIL if error.
 ******************************************************************************/
sl_status_t nvm3_get_switch_state(uint8_t *state);

/***************************************************************************//**
 * App save switch state.
 * brief: write switch state to nvm3
 * parameter: [in] state: switch state
 * return: SL_STATUS_OK if success or SL_STATUS_FAIL if error.
 ******************************************************************************/
sl_status_t nvm3_save_switch_state(uint8_t state);

#endif /* SWITCH_NVM_H_ */
