/***************************************************************************//**
 * @file ak9753_platform.h
 * @brief AK9753 Platform
 * @version 1.0.0
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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

#ifndef _AK9753_PLATFORM_H_
#define _AK9753_PLATFORM_H_

#include <stdint.h>
#include "sl_status.h"
#include "sl_i2cspm.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ak9753_platform AK9753 Driver Platform */

/***************************************************************************//**
 * @addtogroup ak9753_platform
 * @brief  AK9753 Driver Platform
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    This function sets the IC2SPM instance used by platform functions.
 *
 * @param[in] i2cspm_instance
 *    I2CSPM instance, default: AK9753_CONFIG_I2C_INSTANCE
 *
 ******************************************************************************/
void ak9753_platform_set_i2cspm_instance(sl_i2cspm_t *i2cspm_instance);

/***************************************************************************//**
 * @brief
 *    This function is used to read a register.
 *
 * @param[in] addr
 *    Address of register
 * @param[out] pdata
 *    Pointer of output value
 *
 * @return sl_status_t SL_STATUS_OK on success or SL_STATUS_TRANSMIT on failure
 *
 ******************************************************************************/
sl_status_t ak9753_platform_read_register(uint8_t addr,
                                          uint8_t *pdata);

/***************************************************************************//**
 * @brief
 *    This function is used to write a register.
 *
 * @param[in] addr
 *    Address of register
 * @param[out] data
 *    input value
 *
 * @return sl_status_t SL_STATUS_OK on success or SL_STATUS_TRANSMIT on failure
 *
 ******************************************************************************/
sl_status_t ak9753_platform_write_register(uint8_t addr,
                                           uint8_t data);

/***************************************************************************//**
 * @brief
 *    This function is used to write a register.
 *
 * @param[in] addr
 *    Address of register
 * @param[out] pdata
 *    Pointer of output value
 * @param[in] len
 *    Length of data to read
 *
 * @return sl_status_t SL_STATUS_OK on success or SL_STATUS_TRANSMIT on failure
 *
 ******************************************************************************/
sl_status_t ak9753_platform_read_blocking_register(uint8_t addr,
                                                   uint8_t *pdata,
                                                   uint8_t len);

/** @} (end addtogroup ak9753_platform) */

#ifdef __cplusplus
}
#endif

#endif /* AK9753_PLATFORM_H */
