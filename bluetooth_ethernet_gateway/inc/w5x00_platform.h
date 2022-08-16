/***************************************************************************//**
 * @file w5x00_platform.h
 * @brief Wiznet w5x00 platform.
 * @version 0.0.1
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

#ifndef W5x00_PLATFORM_H_
#define W5x00_PLATFORM_H_

#include "sl_status.h"
#include "sl_udelay.h"
#include "w5x00_config.h"
#include "sl_sleeptimer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup W5x00_platform W5x00 Ethernet Driver Platform */

/**
 * @addtogroup W5x00_platform
 * @brief  W5x00 Ethernet Driver Platform.
 * @details
 * @{
 */

// -----------------------------------------------------------------------------
// Logging
/***************************************************************************//**
 * @def w5x00_log_info(tag, fmt, ...)
 * Print info log to the console
 * @def w5x00_log_error(tag, fmt, ...)
 * Print error log to the console
 ******************************************************************************/
#ifdef W5x00_LOG_ENABLE
#define w5x00_log_info(tag, fmt, ...)   \
  w5x00_log_printf("[ I: %s ] " fmt, tag, ##__VA_ARGS__)
#define w5x00_log_error(tag, fmt, ...)  \
  w5x00_log_printf("[ E: %s ] " fmt, tag, ##__VA_ARGS__)
#define w5x00_log_print_ip(ip)                                     \
  w5x00_log_printf("%d.%d.%d.%d", w5x00_ip4_addr_get_byte(ip, 0),  \
                                w5x00_ip4_addr_get_byte(ip, 1),    \
                                w5x00_ip4_addr_get_byte(ip, 2),    \
                                w5x00_ip4_addr_get_byte(ip, 3))
#else
#define w5x00_log_printf(...)
#define w5x00_log_info(...)
#define w5x00_log_error(...)
#define w5x00_log_print_ip(ip)
#endif // #ifdef W5x00_LOG_ENABLE

/***************************************************************************//**
 * @brief
 *    Delay microseconds
 * @param[in] us
 *    Microseconds
 ******************************************************************************/
#define w5x00_delay_us(us) \
    sl_udelay_wait(us)

/***************************************************************************//**
 * @brief
 *    Delay milliseconds
 * @param[in] ms
 *    Milliseconds
 ******************************************************************************/
#define  w5x00_delay_ms(ms) \
    sl_sleeptimer_delay_millisecond(ms)

/***************************************************************************//**
 * @brief
 *    Get curent tick count
 * @return
 *    Current tick count
 ******************************************************************************/
#define w5x00_get_tick_count() \
    sl_sleeptimer_get_tick_count()

/***************************************************************************//**
 * @brief
 *    Get current tick count in milliseconds unit
 * @return
 *    Current tick count in milliseconds
 ******************************************************************************/
#define w5x00_get_tick_ms() \
    sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count())

/***************************************************************************//**
 * @brief
 *    Hard reset chip
 ******************************************************************************/
void  w5x00_reset(void);

/***************************************************************************//**
 * @brief
 *    Generate random number in range
 * @param[in] howsmall
 * @param[in] howbig
 * @return
 *    Random number
 ******************************************************************************/
long w5x00_random2(long howsmall, long howbig);

/***************************************************************************//**
 * @brief
 *    Generate random number
 * @param howbig
 * @return
 *    Random number
 ******************************************************************************/
long w5x00_random(long howbig);

/***************************************************************************//**
 * @brief
 *    Init platform bus
 ******************************************************************************/
void  w5x00_bus_init(void);

/***************************************************************************//**
 * @brief
 *    Select chip
 ******************************************************************************/
void  w5x00_bus_select(void);

/***************************************************************************//**
 * @brief
 *    Deselect chip
 ******************************************************************************/
void  w5x00_bus_deselect(void);

/***************************************************************************//**
 * @brief
 *    Read from SPI bus
 * @param[out] buf
 *    Pointer to the read buffer
 * @param len
 *    Number of byte to be read
 * @return
 *    0 on success
 *    non-zero on failure
 ******************************************************************************/
uint32_t  w5x00_bus_read(uint8_t* buf, uint16_t len);

/***************************************************************************//**
 * @brief
 *    Write to SPI bus
 * @param[in] buf
 *    Pointer to the written buffer
 * @param len
 *    Number of byte to be written
 * @return
 *    0 on success
 *    non-zero on failure
 ******************************************************************************/
uint32_t  w5x00_bus_write(const uint8_t* buf, uint16_t len);

/** @} (end group W5x00_platform) */
#ifdef __cplusplus
}
#endif
#endif // W5x00_PLATFORM_H_
