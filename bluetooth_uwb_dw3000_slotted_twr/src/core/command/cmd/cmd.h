/**
 * @file      cmd.h
 *
 * @brief     Header file for command.c
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __CMD__H__
#define __CMD__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

/* command driver states */
typedef enum
{
  _NO_COMMAND = 0,
  _COMMAND_FOUND,
  _COMMAND_ALLOWED
}command_e;

/*Functionality modes names*/
#define NODE_FUNC      "NODE"
#define TAG_FUNC       "TAG"
#define TRILAT_FUNC    "TRILAT"
#define USPI_FUNC      "USPI"
#define TCWM_FUNC      "TCWM"
#define TCFM_FUNC      "TCFM"
#define LISTENER_FUNC  "LISTENER"

/*For Json parsing*/
#define CMD_NAME       "CMD_NAME"
#define CMD_PARAMS     "CMD_PARAMS"

/* @fn         command_driver
 * @brief    check if input text in known "COMMAND" or "PARAMETER=VALUE" format
 *             and executes COMMAND or set the PARAMETER to the VALUE
 * */
void command_parser(char *text);

#ifdef __cplusplus
}
#endif

#endif /* __CMD__H__ */
