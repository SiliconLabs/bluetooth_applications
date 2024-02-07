/***************************************************************************//**
 * @file key_scan.h
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
 * This code has been minimally tested to ensure that it builds with the
 *   specified
 * dependency versions and is suitable as a demonstration for evaluation
 *   purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 ******************************************************************************/

#ifndef _KEYSCAN_H

/***************************************************************************//**
 * @addtogroup Key Pad Driver
 * @brief Key Pad Driver
 *  KEY_ROW_PINS, KEY_COLUMN_PINS define the key detect pin,
 *  Initialize with key_init(). Put key_scan() in a 10ms time slice.
 * @{
 ******************************************************************************/

#define _KEYSCAN_H

/*****************   INCLUDES
 *    **********************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "em_gpio.h"
#include "ir_generate.h"
#include "gpiointerrupt.h"

/***************** END OF INCLUDES
 *   *****************************************************/

/*****************   DEFINITIONS
 *    *****************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_ROW_NUM       4
#define KEY_COLUMN_NUM    4

#define KEY_ROW_PINS      { { gpioPortB, 0 }, { gpioPortB, 1 }, \
                            { gpioPortB, 2 }, { gpioPortB, 3 } }
#define KEY_COLUMN_PINS   { { gpioPortC, 0 }, { gpioPortC, 1 }, \
                            { gpioPortC, 2 }, { gpioPortC, 3 } }

typedef struct {
  GPIO_Port_TypeDef   port;
  unsigned int        pin;
} key_io_t;

typedef enum {
  KEY_TIME_SHORT       = 5,
  KEY_TIME_CONTINUE_OFFSET = 30,
  KEY_TIME_CONTINUE_START  = 80,
  KEY_TIME_LONG        = 200,
  KEY_TIME_SUPER_LONG    = 1000,
  KEY_TIME_OVER
}key_time_t;

typedef enum {
  KEY_NONE,
  KEY_CODE01,
  KEY_CODE02,
  KEY_CODE03,
  KEY_CODE04,
  KEY_CODE05,
  KEY_CODE06,
  KEY_CODE07,
  KEY_CODE08,
  KEY_CODE09,
  KEY_CODE10,
  KEY_CODE11,
  KEY_CODE12,
  KEY_CODE13,
  KEY_CODE14,
  KEY_CODE15,
  KEY_CODE16,
  KEY_CODE17,
  KEY_CODE18,
  KEY_CODE_NUM
}key_code_t;

/***************** END OF DEFINITIONS
 *   ***************************************************/

/*****************    ENUM
 *    /STRUCT******************************************************/
typedef struct {
  uint8_t value;
  uint8_t last_value;
  uint16_t press_10ms;
  bool pressed;
}key_scan_t;

typedef void (*key_callback_t)(key_code_t key);
typedef void (*key_wakeup_callback_t)(uint8_t pin);

/**
 * @brief Initializes key matrix.
 *
 * @param none
 *
 * @return none
 *
 */
extern void key_init(key_callback_t cb, key_wakeup_callback_t wakeup_cb);

/**
 * @brief Implement key matrix scan
 *
 * @param none
 *
 * @return none
 *
 */
extern void key_scan(void);

#ifdef __cplusplus
}
#endif

/** @} (end addtogroup Key Pad Driver) */
#endif
