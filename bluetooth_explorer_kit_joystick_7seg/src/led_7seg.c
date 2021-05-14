/***************************************************************************//**
 * @file led_7seg.c
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
#include <led_7seg.h>
#include "em_cmu.h"
#include "em_gpio.h"
#include "spidrv.h"
#include "sl_spidrv_instances.h"

#define spi_handle    sl_spidrv_mikroe_handle

static const uint8_t number_array[16] =
{
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F,  // 9
    0x77,  // A
    0x7C,  // B
    0x39,  // C
    0x5E,  // D
    0x79,  // E
    0x71   // F
};

/***************************************************************************//**
 * @brief
 *   Initialize led_7seg_init.
 *
 * @detail
 *  The SPI driver instances will be initialized automatically,
 *  during the sl_system_init() call in main.c.
 *****************************************************************************/
sl_status_t led_7seg_init(void)
{
  sl_status_t sc;

  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Setup GPIOs */
  /* Configure ENABLE pin as an output and drive inactive high to turn ON display */
  GPIO_PinModeSet(LED_7SEG_ENABLE_GPIO_PORT, LED_7SEG_ENABLE_GPIO_PIN, gpioModePushPull, 1);
  /* display number 0 */
  sc = led_7seg_display_number(0);

  return sc;
}

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
sl_status_t led_7seg_display_number(uint8_t number)
{
  Ecode_t ret;
  uint8_t right_number;
  uint8_t left_number;
  uint8_t data_buf[2];

  number %= 100;

  left_number = number / 10;
  right_number = number % 10;

  if (left_number == 0) {
    data_buf[0] = number_array[right_number];
    data_buf[1] = number_array[0];
  }
  else {
    data_buf[0] = number_array[right_number];
    data_buf[1] = number_array[left_number];
  }

  /* Send a block transfer to slave. */
  ret = SPIDRV_MTransmitB(spi_handle, data_buf, 2);
  if (ret != ECODE_EMDRV_SPIDRV_OK) {
    return SL_STATUS_FAIL;
  }

  /* Note that at this point all the data is loaded into the fifo, this does
   * not necessarily mean that the transfer is complete. */
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Turn on display.
 *
 *****************************************************************************/
void led_7seg_display_on(void)
{
  GPIO_PinOutSet(LED_7SEG_ENABLE_GPIO_PORT, LED_7SEG_ENABLE_GPIO_PIN);
}

/***************************************************************************//**
 * @brief
 *   Turn off display.
 *
 *****************************************************************************/
void led_7seg_display_off(void)
{
  GPIO_PinOutClear(LED_7SEG_ENABLE_GPIO_PORT, LED_7SEG_ENABLE_GPIO_PIN);
}
