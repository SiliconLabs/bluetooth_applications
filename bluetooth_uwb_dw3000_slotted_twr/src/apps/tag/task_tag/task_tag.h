/*!----------------------------------------------------------------------------
 * @file    header for task_tag.h
 *
 * @brief   Decawave Application Layer
 *          RTOS tag implementation
 *
 * @attention
 *
 * Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author
 *
 * Decawave
 */

#ifndef __TAG_TASK__H__
#define __TAG_TASK__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
typedef struct {
  uint16_t    addr;
  uint16_t    node_addr;
  uint8_t     rNum;
  int16_t     x_cm;
  int16_t     y_cm;
  int16_t     clkOffset_pphm;
}twr_res_ext_t;

void tag_helper(void const *argument);
void tag_terminate(void);

#ifdef __cplusplus
}
#endif

#endif /* __TWR_TASK__H__ */
