/***************************************************************************//**
 * @file app.h
 * @brief Silicon Labs Empty Example Project
 * This example demonstrates the implementation of IR signal generate and 
 * 4x4 matrix key scan with BLE on our EFR32 device(lynx). 
 * Need to ensure IR signal generate work well in case that BLE in a heavy communication.
 * @version 1.0.1
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

#ifndef APP_H_
#define APP_H_

#include "gecko_configuration.h"

/* DEBUG_LEVEL is used to enable/disable debug prints. Set DEBUG_LEVEL to 1 to enable debug prints */
#define DEBUG_LEVEL 1

#define D_KEYSCAN	1
#define D_IR		1

/* Set this value to 1 if you want to disable deep sleep completely */
#define DISABLE_SLEEP 0

#if DEBUG_LEVEL
#include "retargetserial.h"
#include <stdio.h>
#endif

#if DEBUG_LEVEL
#define initLog()     RETARGET_SerialInit()
#define flushLog()    RETARGET_SerialFlush()
#define printLog(...) printf(__VA_ARGS__)
#else
#define initLog()
#define flushLog()
#define printLog(...)
#endif

/* Main application */
void appMain(gecko_configuration_t *pconfig);

#endif
