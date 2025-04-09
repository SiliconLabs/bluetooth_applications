/***************************************************************************//**
 * @file client_app.c
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
#include "sl_sleeptimer.h"
#include "app_log.h"
#include "sl_pwm_instances.h"
#include "mikroe_cmt_8540s_smt.h"
#include "client_oled_app.h"
#include "client_nvm.h"
#include "client_app.h"

void client_app_init(void)
{
  // Init NVM
  client_config_nvm3_init();

  // Init Oled
  client_oled_app_init();

  // Init Buzzer 2 Clicks
  if (SL_STATUS_OK != mikroe_cmt_8540s_smt_init(&sl_pwm_mikroe)) {
    app_log("Buzzer 2 CLicks Init Failed\r\n");
  } else {
    app_log("Buzzer 2 CLicks Init Done\r\n");
    mikroe_cmt_8540s_smt_set_duty_cycle(0.0);
    mikroe_cmt_8540s_smt_pwm_start();
    sl_sleeptimer_delay_millisecond(100);
  }
}
