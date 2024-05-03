/***************************************************************************//**
 * @file lfxoctune.c
 * @brief CTUNE calibration for LFXO
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: LicenseRef-MSLA
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement
 * By installing, copying or otherwise using this software, you agree to the
 * terms of the MSLA.
 *
 ******************************************************************************/

#include "lfxoctune.h"

#include "app_log.h"

#include "env.h"

#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"

#include <em_cmu.h>

#include <inttypes.h>

static void lfxo_ctune_ramp_to(unsigned int target)
{
  uint32_t reg;
  unsigned int i, current, amount;
  int shift;

  if (target > 0x4f) {
    target = 0x4f;
  }

  reg = LFXO->CAL;
  current = reg & _LFXO_CAL_CAPTUNE_MASK;
  reg &= ~_LFXO_CAL_CAPTUNE_MASK;

  if (current == target) {
    return;
  } else if (current < target) {
    amount = target - current;
    shift = 1;
  } else {
    amount = current - target;
    shift = -1;
  }

  for (i = 0; i < amount; i++)
  {
    current += shift;
    while (LFXO->SYNCBUSY != 0) {}
    LFXO->CAL = reg | (current << _LFXO_CAL_CAPTUNE_SHIFT);
    while (LFXO->SYNCBUSY != 0) {}
  }
}

static void lfxo_ctune_set_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: lfxo ctune set <ctune>\n");
  app_log_level(APP_LOG_LEVEL_INFO, "Where the <ctune> is in 0..79\n");

  app_log_nl();
}

static void lfxo_ctune_set(sl_cli_command_arg_t *arguments)
{
  char *endp = NULL;
  unsigned long val;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    lfxo_ctune_set_help();
    return;
  }

  val = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (val > 79) {
    app_log_level(APP_LOG_LEVEL_ERROR, "CTUNE value is out of range!\n");
    lfxo_ctune_set_help();
    return;
  }

  lfxo_ctune_ramp_to(val);

  app_log_level(APP_LOG_LEVEL_INFO, "Success.\n");
}

static void lfxo_ctune_get(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  uint32_t val;

  val = LFXO->CAL & _LFXO_CAL_CAPTUNE_MASK;

  app_log_level(APP_LOG_LEVEL_INFO, "LFXO CTUNE value is %" PRIu32 ".\n", val);
}

static void lfxo_ctune_save(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  sl_status_t ret;
  uint32_t val;

  val = LFXO->CAL & _LFXO_CAL_CAPTUNE_MASK;
  ret = env_set_lfxo_ctune(val);

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void lfxo_ctune_clear(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  sl_status_t ret;

  ret = env_clear_lfxo_ctune();

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static const sl_cli_command_info_t cmd_lfxo_ctune_set = \
  SL_CLI_COMMAND(lfxo_ctune_set,
                 "set the LFXO's CTUNE value",
                 "<ctune> - CTUNE value (0..79)",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_lfxo_ctune_get = \
  SL_CLI_COMMAND(lfxo_ctune_get,
                 "get the LFXO's CTUNE value",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_lfxo_ctune_save = \
  SL_CLI_COMMAND(lfxo_ctune_save,
                 "save the LFXO's current CTUNE value to non-volatile memory",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_lfxo_ctune_clear = \
  SL_CLI_COMMAND(lfxo_ctune_clear,
                 "clear the LFXO's CTUNE value from non-volatile memory",
                 "",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_lfxo_ctune[] = {
  { "set", &cmd_lfxo_ctune_set, false, },
  { "get", &cmd_lfxo_ctune_get, false, },
  { "save", &cmd_lfxo_ctune_save, false, },
  { "clear", &cmd_lfxo_ctune_clear, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_lfxo_ctune = \
  SL_CLI_COMMAND_GROUP(&cmdtable_lfxo_ctune,
                       "LFO CTUNE management commands");

static sl_cli_command_entry_t cmdtable_lfxo[] = {
  { "ctune", &cmd_lfxo_ctune, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_lfxo = \
  SL_CLI_COMMAND_GROUP(&cmdtable_lfxo,
                       "LFXO management");

static sl_cli_command_entry_t cmdtable[] = {
  { "lfxo", &cmd_lfxo, false, },
  { NULL, NULL, false, },
};

static sl_cli_command_group_t cmdgroup = {
  { NULL },
  false,
  cmdtable,
};

void lfxo_ctune_init(void)
{
  sl_status_t ret;
  unsigned int lfxo_ctune;

  ret = env_get_lfxo_ctune(&lfxo_ctune);
  if (ret == SL_STATUS_OK) {
    lfxo_ctune_ramp_to(lfxo_ctune);
    app_log_level(APP_LOG_LEVEL_INFO,
                  "LFXO CTUNE value has been adjusted to %u.\n",
                  lfxo_ctune);
  }

  sl_cli_command_add_command_group(sl_cli_inst_handle, &cmdgroup);
}
