/***************************************************************************//**
 * @file key_scan.c
 * @brief Key scan driver, APIs.
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
 * # EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the specified
 * dependency versions and is suitable as a demonstration for evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include <stdio.h>
#include "ir_generate.h"
#include "key_scan.h"
#include "gpiointerrupt.h"

static const key_io_t key_row_pins[KEY_ROW_NUM] = KEY_ROW_PINS;
static const key_io_t key_column_pins[KEY_COLUMN_NUM] = KEY_COLUMN_PINS;

static key_scan_t key;
static key_callback_t key_callback = 0;

const uint16_t key_map[KEY_CODE_NUM][2]=
{
  {0x7FFF, KEY_CODE01,},
  {0xBFFF, KEY_CODE02,},
  {0xDFFF, KEY_CODE03,},
  {0xEFFF, KEY_CODE04,},
  {0xF7FF, KEY_CODE05,},
  {0xFBFF, KEY_CODE06,},
  {0xFDFF, KEY_CODE07,},
  {0xFEFF, KEY_CODE08,},
  {0xFF7F, KEY_CODE09,},
  {0xFFBF, KEY_CODE10,},
  {0xFFDF, KEY_CODE11,},
  {0xFFEF, KEY_CODE12,},
  {0xFFF7, KEY_CODE13,},
  {0xFFFB, KEY_CODE14,},
  {0xFFFD, KEY_CODE15,},
  {0xFFFE, KEY_CODE16,},
  {0x3FFF, KEY_CODE17,},
  {0x77FF, KEY_CODE18,},
};

__STATIC_INLINE uint8_t key_decode(uint16_t val)
{
  uint8_t i;
  for (i = 0; i < KEY_CODE_NUM; i++) {
    if (key_map[i][0] == val) {
      return key_map[i][1];
    }
  }
  return KEY_NONE;
}

__STATIC_INLINE void key_10ms_timer(void)
{	
  //printf("key scan=%d\r\n",key.pressed);
  if (key.pressed == true) {
    if(key.value != key.last_value){
    //key change
    key.last_value = key.value;
    key.press_10ms = 0;
  }

  key.press_10ms++;

  switch (key.press_10ms) {
    case KEY_TIME_SHORT:
      key_callback(key.value);
      break;

    case KEY_TIME_CONTINUE_START:
      if ((key.value == KEY_CODE01) ||(key.value == KEY_CODE02)){
        key.press_10ms -= KEY_TIME_CONTINUE_OFFSET;
        key_callback(key.value);
      }
      break;

    case KEY_TIME_LONG:
      //printf("KEY_TIME_LONG\r\n");
      break;

    case KEY_TIME_SUPER_LONG:
      //printf("KEY_TIME_SUPER_LONG\r\n");
      break;

    case KEY_TIME_OVER:
      key.press_10ms = KEY_TIME_SUPER_LONG;
      //printf("KEY_TIME_OVER\r\n");
      break;
    }
  } else if (key.press_10ms > 0) { //Release
	key.press_10ms = 0;
	key_callback(KEY_NONE);
  }
}

/**
 * @brief Initializes key matrix.
 *
 * @param key detected callback and key wakeup callback
 *
 * @return none
 *
 */
void key_init(key_callback_t cb, key_wakeup_callback_t wakeup_cb)
{
  key.pressed = false;
  key.press_10ms = 0;
  key.value = KEY_NONE;

  key_callback = cb;

  for(uint8_t r = 0; r < KEY_ROW_NUM; r++) {
    GPIO_PinModeSet(key_row_pins[r].port, key_row_pins[r].pin, gpioModePushPull, 0);
  }

  GPIOINT_Init();

  for(uint8_t c = 0; c < KEY_COLUMN_NUM; c++) {
    GPIO_PinModeSet(key_column_pins[c].port, key_column_pins[c].pin, gpioModeInputPullFilter, 1);
    GPIO_ExtIntConfig(key_column_pins[c].port, key_column_pins[c].pin, key_column_pins[c].pin, false, true, true);
    GPIOINT_CallbackRegister(key_column_pins[c].pin, wakeup_cb);
  }
}

/**
 * @brief Implement key matrix scan
 *
 * @param none
 *
 * @return none
 *
 */
void key_scan(void)
{	
  uint32_t result = 0;

  //Prepare, set all rows high
  for (uint8_t r = 0; r< KEY_ROW_NUM; r++) {
    GPIO_PinOutSet(key_row_pins[r].port, key_row_pins[r].pin);
  }

  //Key scan main part
  for (uint8_t r = 0; r< KEY_ROW_NUM; r++) {
    GPIO_PinOutClear(key_row_pins[r].port, key_row_pins[r].pin);
    for (uint8_t c = 0; c< KEY_COLUMN_NUM; c++) {
      result |= GPIO_PinInGet(key_column_pins[c].port, key_column_pins[c].pin);
      result <<= 1;
    }
    GPIO_PinOutSet(key_row_pins[r].port, key_row_pins[r].pin);
  }

  //Key interrupt desire a low edge, so set all rows low.
  for (uint8_t r = 0; r< KEY_ROW_NUM; r++) {
    GPIO_PinOutClear(key_row_pins[r].port, key_row_pins[r].pin);
  }

  result >>= 1;

  key.value = key_decode(result);
  if (key.value == KEY_NONE) {
	key.pressed = false;
  } else {
	key.pressed = true;
  }
  key_10ms_timer();
}

