/***************************************************************************//**
* @file bma400._conf.h
* @brief Silicon Labs Empty Example Project
*
* Configuration filr for the BMA accelerometer driver
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
* # Evaluation Quality
* This code has been minimally tested to ensure that it builds and is suitable
* as a demonstration for evaluation purposes only. This code will be maintained
* at the sole discretion of Silicon Labs.
******************************************************************************/
#ifndef BMA400_CONFIG_H
#define BMA400_CONFIG_H

/* ########################################################################## */
/*                           System includes                                  */
/* ########################################################################## */

/* ########################################################################## */
/*                        Non system includes                                 */
/* ########################################################################## */
#include "em_gpio.h"

/* ########################################################################## */
/*                             Mecros Defn                                    */
/* ########################################################################## */
#define BMA400_DEBUG_MODE (0) /*Flag used for develpment */

/* BMA400 I2C address macros */
#define BMA400_I2C_ADDRESS_SDO_LOW  (0x14<<1)
#define BMA400_I2C_ADDRESS_SDO_HIGH (0x15<<1)

/* I2C HW defs */
#define BMA400_I2C0_PORT    gpioPortD
#define BMA400_I2C0_PIN_SDA (3)
#define BMA400_I2C0_PIN_SCL (2)

/* ########################################################################## */
/*                            Enum & and Typedefs                             */
/* ########################################################################## */

/* Interface selection enums */
typedef enum bma400_intf {
  eBMA400_Intfc_I2C0=0, /* I2C0 interface */
  eBMA400_Intfc_I2C1,   /* I2C1 interface */
  eBMA400_Intfc_Total   /* I2C1 interface */
} eBMA400_Intfc;

#endif
