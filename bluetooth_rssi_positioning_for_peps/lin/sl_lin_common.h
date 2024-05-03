/***************************************************************************//**
 * @file sl_lin_common.h
 * @brief Common parts of LIN bus driver
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
 ******************************************************************************/

#ifndef SL_LIN_COMMON__H
#define SL_LIN_COMMON__H

#define SL_LIN_MAX_ENDPOINT                      59
#define SL_LIN_INVALID_ENDPOINT                  255

#ifndef LIN_RX_PORT
#define LIN_RX_PORT                              gpioPortA
#endif
#ifndef LIN_RX_PIN
#define LIN_RX_PIN                               5
#endif

#ifndef LIN_TX_PORT
#define LIN_TX_PORT                              gpioPortA
#endif
#ifndef LIN_TX_PIN
#define LIN_TX_PIN                               6
#endif

typedef void (*sl_lin_irqhandler_t)(void);

extern sl_lin_irqhandler_t sl_lin_LETIMER0_IRQHandler;
extern sl_lin_irqhandler_t sl_lin_USART0_TX_IRQHandler;
extern sl_lin_irqhandler_t sl_lin_USART0_RX_IRQHandler;

#endif
