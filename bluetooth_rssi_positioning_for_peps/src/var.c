/***************************************************************************//**
 * @file var.c
 * @brief Shared variable handling
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#include "var.h"
#include "env.h"

#include "peps_follower.h"
#include "ui.h"

#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"

#include "app_log.h"

#include <stdlib.h>
#include <string.h>

int8_t var_central_rssi[ENV_LOCATION_LAST];
int8_t var_peripheral_rssi[ENV_LOCATION_LAST];
float var_angle[ENV_LOCATION_LAST];
double var_distance[ENV_LOCATION_LAST];

var_location_record_t var_location_record[ENV_LOCATION_LAST];

static int var_location_measurement_compare(const void *first,
                                            const void *second)
{
  var_location_measurement_t *left = (var_location_measurement_t *)first;
  var_location_measurement_t *right = (var_location_measurement_t *)second;

  return (left->distance
          < right->distance) ? -1 : ((left->distance
                                      == right->distance) ? 0 : 1);
}

bool var_location_measurement_add(env_device_location_t location,
                                  float distance,
                                  int8_t central_rssi,
                                  int8_t peripheral_rssi)
{
  var_location_record_t *loc = &var_location_record[location];
  unsigned int i;

  if (loc->measurements == VAR_MAX_LOCATION_MEASUREMENTS) {
    return false;
  }

  for (i = 0; i < loc->measurements; i++)
  {
    if (loc->measurement[i].distance == distance) {
      break;
    }
  }

  loc->measurement[i].distance = distance;
  loc->measurement[i].central_rssi = central_rssi;
  loc->measurement[i].peripheral_rssi = peripheral_rssi;

  if (i == loc->measurements) {
    loc->measurements++;
  }

  if (loc->measurements > 1) {
    qsort(loc->measurement,
          loc->measurements,
          sizeof(loc->measurement[0]),
          var_location_measurement_compare);
  }

  return true;
}

bool var_location_measurement_remove(env_device_location_t location,
                                     float distance)
{
  var_location_record_t *loc = &var_location_record[location];
  unsigned int i;

  if (loc->measurements == 0) {
    return false;
  }

  for (i = 0; i < loc->measurements; i++)
  {
    if (loc->measurement[i].distance == distance) {
      break;
    }
  }

  if (i == loc->measurements) {
    return false;
  }

  loc->measurements--;
  loc->measurement[i].distance = loc->measurement[loc->measurements].distance;
  loc->measurement[i].central_rssi =
    loc->measurement[loc->measurements].central_rssi;
  loc->measurement[i].peripheral_rssi =
    loc->measurement[loc->measurements].peripheral_rssi;

  if (loc->measurements > 1) {
    qsort(loc->measurement,
          loc->measurements,
          sizeof(loc->measurement[0]),
          var_location_measurement_compare);
  }

  return true;
}

bool var_location_measurement_flush(env_device_location_t location)
{
  var_location_record_t *loc = &var_location_record[location];

  loc->measurements = 0;

  return true;
}

static void var_set_rssi_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: var set rssi <Central RSSI> <Peripheral RSSI>\n");
  app_log_nl();
}

static void var_set_rssi(sl_cli_command_arg_t *arguments)
{
  long central_rssi;
  long peripheral_rssi;
  char *endp = NULL;
  sl_status_t ret = SL_STATUS_OK;

  if (sl_cli_get_argument_count(arguments) != 2) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    var_set_rssi_help();
    return;
  }

  central_rssi = strtol(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if ((central_rssi < -128) || (central_rssi > 20)) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Central RSSI is out of range!\n");
    var_set_rssi_help();
    return;
  }

  peripheral_rssi = strtol(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if ((peripheral_rssi < -128) || (peripheral_rssi > 20)) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Peripheral RSSI is out of range!\n");
    var_set_rssi_help();
    return;
  }

  switch (env_device_type)
  {
    case ENV_DEVICE_PEPS_FOLLOWER:
      ret = peps_follower_update_rssi(central_rssi, peripheral_rssi);
      break;

    case ENV_DEVICE_PEPS_LEADER:
      var_central_rssi[ENV_LOCATION_CENTER] = central_rssi;
      var_peripheral_rssi[ENV_LOCATION_CENTER] = peripheral_rssi;
      break;

    default:
      ret = SL_STATUS_FAIL;
  }

  if (ret == SL_STATUS_OK) {
    ui_set_rssi(central_rssi);
  }

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static const sl_cli_command_info_t cmd_var_set_rssi = \
  SL_CLI_COMMAND(var_set_rssi,
                 "set the RSSI measurement",
                 "<RSSI> - RSSI value to set",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_var_set[] = {
  { "rssi", &cmd_var_set_rssi, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_var_set = \
  SL_CLI_COMMAND_GROUP(&cmdtable_var_set,
                       "set the device attributes");

static sl_cli_command_entry_t cmdtable_var[] = {
  { "set", &cmd_var_set, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_var = \
  SL_CLI_COMMAND_GROUP(&cmdtable_var,
                       "variable management");

static sl_cli_command_entry_t cmdtable[] = {
  { "var", &cmd_var, false, },
  { NULL, NULL, false, },
};

static sl_cli_command_group_t cmdgroup = {
  { NULL },
  false,
  cmdtable,
};

void var_init(void)
{
  unsigned int i;

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    var_central_rssi[i] = -128;
    var_peripheral_rssi[i] = -128;
  }

  memset(var_location_record, 0, sizeof(var_location_record));

  sl_cli_command_add_command_group(sl_cli_inst_handle, &cmdgroup);
}
