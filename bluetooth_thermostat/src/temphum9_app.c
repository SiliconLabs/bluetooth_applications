/***************************************************************************//**
 * @file temphum9_app.c
 * @brief Measure temperature and humidity with SHTC3 Sensor
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include "mikroe_shtc3.h"
#include "app_assert.h"
#include "sl_i2cspm_instances.h"

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
void temphum9_app_init()
{
  sl_status_t sc;

  sc = mikroe_shtc3_init(sl_i2cspm_mikroe);
  app_assert_status(sc);

  sl_sleeptimer_delay_millisecond(100);
  mikroe_shtc3_send_command(MIKROE_SHTC3_CMD_WAKEUP);
  sl_sleeptimer_delay_millisecond(100);
  mikroe_shtc3_send_command(MIKROE_SHTC3_CMD_SLEEP);

  app_log("> App init done.\n");
  app_log("> Starting periodic measurement.\n");
}
