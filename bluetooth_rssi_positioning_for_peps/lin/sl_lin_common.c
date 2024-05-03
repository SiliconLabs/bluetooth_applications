/***************************************************************************//**
 * @file sl_lin_common.c
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

#include "sl_lin_common.h"

#include <em_ramfunc.h>

#include <stddef.h>

sl_lin_irqhandler_t sl_lin_LETIMER0_IRQHandler = NULL;
sl_lin_irqhandler_t sl_lin_USART0_TX_IRQHandler = NULL;
sl_lin_irqhandler_t sl_lin_USART0_RX_IRQHandler = NULL;

SL_RAMFUNC_DEFINITION_BEGIN
void LETIMER0_IRQHandler(void)
{
  if (sl_lin_LETIMER0_IRQHandler) {
    sl_lin_LETIMER0_IRQHandler();
  }
}

SL_RAMFUNC_DEFINITION_END

SL_RAMFUNC_DEFINITION_BEGIN
void USART0_TX_IRQHandler(void)
{
  if (sl_lin_USART0_TX_IRQHandler) {
    sl_lin_USART0_TX_IRQHandler();
  }
}

SL_RAMFUNC_DEFINITION_END

SL_RAMFUNC_DEFINITION_BEGIN
void USART0_RX_IRQHandler(void)
{
  if (sl_lin_USART0_RX_IRQHandler) {
    sl_lin_USART0_RX_IRQHandler();
  }
}

SL_RAMFUNC_DEFINITION_END
