/***************************************************************************//**
* @file maxm86161_i2c.h
* @brief I2C device setup for maxm86161 driver.
* @version 1.0
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
* This code has been minimally tested to ensure that it builds with the specified dependency versions and is suitable as a demonstration for evaluation purposes only.
* This code will be maintained at the sole discretion of Silicon Labs.
*
******************************************************************************/
#ifndef MAXM86161_I2C_H_
#define MAXM86161_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "em_gpio.h"   //needed for GPIO_Port_TypeDef
#if (MAXM86161_BUS == MAXM86161_I2C)
  #include "em_i2c.h"
#elif (MAXM86161_BUS == MAXM86161_SPI)
  #include "spidrv.h"
#endif

/* Communicate with the MAXM86161 over I2C or SPI */
#define MAXM86161_I2C   0
#define MAXM86161_SPI   1
#define MAXM86161_BUS   MAXM86161_I2C

#define MAXM86161_SLAVE_ADDRESS         0xC4
#define MIKROE_I2C                      I2C1

#define MAXM86161_EN_GPIO_PORT          gpioPortC
#define MAXM86161_EN_GPIO_PIN           3

#define MAXM86161_INT_GPIO_PORT         gpioPortB
#define MAXM86161_INT_GPIO_PIN          3

 /** Button 0 for start and pause measurement */
#define MAXM86161_BTN0_GPIO_PORT        gpioPortC
#define MAXM86161_BTN0_GPIO_PIN         7

#define MIKROE_I2C_INIT_DEFAULT                                               \
{                                                                             \
    MIKROE_I2C,  /* Use I2C instance */                                       \
    gpioPortD,    /* SCL port */                                              \
    2,     /* SCL pin */                                                      \
    gpioPortD,    /* SDA port */                                              \
    3,     /* SDA pin */                                                      \
    0,                         /* Use currently configured reference clock */ \
    I2C_FREQ_STANDARD_MAX,     /* Set to standard rate  */                    \
    i2cClockHLRStandard,       /* Set to use 4:4 low/high duty cycle */       \
};

typedef struct Max86161PortConfig
{
#if (MAXM86161_BUS == MAXM86161_I2C)
  I2C_TypeDef       *i2cPort;   /**< I2C port Maxm86161 is connected to */
  uint8_t           i2cAddress; /**< I2C address of Maxm86161 */
#elif (MAXM86161_BUS == MAXM86161_SPI)
  SPIDRV_Handle_t   spiHandle;   /**< SPI Handle Maxm86161 is connected to */
  SPIDRV_Init_t     *spiPortConfig;  /**< SPIDRV Initialization struct. */

#endif
  GPIO_Port_TypeDef irqPort;    /**< Port for Maxm86161 INT pin */
  int               irqPin;     /**< Pin for Maxm86161 INT pin */
} Max86161PortConfig_t;


#endif

