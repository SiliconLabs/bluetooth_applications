/*
 * This file is part of VL53L1 Platform
 *
 * Copyright (c) 2016, STMicroelectronics - All Rights Reserved
 *
 * License terms: BSD 3-clause "New" or "Revised" License.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file  vl53l1_platform.h
 * @brief Those platform functions are platform dependent and have
 *        to be implemented for the integrated platform.
 */

#ifndef _VL53L1_PLATFORM_H_
#define _VL53L1_PLATFORM_H_

#include <stdint.h>
#include "sl_status.h"
#include "sl_i2cspm.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief
 *    This function sets the IC2SPM instance used by platform functions.
 *
 * @param[in] i2cspm_instance
 *    I2CSPM instance, default: VL53L1X_CONFIG_I2C_INSTANCE
 *
 ******************************************************************************/
void vl53l1x_platform_set_i2cspm_instance(sl_i2cspm_t *i2cspm_instance);

/** @brief VL53L1_ReadMulti() definition.\n

 */
sl_status_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata,
		uint32_t count);

/** @brief VL53L1_WrByte() definition.\n
 */
sl_status_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data);

/** @brief vl53l1_write_word() definition.\n
 */
sl_status_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data);

/** @brief VL53L1_WrWord() definition.\n
 */
sl_status_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data);

/** @brief VL53L1_RdByte() definition.\n
 */
sl_status_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *pdata);

/** @brief VL53L1_RdWord() definition.\n
 */
sl_status_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *pdata);

/** @brief VL53L1_RdDWord() definition.\n
 */
sl_status_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *pdata);

#ifdef __cplusplus
}
#endif

#endif
