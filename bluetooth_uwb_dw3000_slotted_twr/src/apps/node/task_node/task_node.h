/**
 * @file node_task.h
 * @brief
 *
 * @author Decawave
 *
 * @attention Copyright 2017-2020 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __NODE_TASK__H__
#define __NODE_TASK__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern void signal_to_pc_new_tag_discovered(uint64_t addr64);

void node_helper(void const *argument);
void node_terminate(void);
void suspend_node_tasks(void);

#ifdef __cplusplus
}
#endif

#endif /* __TWR_TASK__H__ */
