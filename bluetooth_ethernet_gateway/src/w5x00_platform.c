/***************************************************************************//**
 * @file w5x00_platform.c
 * @brief Wiznet w5x00 platform implementation.
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
#include <stdint.h>
#include <stdlib.h>

#include "w5x00_platform.h"

/***************************************************************************//**
 * Reset Chip.
 ******************************************************************************/
void w5x00_reset(void)
{
  GPIO_PinOutClear(W5x00_RESET_GPIO_PORT, W5x00_RESET_GPIO_PIN);
  w5x00_delay_ms(100);
  GPIO_PinOutSet(W5x00_RESET_GPIO_PORT, W5x00_RESET_GPIO_PIN);
}

/***************************************************************************//**
 * Initialize Bus IO.
 ******************************************************************************/
void w5x00_bus_init(void)
{
  GPIO_PinModeSet(W5x00_RESET_GPIO_PORT,
                  W5x00_RESET_GPIO_PIN,
                  gpioModePushPull,
                  0);
  GPIO_PinModeSet((GPIO_Port_TypeDef)w5x00_spi_handle->portCs,
                  w5x00_spi_handle->pinCs,
                  gpioModePushPull, 1);
}

/***************************************************************************//**
 * Select Chip.
 ******************************************************************************/
void w5x00_bus_select(void)
{
  GPIO_PinOutClear((GPIO_Port_TypeDef)w5x00_spi_handle->portCs,
                   w5x00_spi_handle->pinCs);
}

/***************************************************************************//**
 * Deselect Chip.
 ******************************************************************************/
void w5x00_bus_deselect(void)
{
  GPIO_PinOutSet((GPIO_Port_TypeDef)w5x00_spi_handle->portCs,
                 w5x00_spi_handle->pinCs);
}

/***************************************************************************//**
 * Read Chip Data From SPI Interface.
 ******************************************************************************/
uint32_t w5x00_bus_read(uint8_t *buf, uint16_t len)
{
  Ecode_t ret = SPIDRV_MReceiveB(w5x00_spi_handle, buf, len);
  return (uint32_t)ret;
}

/***************************************************************************//**
 * Write Data To Chip.
 ******************************************************************************/
uint32_t w5x00_bus_write(const uint8_t *buf, uint16_t len)
{
  Ecode_t ret = SPIDRV_MTransmitB(w5x00_spi_handle, buf, len);
  return (uint32_t)ret;
}

/***************************************************************************//**
 * Generate Random Number In Range.
 ******************************************************************************/
long w5x00_random(long howbig)
{
  uint32_t x = (uint32_t)rand();
  uint64_t m = (uint64_t)x * (uint64_t)howbig;
  uint32_t l = (uint32_t)m;

  if (l < (uint32_t)howbig) {
    uint32_t t = (uint32_t)-howbig;
    if (t >= (uint32_t)howbig) {
      t -= howbig;
      if (t >= (uint32_t)howbig) {
        t %= howbig;
      }
    }
    while (l < t) {
      x = (uint32_t)rand();
      m = (uint64_t)x * (uint64_t)howbig;
      l = (uint32_t)m;
    }
  }
  return (long)(m >> 32);
}

/***************************************************************************//**
 * Generate Random Number.
 ******************************************************************************/
long w5x00_random2(long howsmall, long howbig)
{
  long diff = howbig - howsmall;

  if (howsmall >= howbig) {
    return howsmall;
  }

  return w5x00_random(diff) + howsmall;
}
