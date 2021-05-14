/***************************************************************************//**
 * @file led_7seg.h
 * @brief LED 7-segment driver
 *******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
* This code has been minimally tested to ensure that it builds with the specified
* dependency versions and is suitable as a demonstration for evaluation purposes only.
* This code will be maintained at the sole discretion of Silicon Labs.
*
******************************************************************************/

#ifndef LED_7SEG_H
#define LED_7SEG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_status.h"

#define LED_7SEG_ENABLE_GPIO_PORT                   gpioPortB
#define LED_7SEG_ENABLE_GPIO_PIN                    4

/***************************************************************************//**
 * @brief
 *   Initialize led_7seg_init.
 *
 * @detail
 *  The SPI driver instances will be initialized automatically,
 *  during the sl_system_init() call in main.c.
 *****************************************************************************/
sl_status_t led_7seg_init(void);

/***************************************************************************//**
 * @brief
 *    Write a number on LED 7-segment display via SPI interface.
 *
 * @note
 *    The data received on the MISO wire is discarded.
 *    @n This function is blocking and returns when the transfer is complete.
 *
 * @param[in] number
 *    Transmit number that will display on the 7-segment.
 *
 * @return
 *    @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure
 ******************************************************************************/
sl_status_t led_7seg_display_number(uint8_t number);

/***************************************************************************//**
 * @brief
 *   Turn on display.
 *
 *****************************************************************************/
void led_7seg_display_on(void);

/***************************************************************************//**
 * @brief
 *   Turn off display.
 *
 *****************************************************************************/
void led_7seg_display_off(void);

#ifdef __cplusplus
}
#endif

#endif
