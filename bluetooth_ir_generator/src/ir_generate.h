/***************************************************************************//**
 * @file ir_generate.h
 * @brief IR Generator driver, APIs.
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

#ifndef __IRGENELATE_H__

/***************************************************************************//**
 * @addtogroup IR Generator Driver
 * @brief IR Generator Driver
 *   . . . enter description here
 * @{
 ******************************************************************************/

#define __IRGENELATE_H__
#include "em_gpio.h"
#include "em_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------
  define / Typedef
  ----------------------------------------------*/
#define BSP_CARRIER_PORT	gpioPortD
#define BSP_CARRIER_PIN		2

#define BSP_MODULATION_PORT	gpioPortD
#define BSP_MODULATION_PIN	3

#define TABLE_INDEX_NUM					18
#define NEC_REPEAT_HEAD_SPACE_BIT_SIZE	4
#define BIT(n)							(1<<n)
#define STREAM_BIT_NUM					(200)

typedef enum {
  CODE_NEC,
  CODE_SONY,
  CODE_NUM
}code_t;

typedef enum {
  HEAD_PULSE,
  HEAD_SPACE,
  HEAD_NUM
}head_t;

typedef struct {
  code_t code;
  uint16_t carrier[CODE_NUM];
  uint16_t timebase[CODE_NUM];
  float dutycycle[CODE_NUM];
  uint8_t head_bit_size[CODE_NUM][HEAD_NUM];
  uint8_t address_length[CODE_NUM];
  uint8_t command_length[CODE_NUM];

  uint8_t index;
  uint8_t stream_index;
  bool stream_active;
  bool stream[STREAM_BIT_NUM];
}ir_t;

typedef void (*ir_callback_t)(void);


/*----------------------------------------------
  prototype
  ----------------------------------------------*/
extern const uint8_t ir_table[TABLE_INDEX_NUM][2];

/**
 * @brief Initializes IR generation with IR code/protocol.
 *
 * @param ir_code instance of ir_generate_init to initialize
 *
 * @return none
 *
 */
extern void ir_generate_init(code_t ir_code,ir_callback_t cb);

/**
 * @brief stop ir signal generate.
 *
 * @param none
 *
 * @return none
 *
 */
extern void ir_generate_stop(void);
/**
 * @brief configure ir signal stream.
 *
 * @param none
 *
 * @return none
 *
 */
extern void ir_generate_stream(uint16_t address, uint16_t command, bool repeat);

#ifdef __cplusplus
}
#endif
/** @} (end addtogroup IR Generator Driver) */
#endif
