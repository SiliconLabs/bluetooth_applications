/**
 * @file      task_flush.h
 *
 * @brief     header file for flush task
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __FLUSH_TASK__H__
#define __FLUSH_TASK__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

void FlushTask(void const *argument);
void FlushTask_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLUSH_TASK__H__ */
