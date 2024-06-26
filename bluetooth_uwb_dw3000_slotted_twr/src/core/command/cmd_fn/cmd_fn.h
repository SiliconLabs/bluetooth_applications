/**
 * @file      cmd_fn.h
 *
 * @brief     Header file for macros, structures and protypes cmd_fn.c
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef INC_CMD_FN_H_
#define INC_CMD_FN_H_    1

#ifdef __cplusplus
extern "C" {
#endif

#include "app.h"
#include "cJSON.h"

// -----------------------------------------------------------------------------

/* module DEFINITIONS */
#define MAX_STR_SIZE            255

#define CMD_MALLOC              pvPortMalloc
#define CMD_FREE                vPortFree
#define CMD_ENTER_CRITICAL()    taskENTER_CRITICAL()
#define CMD_EXIT_CRITICAL()     taskEXIT_CRITICAL()

typedef enum {
  mCmdGrp0 =  (0x1 << 16),      /* This group of commands is a delimiter */
  mCmdGrp1 =  (0x2 << 16),      /* This group of commands has format #1 */
  mCmdGrp2 =  (0x4 << 16),      /* below reserved for the future */
  mCmdGrp3 =  (0x8 << 16),
  mCmdGrp4 =  (0x10 << 16),
  mCmdGrp5 =  (0x20 << 16),
  mCmdGrp6 =  (0x40 << 16),
  mCmdGrp7 =  (0x80 << 16),
  mCmdGrp8 =  (0x100 << 16),
  mCmdGrp9 =  (0x200 << 16),
  mCmdGrp10=  (0x400 << 16),
  mCmdGrp11=  (0x800 << 16),
  mCmdGrp12=  (0x1000 << 16),
  mCmdGrp13=  (0x2000 << 16),
  mCmdGrp14=  (0x4000 << 16),
  mCmdGrp15=  (0x8000 << 16),
  mCmdGrpMASK = 0xFFFF0000L
}cmdGroup_e;

// -----------------------------------------------------------------------------

/* All cmd_fn functions have unified input: (char *text, param_block_t *pbss,
 *   int val)
 */

/* use REG_FN(x) macro */
#define REG_FN(x) const char *x(char *text,           \
                                param_block_t * pbss, \
                                int val,              \
                                cJSON * params)

/* command table structure definition */
struct command_s
{
  const char     *name;         /**< Command name string */
  const uint32_t mode;          /**< allowed execution operation mode */
  REG_FN((*fn));                /**< function() */
  const char     *cmnt;
};

typedef struct command_s command_t;
extern const command_t known_commands[];

#ifdef __cplusplus
}
#endif

#endif /* INC_CMD_FN_H_ */
