/***************************************************************************//**
 * @file mikroe_bg96_config.h
 * @brief mikroe_bg96_config.h
 * @version 1.0.0
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
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/
#ifndef MIKROE_BG96_CONFIG_H_
#define MIKROE_BG96_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

// <<< Use Configuration Wizard in Context Menu

// <h> LTE IOT2 BG96 Settings

// <o CMD_MAX_SIZE> Maximum AT Command Length
// <i> Default: 256
#define CMD_MAX_SIZE   256

// <o IN_BUFFER_SIZE> Size of buffer to receive AT Command response
// <i> Default: 256
#define IN_BUFFER_SIZE 256

// <o CMD_Q_SIZE> Size of queue to store at_cmd_desc_t
// <i> Default: 20
#define CMD_Q_SIZE     20

// <q BG96_ENALBLE_DEBUGOUT> Enable Debug Out
// <i> Default: 0
#define BG96_ENALBLE_DEBUGOUT       0

// </h>

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>

// <gpio optional=true> BG96_STA
// $[GPIO_BG96_STA]
#ifndef BG96_STA_PORT                           
#define BG96_STA_PORT                            SL_GPIO_PORT_B
#endif
#ifndef BG96_STA_PIN                            
#define BG96_STA_PIN                             0
#endif
// [GPIO_BG96_STA]$

// <gpio optional=true> BG96_PWK
// $[GPIO_BG96_PWK]
#ifndef BG96_PWK_PORT                           
#define BG96_PWK_PORT                            SL_GPIO_PORT_C
#endif
#ifndef BG96_PWK_PIN                            
#define BG96_PWK_PIN                             6
#endif
// [GPIO_BG96_PWK]$

// <<< sl:end pin_tool >>>

#ifdef __cplusplus
}
#endif

#endif // MIKROE_BG96_CONFIG_H_
