/***************************************************************************//**
 * @file client_app.h
 * @brief client app source file.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
 * 1. The origin of this software must not be misrepresented{} you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include "sensor_app.h"

/***************************************************************************//**
 * @brief
 *    Application initialization for sensor device.
 *
 ******************************************************************************/
void sensor_app_init(void)
{
  BURTC_Init_TypeDef init_burtc = BURTC_INIT_DEFAULT;
  EMU_EM23Init_TypeDef init_em23 = EMU_EM23INIT_DEFAULT;
  EMU_EM4Init_TypeDef init_em4 = EMU_EM4INIT_DEFAULT;

  // Select reference clock/oscillator for the desired clock branch (BURTC Clk).
  // Reference selected for clocking: ULFRCO
  CMU_ClockSelectSet(cmuClock_BURTC, cmuSelect_ULFRCO);

  // Enable BURTC Clk
  CMU_ClockEnable(cmuClock_BURTC, true);

  // Enable BURTC
  BURTC_Enable(true);

  init_burtc.compare0Top = true;
  init_burtc.em4comp = true;
  init_burtc.em4overflow = true;

  // Enable Interrupt from BURTC
  NVIC_EnableIRQ(BURTC_IRQn);
  // Enable compare interrupt flag
  BURTC_IntEnable(BURTC_IF_COMP);

  // Init BURTC
  BURTC_Init(&init_burtc);

  // Init EM2
  EMU_EM23Init(&init_em23);

  // Init EM4
  EMU_EM4Init(&init_em4);
}
