/**
 * @file      cmd.c
 *
 * @brief     Command string as specified in document SWxxxx version X.x.x
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "cmd.h"
#include "cmd_fn.h"
#include "config.h"
#include "usb_uart_tx.h"
#include "cJSON.h"

/*
 *    Command interface
 */

/* We want cJSON to use a definitive malloc/free */
static const
cJSON_Hooks sCmdcJSON_hooks =
{
  .malloc_fn = CMD_MALLOC,
  .free_fn = CMD_FREE
};

/* IMPLEMENTATION */
void command_parser_init(void)
{
  cJSON_InitHooks((cJSON_Hooks *)&sCmdcJSON_hooks);
}

/*
 * @brief "error" will be sent if error during parser or command execution
 *   returned error
 * */
static void cmd_onERROR(const char *err)
{
  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    strcpy(str, "error \r\n");
    if (strlen(err) < (MAX_STR_SIZE - 6 - 3 - 1)) {
      strcpy(&str[6], err);
      strcpy(&str[6 + strlen(err)], "\r\n");
    }
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
  }
}

/* @fn      command_parser
 * @brief   checks if input "text" string in known "COMMAND" or "PARAMETER
 *   VALUE" format,
 *          checks their execution permissions, a VALUE range if restrictions
 *   and
 *          executes COMMAND or sets the PARAMETER to the VALUE
 * */
void command_parser(char *text)
{
  char *temp_str = text;
  uint8_t   cnt;
  command_e equal;
  int     val = 0;
  cJSON     *json_root, *json_params;
  char      cmd[20];
  const char  *ret;

  while (*temp_str)
  {
    *temp_str = (char)toupper(*temp_str);
    temp_str++;
  }

  /* Assume text may have more than one command inside.
   * For example "getKLIST\nnode 0\n" : this will execute 2 commands.
   * */
  text = strtok(text, "\n"); // get first token

  while (text != NULL)
  {
    equal = _NO_COMMAND;
    cnt = 0;
    json_params = NULL;
    json_root = NULL;
    cmd[0] = 0;  // Initialize no command

    if (*text == '{') {// Probably a Json command
      json_root = cJSON_Parse(text);
      if (json_root != NULL) {// Got valid Json command
        temp_str =
          cJSON_GetObjectItem(json_root,
                              CMD_NAME)->valuestring;  // Get command name
        if (temp_str != NULL) {// Got right command name
          json_params =
            cJSON_GetObjectItem(json_root,
                                CMD_PARAMS);  // Get command params
          if (json_params != NULL) {// We have a Json so we need to update
                                    //   command.
            sscanf(temp_str, "%9s", cmd);
          }
        }
      }
    } else {// It is not a Json command
      sscanf(text, "%9s %d", cmd, &val);
    }

    while (known_commands[cnt].cmnt != NULL)
    {
      if (known_commands[cnt].name
          && (strcmp(cmd, known_commands[cnt].name) == 0)) {
        equal = _COMMAND_FOUND;

        /* check command execution permissions.
         * some commands can be executed only from Idle system mode:
         * i.e. when no active processes are running.
         * other commands can be executed at any time.
         * */
        uint32_t mode = known_commands[cnt].mode & mMASK;

        if ((mode == app.mode) || (mode == mANY)) {
          equal = _COMMAND_ALLOWED;
        }
        break;
      }
      cnt++;
    }

    switch (equal)
    {
      case (_COMMAND_FOUND):
      {
        cmd_onERROR(" incompatible mode");
        break;
      }
      case (_COMMAND_ALLOWED):
      {
        /* execute corresponded fn() */
        param_block_t *pbss = get_pbssConfig();
        ret = known_commands[cnt].fn(text, pbss, val, json_params);

        if (ret) {
          port_tx_msg((uint8_t *)ret, strlen(ret));
        } else {
          cmd_onERROR(" function");
        }
        break;
      }
      default:
        break;
    }

    if (json_root != NULL) {
      cJSON_Delete(json_root);
    }

    text = strtok(NULL, "\n");
  }
}

/* end of cmd.c */
